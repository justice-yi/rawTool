/* IspPanelDialog.h - ISP 参数面板（对应原 DIALOG 176/181/184/103 的
 * Digital Clamp / White Balance / Y Generation / Matrix 参数页）*/
#ifndef ISPPANELDIALOG_H
#define ISPPANELDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include "core/itypes.h"

class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;

class IspPanelDialog : public QDialog {
    Q_OBJECT
public:
    explicit IspPanelDialog(ItConfig &cfg, QWidget *parent = nullptr);
    /* 把面板当前值写回 cfg 并返回（OK 时调用） */
    void applyToConfig();

private slots:
    void onDumpDirBrowse();
    void onLoadIni();
    void onSaveIni();

private:
    void refreshFromConfig();

    ItConfig &m_cfg;

    QTabWidget *makeTabs();
    QWidget   *makeClampTab();
    QWidget   *makeWbTab();
    QWidget   *makeShadingTab();
    QWidget   *makeBprGrgbTab();
    QWidget   *makeRgbTab();

    /* Digital Clamp（DIALOG 176）*/
    QSpinBox *m_dclpStrtH, *m_dclpStrtV, *m_poffset;
    QSpinBox *m_cisGr, *m_cisGb, *m_cisR, *m_cisB;
    QSpinBox *m_offGr, *m_offGb, *m_offR, *m_offB;

    /* White Balance（DIALOG 181）*/
    QSpinBox *m_wbR, *m_wbG, *m_wbB;
    QSpinBox *m_dsR, *m_dsG, *m_dsB;

    /* Shading */
    QCheckBox *m_shadeOn, *m_shadeRadial;
    QSpinBox *m_shadeXc, *m_shadeYc;
    QSpinBox *m_shCenter, *m_shCorner;
    QSpinBox *m_gainH, *m_gainV;   /* 分窗模式：统一填 GAIN_H1..9 / V1..9 */

    /* BPR / GRGB */
    QCheckBox *m_bprOn, *m_grgbOn, *m_grgbGrb, *m_grgbAvg;
    QSpinBox *m_bprTh1, *m_bprTh3, *m_bprTh5, *m_bprTh7, *m_gthr01;

    /* RGB 域 */
    QCheckBox *m_filterOn, *m_gmedianOn;
    QSpinBox *m_sig01Const, *m_sig01Mult, *m_sig02Mult;
    QSpinBox *m_gain2R, *m_gain2G, *m_gain2B;
    QSpinBox *m_off2R, *m_off2G, *m_off2B;
    QLineEdit *m_mat[9];
    QDoubleSpinBox *m_gamma;
    QSpinBox *m_ry, *m_gy, *m_by;

    QCheckBox *m_ckDump;      /* 总开关 */
    QCheckBox *m_ckPpm;       /* P2 ASCII / raw 格式 */
    QCheckBox *m_ckStage[5];  /* dclamp/wb/shade/bpr/grgb 各级使能 */
    QLineEdit *m_dumpDir;
};

#endif /* ISPPANELDIALOG_H */
