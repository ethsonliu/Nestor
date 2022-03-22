#ifndef LINEEDITPLUS_H
#define LINEEDITPLUS_H

#include "config.h"

#include <QWidget>
#include <QString>
#include <QSize>
#include <QAtomicInt>

class QLineEdit;
class QLabel;
class Variable;

class LineEditPlus : public QWidget
{
    Q_OBJECT
public:
    LineEditPlus(const QString &name, const QString &contents, Variable *value = nullptr, QWidget *parent = nullptr);
    QString text();
    void setChangeable(bool changeEnabled);
    void setInputRegular(const QString &reg);

private slots:
    void returnPressedSlot();
    void textEditedSlot(const QString &text);

private:
    QLabel *m_label;
    QLineEdit *m_lineEdit;
    Variable *m_syncedValue;
    bool m_isChangeable;
};

#endif // LINEEDITPLUS_H
