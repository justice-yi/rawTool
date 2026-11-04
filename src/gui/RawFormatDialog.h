/* RawFormatDialog.h - RAW 打开参数（复刻 DIALOG 182 "Raw Format"：
 * 位深 8~16 按钮组、H size / V size、HL/LH 字节序、Bayer 排列）*/
#ifndef RAWFORMATDIALOG_H
#define RAWFORMATDIALOG_H

#include <QDialog>
#include "core/itypes.h"

class QSpinBox;
class QComboBox;
class QRadioButton;
class QDialogButtonBox;

class RawFormatDialog : public QDialog {
    Q_OBJECT
public:
    explicit RawFormatDialog(ItRawFormat &fmt, QWidget *parent = nullptr);
    void applyToFormat();

private:
    ItRawFormat &m_fmt;
    QComboBox   *m_bits;
    QComboBox   *m_pattern;
    QComboBox   *m_knownSizes;
    QRadioButton *m_lh, *m_hl;
    QSpinBox    *m_width, *m_height;
    QSpinBox    *m_dummyRows, *m_dummyCols;
};

#endif /* RAWFORMATDIALOG_H */
