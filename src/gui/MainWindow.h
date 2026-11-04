/* MainWindow.h - 主窗口（复刻 CMainFrame 的打开/保存/缩放/取色/ISP/分析）*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRect>
#include "core/itypes.h"

class ImageView;
class QLabel;
class QAction;
class QScrollArea;
class QToolBar;
class QSpinBox;
class QComboBox;
class RawFormatDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool loadRawFile(const QString &path);    /* 供命令行/拖拽共用 */
    bool loadImageFile(const QString &path);
    void openAnalysisDialog();                /* 供命令行 --analysis 自动打开 */

private slots:

private slots:
    void openRaw();          /* Open(raw) — 原菜单 32947 */
    void openImage();        /* Open — 常规图像（QImage 支持）*/
    void saveImage();        /* Save — BMP/PPM/RAW16 */
    void zoomIn();           /* 原菜单 32786 */
    void zoomOut();          /* 原菜单 32787 */
    void normalSize();       /* 原菜单 32788 Normal Viewing (1:1) */
    void fitToWindow();      /* 缩放到整幅可见（大图打开时自动调用）*/
    void runIsp();           /* ISP 流水线运行 */
    void showAnalysis();     /* 原菜单 32932 Histogram / 32946 Step Analysis */
    void showIspPanel();
    void onPixelHovered(int x, int y, int r, int g, int b);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void applyZoom(double factor, const QPoint &anchorWidgetPos);
    void updateViewFromRgb();
    void updateStatusInfo();

    ImageView *m_view;
    QScrollArea *m_scroll;
    QLabel    *m_statusPix;
    QLabel    *m_statusInfo;

    ItConfig    m_cfg;
    ItRawFormat m_fmt;
    ItImage     m_bayer{};    /* 16bit Bayer 平面（{} 零初始化，data=nullptr=未打开）*/
    ItImage     m_simBayer{}; /* 仿真裁剪缓冲（丢首行）*/
    ItRgb       m_rgb{};      /* 当前显示 RGB */
    QRect       m_roi;        /* 分析区域（空=全图）*/
    bool        m_bypassIsp = false;  /* 原始预览：仅 demosaic 旁路 ISP */
    bool        m_fitMode = true;     /* 默认平铺：窗口/视口变化自动重算 */
    QString     m_fileName;

    QToolBar   *m_simBar;
    QSpinBox   *m_skipRows;   /* 仿真：丢弃首部行数 */
    QComboBox  *m_patCombo;   /* 仿真：显示用 pattern 重解析 */
    const ItImage *simulatedBayer();
};

#endif /* MAINWINDOW_H */
