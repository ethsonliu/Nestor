#include "line_edit_plus.h"
#include "variable.h"

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QRegExpValidator>

LineEditPlus::LineEditPlus(const QString &name, const QString &contents, Variable *value, QWidget *parent) : QWidget{parent},
    m_syncedValue{value}, m_isChangeable{true}
{
    m_label = new QLabel(name);
    m_lineEdit = new QLineEdit(contents);
    m_lineEdit->setMaximumWidth(80);

    connect(m_lineEdit, &QLineEdit::returnPressed, this, &LineEditPlus::returnPressedSlot);
    connect(m_lineEdit, &QLineEdit::textEdited, this, &LineEditPlus::textEditedSlot);

    textEditedSlot(contents);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addStretch();
    mainLayout->addWidget(m_label);
    mainLayout->addWidget(m_lineEdit);
    mainLayout->setSpacing(SPACING_OF_LABEL_AND_LINEEDIT);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    setLayout(mainLayout);
}

QString LineEditPlus::text()
{
    return m_lineEdit->text();
}

void LineEditPlus::setChangeable(bool changeEnabled)
{
    m_isChangeable = changeEnabled;
}

void LineEditPlus::setInputRegular(const QString &reg)
{
    QRegExp regx(reg);
    QValidator *validator = new QRegExpValidator(regx, m_lineEdit);
    m_lineEdit->setValidator(validator);
}

void LineEditPlus::returnPressedSlot()
{
    if (!m_lineEdit->text().isEmpty() && m_lineEdit->text().lastIndexOf(PLUS_OR_MINUS) == -1 && m_isChangeable)
    {
        m_lineEdit->setText(m_lineEdit->text() + PLUS_OR_MINUS);
    }
}

void LineEditPlus::textEditedSlot(const QString &text)
{
    if (m_syncedValue)
        m_syncedValue->update(text);
}
