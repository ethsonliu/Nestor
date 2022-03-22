#include "check_box_plus.h"

CheckBoxPlus::CheckBoxPlus(const QString &text, QAtomicInt *value) : QCheckBox{text},
    m_syncedValue{value}
{
    connect(this, &CheckBoxPlus::clicked, this, &CheckBoxPlus::clickedSlot);
    setContentsMargins(0, 0, 0, 0);
}

void CheckBoxPlus::clickedSlot(bool checked)
{
    Q_UNUSED(checked);

    if (m_syncedValue)
        *m_syncedValue = isChecked();
}
