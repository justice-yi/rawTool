/* MainWindow.cpp - 主窗口实现 */
#include "MainWindow.h"
#include "ImageView.h"
#include "RawFormatDialog.h"
#include "IspPanelDialog.h"
#include "AnalysisDialog.h"
#include "core/raw_loader.h"
#include "core/isp_pipeline.h"
#include "core/imgio.h"
#include "core/bayer.h"

#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolBar>
#include <QSpinBox>
#include <QComboBox>
#include <QResizeEvent>
#include <QImage>
#include <QFileInfo>
#include <QSettings>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <cstring>
#include <cstdlib>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("ImageTool Qt - Raw Viewer");
    setAcceptDrops(true);

    m_view = new ImageView;
    m_scroll = new QScrollArea;
    m_scroll->setWidget(m_view);
    m_scroll->setWidgetResizable(false);
    setCentralWidget(m_scroll);
    connect(m_view, &ImageView::zoomRequested, this,
            [this](double factor, const QPoint &pos) {
                m_fitMode = false;   /* 手动滚轮缩放退出平铺模式 */
                applyZoom(factor, pos);
            });

    m_statusPix  = new QLabel(this);
    m_statusInfo = new QLabel(this);
    statusBar()->addWidget(m_statusPix, 1);
    statusBar()->addPermanentWidget(m_statusInfo);
    connect(m_view, &ImageView::pixelHovered,
            this, &MainWindow::onPixelHovered);
    connect(m_view, &ImageView::regionSelected, this,
            [this](const QRect &roi) { m_roi = roi; });

    /* ---- 菜单（对应原 IDR_MAINFRAME 菜单 + 自定义 RAW 项）---- */
    QMenu *mFile = menuBar()->addMenu(tr("&File"));
    mFile->addAction(tr("Open Image..."), QKeySequence::Open,
                     this, &MainWindow::openImage);
    QAction *openRawAct = mFile->addAction(tr("Open (&Raw)..."),
                                           this, &MainWindow::openRaw);
    openRawAct->setShortcut(QKeySequence("Ctrl+Shift+O"));
    mFile->addAction(tr("&Save As..."), QKeySequence::Save,
                     this, &MainWindow::saveImage);
    mFile->addSeparator();
    mFile->addAction(tr("E&xit"), QKeySequence::Quit,
                     this, &QWidget::close);

    QMenu *mView = menuBar()->addMenu(tr("&View"));
    mView->addAction(tr("Zoom &In"), QKeySequence::ZoomIn,
                     this, &MainWindow::zoomIn);
    mView->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut,
                     this, &MainWindow::zoomOut);
    mView->addAction(tr("&Normal Viewing (1:1)"),
                     this, &MainWindow::normalSize);
    mView->addAction(tr("&Fit to Window"), QKeySequence("Ctrl+F"),
                     this, &MainWindow::fitToWindow);
    QAction *loupeAct = mView->addAction(tr("&Magnifier"));
    loupeAct->setCheckable(true);
    loupeAct->setChecked(true);
    connect(loupeAct, &QAction::toggled, m_view, &ImageView::setLoupeEnabled);
    QAction *bypassAct = mView->addAction(tr("Bypass ISP (&raw preview)"));
    bypassAct->setCheckable(true);
    bypassAct->setChecked(false);
    connect(bypassAct, &QAction::toggled, this, [this](bool on) {
        m_bypassIsp = on;
        runIsp();       /* 切换后立即以新模式重新处理 */
    });

    /* ---- 仿真工具栏：丢首行 + pattern 重解析（sensor 调试高频场景）---- */
    m_simBar = addToolBar(tr("Simulate"));
    m_simBar->addWidget(new QLabel(tr(" 丢首行 ")));
    m_skipRows = new QSpinBox;
    m_skipRows->setRange(0, 255);
    m_skipRows->setFixedWidth(60);
    m_skipRows->setToolTip(tr("Simulate lost first N lines: drop N top "
                              "rows, then re-parse with the pattern"));
    connect(m_skipRows, &QSpinBox::valueChanged, this, [this](int) {
        m_roi = QRect();            /* 有效尺寸变化，区域失效 */
        m_view->clearSelection();
        runIsp();
    });
    m_simBar->addWidget(m_skipRows);

    m_simBar->addWidget(new QLabel(tr("   Pattern ")));
    m_patCombo = new QComboBox;
    m_patCombo->addItem(tr("File default"), -1);
    m_patCombo->addItem("RGGB", int(IT_PAT_RGGB));
    m_patCombo->addItem("GRBG", int(IT_PAT_GRBG));
    m_patCombo->addItem("GBRG", int(IT_PAT_GBRG));
    m_patCombo->addItem("BGGR", int(IT_PAT_BGGR));
    connect(m_patCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { runIsp(); });
    m_simBar->addWidget(m_patCombo);

    QMenu *mIsp = menuBar()->addMenu(tr("&ISP"));
    mIsp->addAction(tr("&Parameters..."), this, &MainWindow::showIspPanel);
    mIsp->addAction(tr("&Run Pipeline"), QKeySequence("F5"),
                    this, &MainWindow::runIsp);

    QMenu *mAn = menuBar()->addMenu(tr("Analysis"));
    mAn->addAction(tr("&Histogram / SNR..."), QKeySequence("F7"),
                   this, &MainWindow::showAnalysis);

    it_config_default(&m_cfg);
    m_fmt.bits    = IT_BITS_10;
    m_fmt.order   = IT_BYTEORDER_LH;
    m_fmt.width   = 0;
    m_fmt.height  = 0;
    m_fmt.pattern = IT_PAT_RGGB;

    /* 记忆上次会话的 RAW 格式（对应原程序注册表持久化 OPEN_DIR 的思路）*/
    QSettings s;
    m_fmt.bits    = (ItBitDepth)s.value("rawformat/bits",
                                        int(IT_BITS_10)).toInt();
    m_fmt.order   = (ItByteOrder)s.value("rawformat/order",
                                         int(IT_BYTEORDER_LH)).toInt();
    m_fmt.width   = s.value("rawformat/width", 0).toInt();
    m_fmt.height  = s.value("rawformat/height", 0).toInt();
    m_fmt.pattern = (ItBayerPattern)s.value("rawformat/pattern",
                                            int(IT_PAT_RGGB)).toInt();
    m_fmt.dummy_rows = s.value("rawformat/dummy_rows", 0).toInt();
    m_fmt.dummy_cols = s.value("rawformat/dummy_cols", 0).toInt();
    if (m_fmt.bits != IT_BITS_8 && m_fmt.bits != IT_BITS_10 &&
        m_fmt.bits != IT_BITS_16)
        m_fmt.bits = IT_BITS_10;
    if (m_fmt.pattern < IT_PAT_RGGB || m_fmt.pattern > IT_PAT_BGGR)
        m_fmt.pattern = IT_PAT_RGGB;
    if (m_fmt.dummy_rows < 0) m_fmt.dummy_rows = 0;
    if (m_fmt.dummy_cols < 0) m_fmt.dummy_cols = 0;
}

/* ---------- 打开 RAW ---------- */
void MainWindow::openRaw()
{
    QSettings s;
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open file"), s.value("open/lastDir").toString(),
        tr("RAW files (*.raw *.byr *.dat *.sraw);;All files (*.*)"));
    if (!path.isEmpty())
        loadRawFile(path);
}

/* 菜单与拖拽共用的 RAW 加载路径 */
bool MainWindow::loadRawFile(const QString &path)
{
    ItRawFormat fmt = m_fmt;
    fmt.width = 0;      /* 触发尺寸推断 */
    fmt.height = 0;
    ItStatus st = it_raw_guess_format(path.toLocal8Bit().constData(), &fmt);
    if (st != IT_OK && st != IT_E_SIZE) {
        QMessageBox::warning(this, tr("File Open Error"),
                             it_status_str(st));
        return false;
    }

    RawFormatDialog dlg(fmt, this);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    dlg.applyToFormat();

    int ascii = (path.endsWith(".dat", Qt::CaseInsensitive)) ? 1 : 0;
    ItImage img;
    st = it_raw_load(path.toLocal8Bit().constData(), &fmt, ascii, &img);
    if (st != IT_OK) {
        QMessageBox::warning(this, tr("File Open Error"), it_status_str(st));
        return false;
    }

    it_image_free(&m_bayer);
    m_bayer    = img;
    m_fmt      = fmt;
    m_fileName = QFileInfo(path).fileName();
    m_cfg.pat_sel = fmt.pattern;
    m_roi = QRect();
    m_view->clearSelection();

    QSettings s;
    s.setValue("open/lastDir", QFileInfo(path).absolutePath());
    s.setValue("rawformat/bits",    int(m_fmt.bits));
    s.setValue("rawformat/order",   int(m_fmt.order));
    s.setValue("rawformat/width",   m_fmt.width);
    s.setValue("rawformat/height",  m_fmt.height);
    s.setValue("rawformat/pattern", int(m_fmt.pattern));
    s.setValue("rawformat/dummy_rows", m_fmt.dummy_rows);
    s.setValue("rawformat/dummy_cols", m_fmt.dummy_cols);

    updateStatusInfo();
    runIsp();                       /* 打开后立即跑一遍默认流水线 */
    fitToWindow();                  /* 大图自动缩放到整幅可见 */
    return true;
}

/* ---------- 打开常规图像 ---------- */
void MainWindow::openImage()
{
    QSettings s;
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open file"), s.value("open/lastDir").toString(),
        tr("Images (*.bmp *.jpg *.jpeg *.png *.tif *.tiff *.gif);;All files (*.*)"));
    if (!path.isEmpty())
        loadImageFile(path);
}

/* 菜单与拖拽共用的常规图像加载路径 */
bool MainWindow::loadImageFile(const QString &path)
{
    QImage img(path);
    if (img.isNull()) {
        QMessageBox::warning(this, tr("File Open Error"),
                             tr("Unsupported image file"));
        return false;
    }
    it_rgb_free(&m_rgb);
    QImage conv = img.convertToFormat(QImage::Format_RGB888);
    m_rgb.width  = conv.width();
    m_rgb.height = conv.height();
    m_rgb.data   = (uint8_t *)malloc(size_t(conv.width()) * conv.height() * 3);
    if (!m_rgb.data) {
        QMessageBox::warning(this, tr("File Open Error"), "out of memory");
        return false;
    }
    memcpy(m_rgb.data, conv.constBits(), size_t(conv.width()) * conv.height() * 3);
    m_fileName = QFileInfo(path).fileName();
    it_image_free(&m_bayer);
    m_roi = QRect();
    m_view->clearSelection();
    QSettings s;
    s.setValue("open/lastDir", QFileInfo(path).absolutePath());
    updateStatusInfo();
    updateViewFromRgb();
    fitToWindow();
    return true;
}

/* ---------- 拖拽打开 ---------- */
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;
    const QString path = urls.first().toLocalFile();
    if (path.isEmpty())
        return;
    if (path.endsWith(".raw", Qt::CaseInsensitive) ||
        path.endsWith(".byr", Qt::CaseInsensitive) ||
        path.endsWith(".dat", Qt::CaseInsensitive) ||
        path.endsWith(".sraw", Qt::CaseInsensitive))
        loadRawFile(path);
    else
        loadImageFile(path);
    event->acceptProposedAction();
}

/* ---------- 保存 ---------- */
void MainWindow::saveImage()
{
    if (!m_rgb.data) {
        QMessageBox::information(this, tr("Save file"), tr("No image"));
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save file"), QString(),
        tr("Bmp files (*.bmp);;Raw files (*.raw);;PPM (*.ppm)"));
    if (path.isEmpty())
        return;
    if (path.endsWith(".raw", Qt::CaseInsensitive) && !m_bayer.data) {
        QMessageBox::information(this, tr("Save file"),
                                 tr("RAW saving requires a RAW source "
                                    "(open a .raw file first)"));
        return;
    }
    QByteArray p8 = path.toLocal8Bit();
    ItStatus st;
    if (path.endsWith(".bmp", Qt::CaseInsensitive))
        st = it_bmp_write(p8.constData(), &m_rgb);
    else if (path.endsWith(".ppm", Qt::CaseInsensitive))
        st = it_ppm_rgb_write(p8.constData(), &m_rgb);
    else
        st = it_raw16_write(p8.constData(), &m_bayer);
    if (st != IT_OK)
        QMessageBox::warning(this, tr("Save file"), it_status_str(st));
}

/* ---------- 缩放（anchor 为控件坐标锚点；视口外则取视口中心）---------- */
void MainWindow::applyZoom(double factor, const QPoint &anchorWidgetPos)
{
    double oldZoom = m_view->zoom();
    double newZoom = oldZoom * factor;
    if (newZoom < 0.05) newZoom = 0.05;
    if (newZoom > 40.0) newZoom = 40.0;
    if (newZoom == oldZoom)
        return;

    QPoint anchor = anchorWidgetPos;
    QSize vp = m_scroll->viewport()->size();
    if (anchor.x() < 0 || anchor.y() < 0 ||
        anchor.x() > vp.width() || anchor.y() > vp.height())
        anchor = QPoint(vp.width() / 2, vp.height() / 2);

    /* 锚点处的图像坐标在缩放前后都映射到同一控件位置 */
    double imgX = anchor.x() / oldZoom;
    double imgY = anchor.y() / oldZoom;
    m_view->setZoom(newZoom);
    m_scroll->horizontalScrollBar()->setValue(int(imgX * newZoom - anchor.x()));
    m_scroll->verticalScrollBar()->setValue(int(imgY * newZoom - anchor.y()));
}

void MainWindow::zoomIn()    { m_fitMode = false; applyZoom(1.25, QPoint(-1, -1)); }
void MainWindow::zoomOut()   { m_fitMode = false; applyZoom(0.8,  QPoint(-1, -1)); }
void MainWindow::normalSize(){ m_fitMode = false; applyZoom(1.0 / m_view->zoom(), QPoint(-1, -1)); }

/* 平铺模式：视口尺寸变化时自动重算（窗口缩放/最大化跟随）*/
void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (m_fitMode)
        fitToWindow();
    QMainWindow::resizeEvent(event);
}

void MainWindow::fitToWindow()
{
    if (!m_rgb.data)
        return;
    m_fitMode = true;
    QSize vp = m_scroll->viewport()->size();
    double z = qMin(double(vp.width()) / m_rgb.width,
                    double(vp.height()) / m_rgb.height);
    if (z >= 1.0)
        z = 1.0;             /* 小图保持 1:1，不放大糊化 */
    applyZoom(z / m_view->zoom(), QPoint(-1, -1));
}

/* ---------- ISP ---------- */
void MainWindow::showIspPanel()
{
    IspPanelDialog dlg(m_cfg, this);
    if (dlg.exec() == QDialog::Accepted)
        dlg.applyToConfig();
}

/* 仿真路径：丢首行裁剪 + 显示用 pattern 重解析 */
const ItImage *MainWindow::simulatedBayer()
{
    it_image_free(&m_simBayer);
    int skip = m_skipRows->value();
    if (skip > 0 && m_bayer.height > skip) {
        if (it_image_crop(&m_bayer, skip, 0, m_bayer.width,
                          m_bayer.height - skip,
                          &m_simBayer) == IT_OK)
            return &m_simBayer;
    }
    return &m_bayer;                    /* 无仿真：原始平面 */
}

void MainWindow::runIsp()
{
    if (!m_bayer.data)
        return;                         /* 无图时静默返回（仿真控件触发）*/
    const ItImage *src = simulatedBayer();

    ItConfig cfg = m_cfg;               /* pattern 重解析仅作用于显示 */
    int pat = m_patCombo->currentData().toInt();
    if (pat >= 0)
        cfg.pat_sel = (ItBayerPattern)pat;

    it_rgb_free(&m_rgb);
    ItStatus st = m_bypassIsp
        ? it_isp_preview(src, &cfg, &m_rgb)
        : it_isp_run(src, &cfg, 1, &m_rgb);
    if (st != IT_OK) {
        QMessageBox::warning(this, tr("ISP"), it_status_str(st));
        return;
    }
    updateStatusInfo();
    updateViewFromRgb();
}

/* ---------- 分析 ---------- */
void MainWindow::showAnalysis()
{
    if (!m_rgb.data) {
        QMessageBox::information(this, tr("Analysis"), tr("No image"));
        return;
    }
    QImage img(m_rgb.width, m_rgb.height, QImage::Format_RGB888);
    for (int y = 0; y < m_rgb.height; y++)
        memcpy(img.scanLine(y), m_rgb.data + (long)y * m_rgb.width * 3,
               size_t(m_rgb.width) * 3);
    AnalysisDialog dlg;
    dlg.analyze(img, m_fileName, m_roi,
                m_bayer.data ? &m_bayer : nullptr, m_fmt.bits);
    dlg.exec();
}

/* ---------- 取色状态栏（复刻 " I : %d, RGBA : %d, %d, %d, %d"）---------- */
void MainWindow::onPixelHovered(int x, int y, int r, int g, int b)
{
    m_statusPix->setText(tr(" I : %1, RGBA : %2, %3, %4, %5")
                             .arg(x).arg(r).arg(g).arg(b).arg(255));
    const char *pat = it_bayer_pattern_name(m_cfg.pat_sel);
    m_view->setBayerPixelText(m_bayer.data != nullptr, x, y, pat);
}

/* ---------- 内部 ---------- */
void MainWindow::updateViewFromRgb()
{
    if (!m_rgb.data)
        return;
    QImage img(m_rgb.width, m_rgb.height, QImage::Format_RGB888);
    for (int y = 0; y < m_rgb.height; y++)
        memcpy(img.scanLine(y), m_rgb.data + (long)y * m_rgb.width * 3,
               size_t(m_rgb.width) * 3);
    m_view->setRgbImage(img);
}

void MainWindow::updateStatusInfo()
{
    int w = m_rgb.width, h = m_rgb.height;
    if (m_bayer.data) {                 /* RAW 模式显示仿真后的有效尺寸 */
        w = m_bayer.width;
        h = m_bayer.height - m_skipRows->value();
        if (h < 1) h = 1;
    }
    if (w <= 0)
        return;
    /* 复刻 0x4362a0 的信息行格式 */
    m_statusInfo->setText(tr("Width x Height   : %1 x %2")
                              .arg(w).arg(h));
}
