/* RawFormatDialog.cpp - RAW 打开参数对话框实现 */
#include "RawFormatDialog.h"
#include "core/raw_loader.h"
#include <QComboBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>

RawFormatDialog::RawFormatDialog(ItRawFormat &fmt, QWidget *parent)
    : QDialog(parent), m_fmt(fmt)
{
    setWindowTitle(tr("Raw Format"));
    setMinimumWidth(300);

    QVBoxLayout *v = new QVBoxLayout(this);
    QFormLayout *f = new QFormLayout;

    m_bits = new QComboBox;
    m_bits->addItem("10 bit", int(IT_BITS_10));
    m_bits->addItem("16 bit", int(IT_BITS_16));
    m_bits->addItem("8 bit",  int(IT_BITS_8));

    m_pattern = new QComboBox;
    m_pattern->addItem("RGGB", int(IT_PAT_RGGB));
    m_pattern->addItem("GRBG", int(IT_PAT_GRBG));
    m_pattern->addItem("GBRG", int(IT_PAT_GBRG));
    m_pattern->addItem("BGGR", int(IT_PAT_BGGR));

    m_width  = new QSpinBox; m_width->setRange(1, 16384);
    m_height = new QSpinBox; m_height->setRange(1, 16384);
    m_dummyRows = new QSpinBox; m_dummyRows->setRange(0, 256);
    m_dummyCols = new QSpinBox; m_dummyCols->setRange(0, 256);

    /* 已知传感器尺寸（对应原程序 .data 内置表）*/
    m_knownSizes = new QComboBox;
    int kw[32], kh[32];
    int nKnown = it_raw_known_sizes(kw, kh, 32);
    m_knownSizes->addItem(tr("custom"), -1);
    for (int i = 0; i < nKnown; i++)
        m_knownSizes->addItem(QString("%1 x %2").arg(kw[i]).arg(kh[i]), i);
    connect(m_knownSizes, &QComboBox::currentIndexChanged, this,
            [this](int idx) {
                int i = m_knownSizes->itemData(idx).toInt();
                if (i < 0) return;
                int kw2[32], kh2[32];
                it_raw_known_sizes(kw2, kh2, 32);
                m_width->setValue(kw2[i]);
                m_height->setValue(kh2[i]);
            });

    f->addRow(tr("known size"), m_knownSizes);
    f->addRow(tr("bit resolution"), m_bits);
    f->addRow(tr("Bayer Pattern"),  m_pattern);
    f->addRow(tr("H size"),         m_width);
    f->addRow(tr("V size"),         m_height);
    f->addRow(tr("dummy rows (skip)"), m_dummyRows);
    f->addRow(tr("dummy cols (skip)"), m_dummyCols);

    QGroupBox *gb = new QGroupBox(tr("Byte order (10/16 bit)"));
    QHBoxLayout *hb = new QHBoxLayout(gb);
    m_lh = new QRadioButton("LH");
    m_hl = new QRadioButton("HL");
    hb->addWidget(m_lh);
    hb->addWidget(m_hl);

    v->addLayout(f);
    v->addWidget(gb);

    QDialogButtonBox *bb = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    v->addWidget(bb);

    int idxBits = m_bits->findData(int(m_fmt.bits));
    m_bits->setCurrentIndex(idxBits < 0 ? 0 : idxBits);
    int idxPat = m_pattern->findData(int(m_fmt.pattern));
    m_pattern->setCurrentIndex(idxPat < 0 ? 0 : idxPat);
    m_width->setValue(m_fmt.width  > 0 ? m_fmt.width  : 640);
    m_height->setValue(m_fmt.height > 0 ? m_fmt.height : 480);
    m_dummyRows->setValue(m_fmt.dummy_rows);
    m_dummyCols->setValue(m_fmt.dummy_cols);
    (m_fmt.order == IT_BYTEORDER_HL ? m_hl : m_lh)->setChecked(true);
}

void RawFormatDialog::applyToFormat()
{
    m_fmt.bits    = ItBitDepth(m_bits->currentData().toInt());
    m_fmt.pattern = ItBayerPattern(m_pattern->currentData().toInt());
    m_fmt.width   = m_width->value();
    m_fmt.height  = m_height->value();
    m_fmt.order   = m_hl->isChecked() ? IT_BYTEORDER_HL : IT_BYTEORDER_LH;
    /* dummy 裁剪不得吃掉全部有效行列（换文件时旧值可能超界）*/
    m_fmt.dummy_rows = qBound(0, m_dummyRows->value(), m_fmt.height - 1);
    m_fmt.dummy_cols = qBound(0, m_dummyCols->value(), m_fmt.width - 1);
}
