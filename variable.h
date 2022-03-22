#ifndef VARIABLE_H
#define VARIABLE_H

#include "config.h"

#include <QString>
#include <QAtomicInt>
#include <QStringList>
#include <QRandomGenerator>
#include <QMutexLocker>
#include <QMutex>

class Variable
{
public:
    Variable() { m_lowest = m_highest = 0; }

    void update(const QString &str)
    {
        QMutexLocker locker(&m_mutex);

        // reset first
        m_lowest = m_highest = 0;

        QStringList strList = str.split(PLUS_OR_MINUS);
        m_lowest = strList[0].toInt();
        if (strList.size() > 1)
            m_highest = strList[1].toInt();

        // calculate the range
        int low = m_lowest - m_highest;
        int high = m_lowest + m_highest;
        m_lowest = low < 0 ? 0 : low;
        m_highest = high;
    }

    int rand()
    {
        QMutexLocker locker(&m_mutex);
        return QRandomGenerator::global()->bounded(m_lowest, m_highest + 1);
    }

private:
    QMutex m_mutex;
    int m_lowest;
    int m_highest;
};

class VariableBundle
{
public:
    VariableBundle()
    {
        lagEnabled = 0;
        dropEnabled = 0;
        throttleEnabled = 0;
        outoforderEnabled = 0;
        duplicateEnabled = 0;
        tamperEnabled  = 0;
        tamperRedoChecksum = 0;
    }

    QAtomicInt lagEnabled;
    QAtomicInt dropEnabled;
    QAtomicInt throttleEnabled;
    QAtomicInt outoforderEnabled;
    QAtomicInt duplicateEnabled;
    QAtomicInt tamperEnabled;
    QAtomicInt tamperRedoChecksum;

    Variable lagTime;
    Variable dropChance;
    Variable throttleTimeFrame;
    Variable throttleChance;
    Variable outoforderChance;
    Variable duplicateCount;
    Variable duplicateChance;
    Variable tamperChance;
};

#endif // VARIABLE_H
