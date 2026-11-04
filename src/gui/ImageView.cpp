/* ImageView.cpp - 缩放视图实现 */
#include "ImageView.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

ImageView::ImageView(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(320, 240);
}

void ImageView::setRgbImage(const QImage &img)
{
    m_image = img;
    m_lastHoverX = -1;      /* 新图像：节流锚点失效，强制下次移动取色 */
    m_selRect = QRect();
    resize(sizeHint());     /* 滚动区 widgetResizable=false：须自扩尺寸 */
    update();
}

void ImageView::setBayerPixelText(bool valid, int x, int y,
                                  const QString &patternName)
{
    /* 复刻原程序取色文本：Bayer_<pattern> of <x>,<y> */
    m_hasBayerText = valid;
    if (valid)
        m_bayerText = QString("Bayer_%1 of (%2, %3)")
                          .arg(patternName).arg(x).arg(y);
    update();
}

void ImageView::setZoom(double factor)
{
    if (factor <= 0.0)
        return;
    m_zoom = factor;
    resize(sizeHint());     /* 与 setRgbImage 同理：尺寸随缩放变化 */
    update();
}

void ImageView::clearSelection()
{
    m_selRect = QRect();
    m_selecting = false;
    update();
}

QPointF ImageView::toImageCoord(const QPoint &widgetPos) const
{
    if (m_image.isNull() || m_zoom <= 0.0)
        return QPointF(-1, -1);
    return QPointF(widgetPos.x() / m_zoom, widgetPos.y() / m_zoom);
}

void ImageView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(40, 40, 40));
    if (m_image.isNull())
        return;
    p.setRenderHint(QPainter::SmoothPixmapTransform, m_zoom < 1.0);
    p.scale(m_zoom, m_zoom);
    p.drawImage(0, 0, m_image);
    p.resetTransform();

    /* 框选叠加层（把图像坐标 ROI 映射回控件坐标绘制）*/
    if (!m_selRect.isNull()) {
        QPen pen(Qt::cyan, 1, Qt::DashLine);
        p.setPen(pen);
        p.setBrush(QColor(0, 255, 255, 30));
        p.drawRect(int(m_selRect.x() * m_zoom), int(m_selRect.y() * m_zoom),
                   int(m_selRect.width() * m_zoom),
                   int(m_selRect.height() * m_zoom));
    }

    if (m_hasBayerText) {
        p.setPen(Qt::yellow);
        p.drawText(8, height() - 8, m_bayerText);
    }

    /* 放大镜：光标处 15x15 像素以最近邻放大到 150x150，右上角显示 */
    if (m_loupeEnabled && m_cursorInside && !m_image.isNull()) {
        QPointF ip = toImageCoord(m_cursorPos);
        int cx = int(ip.x()), cy = int(ip.y());
        if (cx >= 0 && cy >= 0 && cx < m_image.width() && cy < m_image.height()) {
            const int SRC = 15, DST = 150;
            QRect src = QRect(cx - SRC / 2, cy - SRC / 2, SRC, SRC)
                            .intersected(m_image.rect());   /* 边缘钳位 */
            QRect dst(width() - DST - 8, 8, DST, DST);
            p.setRenderHint(QPainter::SmoothPixmapTransform, false);
            p.drawImage(dst, m_image, src);
            p.setRenderHint(QPainter::SmoothPixmapTransform, true);
            p.setPen(Qt::white);
            p.drawRect(dst);
            p.setPen(QColor(255, 255, 255, 140));
            p.drawLine(dst.center().x(), dst.top(),
                       dst.center().x(), dst.bottom());
            p.drawLine(dst.left(), dst.center().y(),
                       dst.right(), dst.center().y());
        }
    }
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    m_cursorPos = event->pos();
    m_cursorInside = true;
    QPointF ip = toImageCoord(event->pos());
    int x = int(ip.x()), y = int(ip.y());
    if (m_image.isNull() || x < 0 || y < 0 ||
        x >= m_image.width() || y >= m_image.height())
        return;
    if (m_selecting) {
        m_selRect = QRect(m_selStartImg, QPoint(x, y)).normalized();
        update();
    }
    /* 节流：同像素内移动时取色文本与放大镜内容不变，跳过重绘 */
    bool samePixel = (x == m_lastHoverX && y == m_lastHoverY);
    m_lastHoverX = x;
    m_lastHoverY = y;
    if (samePixel && !m_selecting)
        return;
    QColor c = m_image.pixelColor(x, y);
    emit pixelHovered(x, y, c.red(), c.green(), c.blue());
    update();       /* 放大镜跟随光标 */
    QWidget::mouseMoveEvent(event);
}

void ImageView::leaveEvent(QEvent *event)
{
    m_cursorInside = false;
    update();
    QWidget::leaveEvent(event);
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        if (m_selRect.width() < 2 || m_selRect.height() < 2) {
            /* 微小位移视为单击取色 */
            m_selRect = QRect();
            emit pixelClicked(m_selStartImg.x(), m_selStartImg.y());
        } else {
            emit regionSelected(m_selRect);
        }
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_image.isNull()) {
        QPointF ip = toImageCoord(event->pos());
        int x = int(ip.x()), y = int(ip.y());
        if (x >= 0 && y >= 0 && x < m_image.width() && y < m_image.height()) {
            m_selecting = true;
            m_selStartImg = QPoint(x, y);
            m_selRect = QRect();
            update();
        }
    }
    QWidget::mousePressEvent(event);
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    double step = (event->angleDelta().y() > 0) ? 1.25 : 0.8;
    /* 缩放锚点归 MainWindow（拥有滚动区）：请求信号携带光标位置 */
    emit zoomRequested(step, event->position().toPoint());
    event->accept();
}

QSize ImageView::sizeHint() const
{
    if (m_image.isNull())
        return QSize(320, 240);
    return QSize(int(m_image.width() * m_zoom), int(m_image.height() * m_zoom));
}
