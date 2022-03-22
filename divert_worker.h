#ifndef DIVERTWORKER_H
#define DIVERTWORKER_H

#include "config.h"
#include "variable.h"

#include <WinDivert/windivert.h>
#include <QObject>
#include <list>
#include <QAtomicInt>
#include <thread>
#include <QMutex>
#include <QWaitCondition>

#pragma comment(lib,"winmm.lib")

enum DivertState
{
    start,
    end
};

enum ResultCode
{
    error,
    success
};

class DivertWorker : public QObject
{
    Q_OBJECT

private:
    struct PacketNode
    {
        char *packet;
        UINT packetLen;
        WINDIVERT_ADDRESS addr;
        DWORD lagExpiredTimestamp;

        PacketNode()
        {
            packet = nullptr;
            packetLen = 0;
            memset(&addr, 0, sizeof(addr));
            lagExpiredTimestamp = timeGetTime();
        }

        PacketNode(char *__packet, UINT __packetLen, const WINDIVERT_ADDRESS __addr)
        {
            packet = new char[__packetLen + 1];
            memcpy(packet, __packet, __packetLen);
            packet[__packetLen] = '\0';
            packetLen = __packetLen;
            addr = __addr;
            lagExpiredTimestamp = timeGetTime();
        }

        PacketNode(const PacketNode &node)
        {
            packet = new char[node.packetLen + 1];
            memcpy(packet, node.packet, node.packetLen);
            packet[node.packetLen] = '\0';
            packetLen = node.packetLen;
            addr = node.addr;
            lagExpiredTimestamp = timeGetTime();
        }

        ~PacketNode()
        {
            delete packet;
        }
    };

    enum Direction
    {
        inbound,
        outbound
    };

public:
    explicit DivertWorker(QObject *parent = nullptr);

public slots:
    void startStateChangedSlot(DivertState state, const QString &filter);

signals:
    void resultFeedback(ResultCode code, const QString &info);

private:
    bool calcChance(int chance);
    void divertReadLoop();
    void divertWriteLoop();
    void sendAllListPackets(std::list<PacketNode> &packetsList);
    void divertProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle, Direction dir);
    void lagProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle, Direction dir);
    void dropProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle);
    void throttleProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle, Direction dir);
    void duplicateProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle);
    void outoforderProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle);
    void tamperProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle);
    inline void tamperBuf(char *buf, UINT len)
    {
        UINT ix;
        for (ix = 0; ix < len; ++ix)
        {
            buf[ix] ^= patterns[patIx++ & 0x7];
        }
    }

public:
    static VariableBundle inboundVariables;
    static VariableBundle outboundVariables;

private:
    QMutex m_listMutex;
    QWaitCondition m_listCondition;
    std::list<PacketNode> m_inboundPacketsList;
    std::list<PacketNode> m_outboundPacketsList;

    QAtomicInt m_startEnabled;

    std::thread m_readThread;
    std::thread m_writeThread;

    HANDLE m_divertHandle;
    static char patterns[8];
    static int patIx;
};

#endif // DIVERTWORKER_H
