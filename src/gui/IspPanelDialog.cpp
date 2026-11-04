/* IspPanelDialog.cpp - ISP 参数面板实现（控件名与原对话框控件文本对应）*/
#include "IspPanelDialog.h"
#include "core/it_persist.h"
#include <QTabWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialogButtonBox>

IspPanelDialog::IspPanelDialog(ItConfig &cfg, QWidget *parent)
    : QDialog(parent), m_cfg(cfg)
{
    setWindowTitle(tr("ISP Parameters (ISP_P1)"));
    setMinimumWidth(420);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->addWidget(makeTabs());

    /* 调试转储区（对应原 0x40d110 各阶段使能位 + 输出目录）*/
    QGroupBox *gb = new QGroupBox(tr("Stage Debug Dump"));
    QVBoxLayout *vd = new QVBoxLayout(gb);
    QHBoxLayout *hd = new QHBoxLayout;
    m_ckDump = new QCheckBox(tr("Enable"));
    m_ckPpm  = new QCheckBox(tr("PPM (ASCII)"));
    m_dumpDir = new QLineEdit(m_cfg.dump_dir);
    QPushButton *br = new QPushButton(tr("Browse..."));
    connect(br, &QPushButton::clicked, this, &IspPanelDialog::onDumpDirBrowse);
    hd->addWidget(m_ckDump);
    hd->addWidget(m_ckPpm);
    hd->addWidget(m_dumpDir, 1);
    hd->addWidget(br);
    vd->addLayout(hd);
    QHBoxLayout *hs = new QHBoxLayout;
    const char *stageNames[5] = { "00_dclamp", "01_wb", "02_shade",
                                  "03_bpr", "04_grgb" };
    for (int i = 0; i < 5; i++) {
        m_ckStage[i] = new QCheckBox(tr(stageNames[i]));
        m_ckStage[i]->setChecked(true);
        hs->addWidget(m_ckStage[i]);
    }
    hs->addStretch(1);
    vd->addLayout(hs);
    main->addWidget(gb);

    QDialogButtonBox *bb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    main->addWidget(bb);

    /* INI 持久化（对应原程序 ISP_P1 节读写）*/
    QPushButton *btnLoad = new QPushButton(tr("Load INI..."));
    QPushButton *btnSave = new QPushButton(tr("Save INI..."));
    connect(btnLoad, &QPushButton::clicked, this, &IspPanelDialog::onLoadIni);
    connect(btnSave, &QPushButton::clicked, this, &IspPanelDialog::onSaveIni);
    QHBoxLayout *iniRow = new QHBoxLayout;
    iniRow->addWidget(btnLoad);
    iniRow->addWidget(btnSave);
    iniRow->addStretch(1);
    main->addLayout(iniRow);
}

QTabWidget *IspPanelDialog::makeTabs()
{
    QTabWidget *tabs = new QTabWidget;
    tabs->addTab(makeClampTab(),   tr("Digital Clamp"));
    tabs->addTab(makeWbTab(),      tr("White Balance"));
    tabs->addTab(makeShadingTab(), tr("Shading"));
    tabs->addTab(makeBprGrgbTab(), tr("BPR / GRGB"));
    tabs->addTab(makeRgbTab(),     tr("RGB Domain"));
    return tabs;
}

static QFormLayout *addForm(QVBoxLayout *outer)
{
    QFormLayout *f = new QFormLayout;
    outer->addLayout(f);
    outer->addStretch(1);
    return f;
}

QWidget *IspPanelDialog::makeClampTab()
{
    QWidget *w = new QWidget;
    QVBoxLayout *v = new QVBoxLayout(w);
    QFormLayout *f = addForm(v);

    m_dclpStrtH = new QSpinBox; m_dclpStrtH->setRange(0, 4096);
    m_dclpStrtV = new QSpinBox; m_dclpStrtV->setRange(0, 4096);
    m_poffset   = new QSpinBox; m_poffset->setRange(0, 1023);
    m_cisGr = new QSpinBox; m_cisGr->setRange(1, 1023);
    m_cisGb = new QSpinBox; m_cisGb->setRange(1, 1023);
    m_cisR  = new QSpinBox; m_cisR->setRange(1, 1023);
    m_cisB  = new QSpinBox; m_cisB->setRange(1, 1023);
    m_offGr = new QSpinBox; m_offGr->setRange(0, 1023);
    m_offGb = new QSpinBox; m_offGb->setRange(0, 1023);
    m_offR  = new QSpinBox; m_offR->setRange(0, 1023);
    m_offB  = new QSpinBox; m_offB->setRange(0, 1023);

    f->addRow(tr("DCLP_STRT_H"), m_dclpStrtH);
    f->addRow(tr("DCLP_STRT_V"), m_dclpStrtV);
    f->addRow(tr("POFFSET"),     m_poffset);
    f->addRow(tr("CISGAIN_GR"),  m_cisGr);
    f->addRow(tr("CISGAIN_GB"),  m_cisGb);
    f->addRow(tr("CISGAIN_R"),   m_cisR);
    f->addRow(tr("CISGAIN_B"),   m_cisB);
    f->addRow(tr("POFFSET_GR"),  m_offGr);
    f->addRow(tr("POFFSET_GB"),  m_offGb);
    f->addRow(tr("POFFSET_R"),   m_offR);
    f->addRow(tr("POFFSET_B"),   m_offB);

    m_dclpStrtH->setValue(m_cfg.dclp_strt_h);
    m_dclpStrtV->setValue(m_cfg.dclp_strt_v);
    m_poffset->setValue(m_cfg.poffset);
    m_cisGr->setValue(m_cfg.cisgain_gr);
    m_cisGb->setValue(m_cfg.cisgain_gb);
    m_cisR->setValue(m_cfg.cisgain_r);
    m_cisB->setValue(m_cfg.cisgain_b);
    m_offGr->setValue(m_cfg.poffset_gr);
    m_offGb->setValue(m_cfg.poffset_gb);
    m_offR->setValue(m_cfg.poffset_r);
    m_offB->setValue(m_cfg.poffset_b);
    return w;
}

QWidget *IspPanelDialog::makeWbTab()
{
    QWidget *w = new QWidget;
    QVBoxLayout *v = new QVBoxLayout(w);
    QFormLayout *f = addForm(v);

    m_wbR = new QSpinBox; m_wbR->setRange(1, 1023);
    m_wbG = new QSpinBox; m_wbG->setRange(1, 1023);
    m_wbB = new QSpinBox; m_wbB->setRange(1, 1023);
    m_dsR = new QSpinBox; m_dsR->setRange(0, 1023);
    m_dsG = new QSpinBox; m_dsG->setRange(0, 1023);
    m_dsB = new QSpinBox; m_dsB->setRange(0, 1023);

    f->addRow(tr("CRWB (R gain)"), m_wbR);
    f->addRow(tr("CGWB (G gain)"), m_wbG);
    f->addRow(tr("CBWB (B gain)"), m_wbB);
    f->addRow(tr("CRDS (R dark)"), m_dsR);
    f->addRow(tr("CGDS (G dark)"), m_dsG);
    f->addRow(tr("CBDS (B dark)"), m_dsB);

    m_wbR->setValue(m_cfg.wb_gain_r);
    m_wbG->setValue(m_cfg.wb_gain_g);
    m_wbB->setValue(m_cfg.wb_gain_b);
    m_dsR->setValue(m_cfg.wb_ds_r);
    m_dsG->setValue(m_cfg.wb_ds_g);
    m_dsB->setValue(m_cfg.wb_ds_b);
    return w;
}

QWidget *IspPanelDialog::makeShadingTab()
{
    QWidget *w = new QWidget;
    QVBoxLayout *v = new QVBoxLayout(w);
    QFormLayout *f = addForm(v);

    m_shadeOn     = new QCheckBox(tr("SHADE_ON"));
    m_shadeRadial = new QCheckBox(tr("SHADE_RADIAL (else 3x3 subwin)"));
    m_shadeXc = new QSpinBox; m_shadeXc->setRange(0, 8192);
    m_shadeYc = new QSpinBox; m_shadeYc->setRange(0, 8192);
    m_shCenter = new QSpinBox; m_shCenter->setRange(-255, 255);
    m_shCorner = new QSpinBox; m_shCorner->setRange(-255, 255);
    m_gainH = new QSpinBox; m_gainH->setRange(-128, 127);
    m_gainV = new QSpinBox; m_gainV->setRange(-128, 127);

    f->addRow(m_shadeOn);
    f->addRow(m_shadeRadial);
    f->addRow(tr("SHADE_XC"), m_shadeXc);
    f->addRow(tr("SHADE_YC"), m_shadeYc);
    f->addRow(tr("SH_VAL center"), m_shCenter);
    f->addRow(tr("SH_VAL corner"), m_shCorner);
    f->addRow(tr("GAIN_H (all columns)"), m_gainH);
    f->addRow(tr("GAIN_V (all rows)"), m_gainV);

    m_shadeOn->setChecked(m_cfg.shade_on != 0);
    m_shadeRadial->setChecked(m_cfg.shade_radial != 0);
    m_shadeXc->setValue(m_cfg.shade_xc);
    m_shadeYc->setValue(m_cfg.shade_yc);
    m_shCenter->setValue(m_cfg.sh_val_r[0]);
    m_shCorner->setValue(m_cfg.sh_val_r[1]);
    m_gainH->setValue(m_cfg.gain_h[0]);
    m_gainV->setValue(m_cfg.gain_v[0]);
    return w;
}

QWidget *IspPanelDialog::makeBprGrgbTab()
{
    QWidget *w = new QWidget;
    QVBoxLayout *v = new QVBoxLayout(w);
    QGroupBox *gb = new QGroupBox(tr("Bad Pixel Removal (BPR)"));
    QFormLayout *f = new QFormLayout(gb);

    m_bprOn  = new QCheckBox(tr("BPR_ON"));
    m_bprTh1 = new QSpinBox; m_bprTh1->setRange(0, 1023);
    m_bprTh3 = new QSpinBox; m_bprTh3->setRange(0, 1023);
    m_bprTh5 = new QSpinBox; m_bprTh5->setRange(0, 1023);
    m_bprTh7 = new QSpinBox; m_bprTh7->setRange(0, 1023);
    f->addRow(m_bprOn);
    f->addRow(tr("BPR_TH1 (3x3 window)"), m_bprTh1);
    f->addRow(tr("BPR_TH3 (7x7 window)"), m_bprTh3);
    f->addRow(tr("BPR_TH5 (11x11 window)"), m_bprTh5);
    f->addRow(tr("BPR_TH7 (15x15 window)"), m_bprTh7);

    QGroupBox *gg = new QGroupBox(tr("Green Imbalance (GRGB)"));
    QFormLayout *fg = new QFormLayout(gg);
    m_grgbOn  = new QCheckBox(tr("GRGB_ON"));
    m_grgbGrb = new QCheckBox(tr("GRGB_GRB (correction direction)"));
    m_grgbAvg = new QCheckBox(tr("GRGB_AVG"));
    m_gthr01  = new QSpinBox; m_gthr01->setRange(0, 1023);
    fg->addRow(m_grgbOn);
    fg->addRow(m_grgbGrb);
    fg->addRow(m_grgbAvg);
    fg->addRow(tr("GTHR01"), m_gthr01);

    m_bprOn->setChecked(m_cfg.bpr_on != 0);
    m_bprTh1->setValue(m_cfg.bpr_th1);
    m_bprTh3->setValue(m_cfg.bpr_th3);
    m_bprTh5->setValue(m_cfg.bpr_th5);
    m_bprTh7->setValue(m_cfg.bpr_th7);
    m_grgbOn->setChecked(m_cfg.grgb_on != 0);
    m_grgbGrb->setChecked(m_cfg.grgb_grb != 0);
    m_grgbAvg->setChecked(m_cfg.grgb_avg != 0);
    m_gthr01->setValue(m_cfg.gthr01);

    v->addWidget(gb);
    v->addWidget(gg);
    v->addStretch(1);
    return w;
}

QWidget *IspPanelDialog::makeRgbTab()
{
    QWidget *w = new QWidget;
    QVBoxLayout *v = new QVBoxLayout(w);
    QFormLayout *f = addForm(v);

    QGroupBox *gp = new QGroupBox(tr("RGB Prefilter (rgb_prefilter stage)"));
    QFormLayout *fp = new QFormLayout(gp);
    m_filterOn   = new QCheckBox(tr("RGB_FILTER_ON"));
    m_gmedianOn  = new QCheckBox(tr("RGB_GMEDIAN_ON (green 3x3 median)"));
    m_sig01Const = new QSpinBox; m_sig01Const->setRange(0, 4095);
    m_sig01Mult  = new QSpinBox; m_sig01Mult->setRange(0, 1023);
    m_sig02Mult  = new QSpinBox; m_sig02Mult->setRange(0, 256);
    fp->addRow(m_filterOn);
    fp->addRow(m_gmedianOn);
    fp->addRow(tr("SIG01_CONST"), m_sig01Const);
    fp->addRow(tr("SIG01_MULT"),  m_sig01Mult);
    fp->addRow(tr("SIG02_MULT"),  m_sig02Mult);
    v->addWidget(gp);

    m_gain2R = new QSpinBox; m_gain2R->setRange(1, 1023);
    m_gain2G = new QSpinBox; m_gain2G->setRange(1, 1023);
    m_gain2B = new QSpinBox; m_gain2B->setRange(1, 1023);
    m_off2R = new QSpinBox; m_off2R->setRange(-255, 255);
    m_off2G = new QSpinBox; m_off2G->setRange(-255, 255);
    m_off2B = new QSpinBox; m_off2B->setRange(-255, 255);
    m_gamma = new QDoubleSpinBox; m_gamma->setRange(0.1, 5.0);
    m_gamma->setSingleStep(0.1);
    m_ry = new QSpinBox; m_ry->setRange(0, 512);
    m_gy = new QSpinBox; m_gy->setRange(0, 512);
    m_by = new QSpinBox; m_by->setRange(0, 512);

    f->addRow(tr("gain_r_2nd"), m_gain2R);
    f->addRow(tr("gain_g_2nd"), m_gain2G);
    f->addRow(tr("gain_b_2nd"), m_gain2B);
    f->addRow(tr("offset_r_2nd"), m_off2R);
    f->addRow(tr("offset_g_2nd"), m_off2G);
    f->addRow(tr("offset_b_2nd"), m_off2B);
    f->addRow(tr("Gamma value"), m_gamma);
    f->addRow(tr("RY_COEF"), m_ry);
    f->addRow(tr("GY_COEF"), m_gy);
    f->addRow(tr("BY_COEF"), m_by);

    QGroupBox *gm = new QGroupBox(tr("Colour Correction Matrix (RGM10..RGM32)"));
    QGridLayout *gl = new QGridLayout(gm);
    for (int i = 0; i < 9; i++) {
        m_mat[i] = new QLineEdit;
        m_mat[i]->setMaximumWidth(70);
        gl->addWidget(m_mat[i], i / 3, i % 3);
    }
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            m_mat[r * 3 + c]->setText(QString::number(m_cfg.rgm[r][c]));

    m_filterOn->setChecked(m_cfg.rgb_filter_on != 0);
    m_gmedianOn->setChecked(m_cfg.rgb_gmedian_on != 0);
    m_sig01Const->setValue(m_cfg.sig01_const);
    m_sig01Mult->setValue(m_cfg.sig01_mult);
    m_sig02Mult->setValue(m_cfg.sig02_mult);

    m_gain2R->setValue(m_cfg.gain_r_2nd);
    m_gain2G->setValue(m_cfg.gain_g_2nd);
    m_gain2B->setValue(m_cfg.gain_b_2nd);
    m_off2R->setValue(m_cfg.offset_r_2nd);
    m_off2G->setValue(m_cfg.offset_g_2nd);
    m_off2B->setValue(m_cfg.offset_b_2nd);
    m_gamma->setValue(m_cfg.gamma_value);
    m_ry->setValue(m_cfg.ry_coef);
    m_gy->setValue(m_cfg.gy_coef);
    m_by->setValue(m_cfg.by_coef);

    v->addWidget(gm);
    return w;
}

void IspPanelDialog::onDumpDirBrowse()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Dump directory"), m_dumpDir->text());
    if (!dir.isEmpty())
        m_dumpDir->setText(dir);
}

/* 把当前控件值写回 cfg，执行 IO，再把 cfg 刷回控件 */
void IspPanelDialog::onSaveIni()
{
    applyToConfig();
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save INI"), "isp_p1.ini",
        tr("INI files (*.ini);;All files (*.*)"));
    if (path.isEmpty())
        return;
    ItStatus st = it_config_save_ini(&m_cfg, path.toLocal8Bit().constData());
    if (st != IT_OK)
        QMessageBox::warning(this, tr("Save INI"), it_status_str(st));
}

void IspPanelDialog::onLoadIni()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Load INI"), QString(),
        tr("INI files (*.ini);;All files (*.*)"));
    if (path.isEmpty())
        return;
    ItStatus st = it_config_load_ini(path.toLocal8Bit().constData(),
                                     &m_cfg, nullptr);
    if (st != IT_OK) {
        QMessageBox::warning(this, tr("Load INI"), it_status_str(st));
        return;
    }
    refreshFromConfig();
}

void IspPanelDialog::refreshFromConfig()
{
    m_bprOn->setChecked(m_cfg.bpr_on != 0);
    m_bprTh1->setValue(m_cfg.bpr_th1);
    m_bprTh3->setValue(m_cfg.bpr_th3);
    m_bprTh5->setValue(m_cfg.bpr_th5);
    m_bprTh7->setValue(m_cfg.bpr_th7);
    m_grgbOn->setChecked(m_cfg.grgb_on != 0);
    m_grgbGrb->setChecked(m_cfg.grgb_grb != 0);
    m_grgbAvg->setChecked(m_cfg.grgb_avg != 0);
    m_gthr01->setValue(m_cfg.gthr01);

    m_filterOn->setChecked(m_cfg.rgb_filter_on != 0);
    m_gmedianOn->setChecked(m_cfg.rgb_gmedian_on != 0);
    m_sig01Const->setValue(m_cfg.sig01_const);
    m_sig01Mult->setValue(m_cfg.sig01_mult);
    m_sig02Mult->setValue(m_cfg.sig02_mult);

    m_shadeOn->setChecked(m_cfg.shade_on != 0);
    m_shadeRadial->setChecked(m_cfg.shade_radial != 0);
    m_gainH->setValue(m_cfg.gain_h[0]);
    m_gainV->setValue(m_cfg.gain_v[0]);

    m_dclpStrtH->setValue(m_cfg.dclp_strt_h);
    m_dclpStrtV->setValue(m_cfg.dclp_strt_v);
    m_poffset->setValue(m_cfg.poffset);
    m_cisGr->setValue(m_cfg.cisgain_gr);
    m_cisGb->setValue(m_cfg.cisgain_gb);
    m_cisR->setValue(m_cfg.cisgain_r);
    m_cisB->setValue(m_cfg.cisgain_b);
    m_offGr->setValue(m_cfg.poffset_gr);
    m_offGb->setValue(m_cfg.poffset_gb);
    m_offR->setValue(m_cfg.poffset_r);
    m_offB->setValue(m_cfg.poffset_b);

    m_wbR->setValue(m_cfg.wb_gain_r);
    m_wbG->setValue(m_cfg.wb_gain_g);
    m_wbB->setValue(m_cfg.wb_gain_b);
    m_dsR->setValue(m_cfg.wb_ds_r);
    m_dsG->setValue(m_cfg.wb_ds_g);
    m_dsB->setValue(m_cfg.wb_ds_b);

    m_shadeXc->setValue(m_cfg.shade_xc);
    m_shadeYc->setValue(m_cfg.shade_yc);
    m_shCenter->setValue(m_cfg.sh_val_r[0]);
    m_shCorner->setValue(m_cfg.sh_val_r[1]);

    m_gain2R->setValue(m_cfg.gain_r_2nd);
    m_gain2G->setValue(m_cfg.gain_g_2nd);
    m_gain2B->setValue(m_cfg.gain_b_2nd);
    m_off2R->setValue(m_cfg.offset_r_2nd);
    m_off2G->setValue(m_cfg.offset_g_2nd);
    m_off2B->setValue(m_cfg.offset_b_2nd);
    m_gamma->setValue(m_cfg.gamma_value);
    m_ry->setValue(m_cfg.ry_coef);
    m_gy->setValue(m_cfg.gy_coef);
    m_by->setValue(m_cfg.by_coef);
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            m_mat[r * 3 + c]->setText(QString::number(m_cfg.rgm[r][c]));
}

void IspPanelDialog::applyToConfig()
{
    m_cfg.bpr_on    = m_bprOn->isChecked() ? 1 : 0;
    m_cfg.bpr_th1   = m_bprTh1->value();
    m_cfg.bpr_th3   = m_bprTh3->value();
    m_cfg.bpr_th5   = m_bprTh5->value();
    m_cfg.bpr_th7   = m_bprTh7->value();
    m_cfg.grgb_on   = m_grgbOn->isChecked() ? 1 : 0;
    m_cfg.grgb_grb  = m_grgbGrb->isChecked() ? 1 : 0;
    m_cfg.grgb_avg  = m_grgbAvg->isChecked() ? 1 : 0;
    m_cfg.gthr01    = m_gthr01->value();

    m_cfg.dclp_strt_h = m_dclpStrtH->value();
    m_cfg.dclp_strt_v = m_dclpStrtV->value();
    m_cfg.poffset     = m_poffset->value();
    m_cfg.cisgain_gr  = m_cisGr->value();
    m_cfg.cisgain_gb  = m_cisGb->value();
    m_cfg.cisgain_r   = m_cisR->value();
    m_cfg.cisgain_b   = m_cisB->value();
    m_cfg.poffset_gr  = m_offGr->value();
    m_cfg.poffset_gb  = m_offGb->value();
    m_cfg.poffset_r   = m_offR->value();
    m_cfg.poffset_b   = m_offB->value();

    m_cfg.wb_gain_r = m_wbR->value();
    m_cfg.wb_gain_g = m_wbG->value();
    m_cfg.wb_gain_b = m_wbB->value();
    m_cfg.wb_ds_r   = m_dsR->value();
    m_cfg.wb_ds_g   = m_dsG->value();
    m_cfg.wb_ds_b   = m_dsB->value();

    m_cfg.shade_on     = m_shadeOn->isChecked() ? 1 : 0;
    m_cfg.shade_radial = m_shadeRadial->isChecked() ? 1 : 0;
    m_cfg.shade_xc    = m_shadeXc->value();
    m_cfg.shade_yc    = m_shadeYc->value();
    for (int i = 0; i < 9; i++) {
        m_cfg.gain_h[i] = m_gainH->value();
        m_cfg.gain_v[i] = m_gainV->value();
    }
    /* 中心/角点增益同用于三通道（GUI 单组输入）*/
    m_cfg.sh_val_r[0] = m_shCenter->value();
    m_cfg.sh_val_g[0] = m_shCenter->value();
    m_cfg.sh_val_b[0] = m_shCenter->value();
    m_cfg.sh_val_r[1] = m_shCorner->value();
    m_cfg.sh_val_g[1] = m_shCorner->value();
    m_cfg.sh_val_b[1] = m_shCorner->value();

    m_cfg.rgb_filter_on   = m_filterOn->isChecked() ? 1 : 0;
    m_cfg.rgb_gmedian_on  = m_gmedianOn->isChecked() ? 1 : 0;
    m_cfg.sig01_const     = m_sig01Const->value();
    m_cfg.sig01_mult      = m_sig01Mult->value();
    m_cfg.sig02_mult      = m_sig02Mult->value();

    m_cfg.gain_r_2nd    = m_gain2R->value();
    m_cfg.gain_g_2nd    = m_gain2G->value();
    m_cfg.gain_b_2nd    = m_gain2B->value();
    m_cfg.offset_r_2nd  = m_off2R->value();
    m_cfg.offset_g_2nd  = m_off2G->value();
    m_cfg.offset_b_2nd  = m_off2B->value();
    m_cfg.gamma_value   = m_gamma->value();
    m_cfg.gam_sel       = (m_cfg.gamma_value != 1.0) ? 1 : 0;
    m_cfg.ry_coef = m_ry->value();
    m_cfg.gy_coef = m_gy->value();
    m_cfg.by_coef = m_by->value();

    for (int i = 0; i < 9; i++)
        m_cfg.rgm[i / 3][i % 3] = m_mat[i]->text().toInt();

    if (m_ckDump->isChecked() && !m_dumpDir->text().isEmpty()) {
        snprintf(m_cfg.dump_dir, sizeof(m_cfg.dump_dir), "%s",
                 m_dumpDir->text().toLocal8Bit().constData());
        m_cfg.dump_ppm     = m_ckPpm->isChecked() ? 1 : 0;
        m_cfg.dump_dclamp  = m_ckStage[0]->isChecked() ? 1 : 0;
        m_cfg.dump_wb      = m_ckStage[1]->isChecked() ? 1 : 0;
        m_cfg.dump_shading = m_ckStage[2]->isChecked() ? 1 : 0;
        m_cfg.dump_bpr     = m_ckStage[3]->isChecked() ? 1 : 0;
        m_cfg.dump_grgb    = m_ckStage[4]->isChecked() ? 1 : 0;
    } else {
        m_cfg.dump_dir[0] = '\0';
    }
}
