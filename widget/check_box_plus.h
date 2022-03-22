#ifndef CHECKBOXPLUS_H
#define CHECKBOXPLUS_H

#include <QCheckBox>
#include <QString>

class QAtomicInt;

class CheckBoxPlus : public QCheckBox
{
    Q_OBJECT
public:
    explicit CheckBoxPlus(const QString &text, QAtomicInt *value = nullptr);

private slots:
    void clickedSlot(bool checked);

private:
    QAtomicInt *m_syncedValue;;
};

#endif // CHECKBOXPLUS_H
