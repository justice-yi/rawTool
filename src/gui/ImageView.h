/* ImageView.h - 缩放图像视图 + 像素取色（复刻 0x421850 的
 * "Bayer_%s of %s" / "RGBA: (%d, %d, %d, %d)" 交互）*/
#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QWidget>
#include <QImage>
#include <QString>
#include <QRect>

class ImageView : public QWidget {
    Q_OBJECT
public:
    explicit ImageView(QWidget *parent = nullptr);

    void setRgbImage(const QImage &img);
    void setBayerPixelText(bool valid, int x, int y, const QString &patternName);
    void setZoom(double factor);
    void setLoupeEnabled(bool on) { m_loupeEnabled = on; update(); }
    bool loupeEnabled() const { return m_loupeEnabled; }
    void clearSelection();
    QRect selection() const { return m_selRect; }
    double zoom() const { return m_zoom; }

signals:
    void pixelHovered(int x, int y, int r, int g, int b);
    void pixelClicked(int x, int y);
    void regionSelected(const QRect &roi);   /* 图像坐标 ROI */
    void zoomRequested(double factor, const QPoint &widgetPos);
    /* 滚轮缩放请求：MainWindow 以 widgetPos（控件坐标）为锚调整滚动 */

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    QSize sizeHint() const override;

private:
    QPointF toImageCoord(const QPoint &widgetPos) const;

    QImage  m_image;
    double  m_zoom = 1.0;
    bool    m_hasBayerText = false;
    QString m_bayerText;
    bool    m_selecting = false;
    QPoint  m_selStartImg;      /* 图像坐标 */
    QRect   m_selRect;          /* 当前框选（图像坐标，空=无）*/
    bool    m_loupeEnabled = true;
    QPoint  m_cursorPos;        /* 控件坐标（leaveEvent 后无效）*/
    bool    m_cursorInside = false;
    int     m_lastHoverX = -1;  /* 上次取色图像坐标（节流判定）*/
    int     m_lastHoverY = -1;
};

#endif /* IMAGEVIEW_H */
