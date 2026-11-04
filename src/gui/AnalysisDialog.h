/* AnalysisDialog.h - 统计分析页（复刻 DIALOG 178 "Analysis"：
 * Y/R/G/B Mean/Std/SNR 表 + 直方图 + Save Summary）*/
#ifndef ANALYSISDIALOG_H
#define ANALYSISDIALOG_H

#include <QDialog>
#include <QImage>
#include <QTabWidget>
#include "core/itypes.h"

class QLabel;
class QPushButton;
class QTableWidget;

class AnalysisDialog : public QDialog {
    Q_OBJECT
public:
    explicit AnalysisDialog(QWidget *parent = nullptr);
    /* 每次图像变更后调用；roi 非空时仅统计该区域（原 DIALOG 178 区域 SNR）。
     * raw 非空时同时绘制 16bit 原始直方图（bits 为其位深）。 */
    void analyze(const QImage &img, const QString &name, const QRect &roi,
                 const ItImage *raw = nullptr, int rawBits = 10);

private slots:
    void saveSummary();
    void exportCsv();

private:
    void drawHistogram();
    void drawRawHistogram(const ItImage *raw, int bits);

    QImage        m_img;
    QString       m_name;
    QRect         m_roi;
    ItStat        m_stat;
    QLabel       *m_histLabel;
    QLabel       *m_rawHistLabel;
    QTabWidget   *m_histTabs;
    QTableWidget *m_table;
    QPushButton  *m_saveBtn;
    QLabel       *m_roiLabel;
    QPixmap       m_histPixmap;
    QPixmap       m_rawHistPixmap;
};

#endif /* ANALYSISDIALOG_H */
