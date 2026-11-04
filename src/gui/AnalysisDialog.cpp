/* AnalysisDialog.cpp - 统计分析页实现 */
#include "AnalysisDialog.h"
#include "core/analysis.h"
#include "core/imgio.h"
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>

AnalysisDialog::AnalysisDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Analysis"));
    resize(560, 480);

    QVBoxLayout *v = new QVBoxLayout(this);

    m_histLabel = new QLabel;
    m_histLabel->setMinimumSize(520, 160);
    m_histLabel->setAlignment(Qt::AlignCenter);
    m_histLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    m_rawHistLabel = new QLabel;
    m_rawHistLabel->setMinimumSize(520, 160);
    m_rawHistLabel->setAlignment(Qt::AlignCenter);
    m_rawHistLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    m_histTabs = new QTabWidget;
    m_histTabs->addTab(m_histLabel, tr("RGB Histogram"));
    m_histTabs->addTab(m_rawHistLabel, tr("Raw Histogram"));
    v->addWidget(m_histTabs);

    m_table = new QTableWidget(3, 5);
    m_table->setHorizontalHeaderLabels({ tr("Stat"), tr("Y"), tr("R"),
                                         tr("G"), tr("B") });
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    v->addWidget(m_table, 1);

    m_roiLabel = new QLabel(tr("Region : full image"));
    v->addWidget(m_roiLabel);

    QHBoxLayout *h = new QHBoxLayout;
    m_saveBtn = new QPushButton(tr("Save Summary..."));
    connect(m_saveBtn, &QPushButton::clicked, this,
            &AnalysisDialog::saveSummary);
    QPushButton *csvBtn = new QPushButton(tr("Export CSV..."));
    connect(csvBtn, &QPushButton::clicked, this, &AnalysisDialog::exportCsv);
    h->addStretch(1);
    h->addWidget(m_saveBtn);
    h->addWidget(csvBtn);
    v->addLayout(h);
}

void AnalysisDialog::analyze(const QImage &img, const QString &name,
                             const QRect &roi,
                             const ItImage *raw, int rawBits)
{
    if (img.isNull())
        return;
    m_img  = img;
    m_name = name;
    m_roi  = roi.isValid() && roi.width() > 0 && roi.height() > 0
                 ? roi.intersected(img.rect()) : QRect();

    ItRgb rgb;
    rgb.width  = img.width();
    rgb.height = img.height();
    rgb.data   = const_cast<uint8_t *>(img.constBits());

    /* ROI 掩码：框选区域内 1，其余 0（原 DIALOG 178 区域 SNR）*/
    ItImage mask;
    ItImage *maskPtr = nullptr;
    if (!m_roi.isNull()) {
        if (it_image_alloc(&mask, img.width(), img.height()) == IT_OK) {
            for (int y = m_roi.top(); y <= m_roi.bottom(); y++)
                for (int x = m_roi.left(); x <= m_roi.right(); x++)
                    mask.data[(long)y * img.width() + x] = 1;
            maskPtr = &mask;
        } else {
            m_roi = QRect();    /* 掩码分配失败：退化为全图并如实显示 */
        }
    }

    ItStatus st = it_stat_compute(&rgb, maskPtr, &m_stat);
    if (maskPtr)
        it_image_free(&mask);
    if (st != IT_OK)
        return;

    m_roiLabel->setText(m_roi.isNull()
        ? tr("Region : full image")
        : tr("Region : (%1, %2) %3 x %4")
              .arg(m_roi.x()).arg(m_roi.y())
              .arg(m_roi.width()).arg(m_roi.height()));

    const char *rows[3] = { "Mean", "Std", "SNR" };
    double vals[3][4] = {
        { m_stat.mean[0], m_stat.mean[1], m_stat.mean[2], m_stat.mean[3] },
        { m_stat.std[0],  m_stat.std[1],  m_stat.std[2],  m_stat.std[3] },
        { m_stat.snr[0],  m_stat.snr[1],  m_stat.snr[2],  m_stat.snr[3] },
    };
    m_table->setRowCount(3);
    for (int r = 0; r < 3; r++) {
        m_table->setItem(r, 0, new QTableWidgetItem(QString(rows[r])));
        for (int c = 0; c < 4; c++)
            m_table->setItem(r, c + 1, new QTableWidgetItem(
                QString::number(vals[r][c], 'f', 2)));
    }

    drawHistogram();
    drawRawHistogram(raw, rawBits);
}

void AnalysisDialog::drawRawHistogram(const ItImage *raw, int bits)
{
    if (!raw || !raw->data) {
        m_rawHistPixmap = QPixmap();
        m_rawHistLabel->setText(tr("No raw data"));
        return;
    }
    long hist[256];
    if (it_hist_compute(raw, bits, hist) != IT_OK) {
        m_rawHistLabel->setText(tr("No raw data"));
        return;
    }
    long mx = 1;
    for (int i = 0; i < 256; i++)
        if (hist[i] > mx) mx = hist[i];

    const int W = 520, H = 160;
    m_rawHistPixmap = QPixmap(W, H);
    m_rawHistPixmap.fill(Qt::black);
    QPainter p(&m_rawHistPixmap);
    p.setPen(QColor(200, 200, 90));
    for (int i = 0; i < 256; i++) {
        int hh = int(hist[i] * (H - 4) / mx);
        p.drawLine(i * 2, H - 1, i * 2, H - 1 - hh);
    }
    m_rawHistLabel->setPixmap(m_rawHistPixmap);
}

void AnalysisDialog::drawHistogram()
{
    if (m_img.isNull())
        return;
    long histR[256] = { 0 }, histG[256] = { 0 }, histB[256] = { 0 };
    for (int y = 0; y < m_img.height(); y++) {
        /* m_img 为 Format_RGB888（3 字节/像素）——按字节读，勿当 QRgb(4B) */
        const uchar *line = m_img.constScanLine(y);
        for (int x = 0; x < m_img.width(); x++) {
            histR[line[x * 3 + 0]]++;
            histG[line[x * 3 + 1]]++;
            histB[line[x * 3 + 2]]++;
        }
    }
    long mx = 1;
    for (int i = 0; i < 256; i++)
        mx = qMax(mx, qMax(histR[i], qMax(histG[i], histB[i])));

    const int W = 520, H = 160;
    m_histPixmap = QPixmap(W, H);
    m_histPixmap.fill(Qt::black);
    QPainter p(&m_histPixmap);
    for (int i = 0; i < 256; i++) {
        int hr = int(histR[i] * (H - 4) / mx);
        int hg = int(histG[i] * (H - 4) / mx);
        int hb = int(histB[i] * (H - 4) / mx);
        p.setPen(Qt::red);   p.drawLine(i * 2, H - 1, i * 2, H - 1 - hr);
        p.setPen(Qt::green); p.drawLine(i * 2 + 1, H - 1, i * 2 + 1, H - 1 - hg);
        p.setPen(Qt::blue);  p.drawLine(i * 2, H - 1, i * 2, H - 1 - hb);
    }
    m_histLabel->setPixmap(m_histPixmap);
}

void AnalysisDialog::saveSummary()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save file"), "hist_summary.txt",
        tr("Text files (*.txt);;All files (*.*)"));
    if (path.isEmpty())
        return;
    ItStatus st = it_stat_write_summary(path.toLocal8Bit().constData(),
                                        m_name.toLocal8Bit().constData(),
                                        &m_stat);
    if (st != IT_OK)
        QMessageBox::warning(this, tr("Save file"),
                             tr("Save failed: %1").arg(it_status_str(st)));
}

void AnalysisDialog::exportCsv()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Export CSV"), "statistic_summary.csv",
        tr("CSV files (*.csv);;All files (*.*)"));
    if (path.isEmpty())
        return;
    ItStatus st = it_stat_write_csv(path.toLocal8Bit().constData(),
                                    m_name.toLocal8Bit().constData(),
                                    &m_stat);
    if (st != IT_OK)
        QMessageBox::warning(this, tr("Export CSV"),
                             tr("Export failed: %1").arg(it_status_str(st)));
}
