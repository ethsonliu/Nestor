#include "divert_worker.h"

#include <QMutexLocker>
#include <QRandomGenerator>

VariableBundle DivertWorker::inboundVariables;
VariableBundle DivertWorker::outboundVariables;
char DivertWorker::patterns[8] = {0x64,0x13,0x88,0x40,0x1F,0xA0,0xAA,0x55};
int DivertWorker::patIx = 0;

DivertWorker::DivertWorker(QObject *parent) : QObject{parent}
{
    m_startEnabled = 0;
}

void DivertWorker::startStateChangedSlot(DivertState state, const QString &filter)
{
    ResultCode code = ResultCode::success;
    QString info;
    if (state == DivertState::start)
    {
        m_startEnabled = 1;

        m_divertHandle = WinDivertOpen(filter.toStdString().c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);
        if (m_divertHandle == INVALID_HANDLE_VALUE)
        {
            code = ResultCode::error;
            DWORD lastError = GetLastError();
            if (lastError == ERROR_INVALID_PARAMETER)
                info = "Failed to start filtering : filter syntax error.";
            else
                info = QString("Failed to start filtering : failed to open device (code:%1). "
                             "Make sure you run clumsy as Administrator.").arg(lastError);

            emit resultFeedback(code, info);
            return;
        }

        WinDivertSetParam(m_divertHandle, WINDIVERT_PARAM_QUEUE_LENGTH, 2 << 10);
        WinDivertSetParam(m_divertHandle, WINDIVERT_PARAM_QUEUE_TIME, 2 << 9);

        m_readThread = std::thread(&DivertWorker::divertReadLoop, this);
        m_writeThread = std::thread(&DivertWorker::divertWriteLoop, this);

        info = "Start filtering successfully.";
        emit resultFeedback(code, info);
    }
    else
    {
        m_startEnabled = 0;

        // notify condition to exit if it can't read any packets
        m_listCondition.notify_one();

        if (m_readThread.joinable())
            m_readThread.join();
        if (m_writeThread.joinable())
            m_writeThread.join();

        WinDivertClose(m_divertHandle);
    }
}

bool DivertWorker::calcChance(int chance)
{
    if (QRandomGenerator::global()->bounded(100) < chance)
        return true;
    return false;
}

void DivertWorker::divertReadLoop()
{
    char packetBuf[MAX_PACKETSIZE];
    WINDIVERT_ADDRESS addrBuf[1];
    UINT addrLen = sizeof(addrBuf);
    UINT readLen;
    static DWORD lastTime = timeGetTime();
    static int inboundCnt = 0;
    static int outboundCnt = 0;

    while (m_startEnabled)
    {
        memset(packetBuf, 0, sizeof(packetBuf));

        if (!WinDivertRecvEx(m_divertHandle, packetBuf, MAX_PACKETSIZE, &readLen, 0, addrBuf, &addrLen, nullptr))
        {
            DWORD lastError = GetLastError();
            if (lastError == ERROR_INVALID_HANDLE || lastError == ERROR_OPERATION_ABORTED)
            {
                LOG_ERROR << "Handle died or operation aborted. Exit loop.";

                return;
            }

            LOG_ERROR << "Failed to recv a packet: " << GetLastError();

            continue;
        }

        if (readLen > MAX_PACKETSIZE)
        {
            LOG_ERROR << "Internal Error: DivertRecv truncated recv packet.";
        }

        if (addrBuf[0].Outbound)
        {
            outboundCnt++;
            QMutexLocker locker(&m_listMutex);
            m_outboundPacketsList.push_back(PacketNode(packetBuf, readLen, addrBuf[0]));
            m_listCondition.notify_one();
        }
        else
        {
            inboundCnt++;
            QMutexLocker locker(&m_listMutex);
            m_inboundPacketsList.push_back(PacketNode(packetBuf, readLen, addrBuf[0]));
            m_listCondition.notify_one();
        }

        DWORD curTime = timeGetTime();
        if (curTime - lastTime > 3000)
        {
            LOG_INFO << "capture inbound=" << inboundCnt << ", outbound=" << outboundCnt;

            inboundCnt = outboundCnt = 0;
            lastTime = curTime;
        }
    }
}

void DivertWorker::divertWriteLoop()
{
    std::list<PacketNode> tmpList;

    while (m_startEnabled)
    {
        {
            QMutexLocker locker(&m_listMutex);

            if (m_outboundPacketsList.empty() && m_inboundPacketsList.empty())
                m_listCondition.wait(&m_listMutex);

            if (!m_startEnabled)
                return;

            if (!m_outboundPacketsList.empty())
            {
                divertProcess(m_outboundPacketsList, outboundVariables, Direction::outbound);
                tmpList.splice(tmpList.end(), m_outboundPacketsList);
            }

            if (!m_inboundPacketsList.empty())
            {
                divertProcess(m_inboundPacketsList, inboundVariables, Direction::inbound);
                tmpList.splice(tmpList.end(), m_inboundPacketsList);
            }
        }

        if (!tmpList.empty())
            sendAllListPackets(tmpList);
    }
}

void DivertWorker::sendAllListPackets(std::list<PacketNode> &packetsList)
{
    UINT sendLen;
    static DWORD lastTime = timeGetTime();
    static int sendCnt = 0;

    while (!packetsList.empty())
    {
        std::list<PacketNode>::iterator it = packetsList.begin();
        sendLen = 0;

        if (!WinDivertSend(m_divertHandle, it->packet, it->packetLen, &sendLen, &(it->addr)))
        {
            LOG_ERROR << "Failed to send a packet: ", GetLastError();

            PWINDIVERT_ICMPHDR icmp_header;
            PWINDIVERT_ICMPV6HDR icmpv6_header;
            PWINDIVERT_IPHDR ip_header;
            PWINDIVERT_IPV6HDR ipv6_header;
            WinDivertHelperParsePacket(it->packet, it->packetLen, &ip_header, &ipv6_header, NULL,
                                       &icmp_header, &icmpv6_header, NULL, NULL, NULL, NULL, NULL, NULL);
            if ((icmp_header || icmpv6_header) && !it->addr.Outbound)
            {
                BOOL resent;
                it->addr.Outbound = TRUE;
                if (ip_header)
                {
                    UINT32 tmp = ip_header->SrcAddr;
                    ip_header->SrcAddr = ip_header->DstAddr;
                    ip_header->DstAddr = tmp;
                }
                else if (ipv6_header)
                {
                    UINT32 tmpArr[4];
                    memcpy(tmpArr, ipv6_header->SrcAddr, sizeof(tmpArr));
                    memcpy(ipv6_header->SrcAddr, ipv6_header->DstAddr, sizeof(tmpArr));
                    memcpy(ipv6_header->DstAddr, tmpArr, sizeof(tmpArr));
                }

                resent = WinDivertSend(m_divertHandle, it->packet, it->packetLen, &sendLen, &(it->addr));
                LOG_ERROR << "Resend failed inbound ICMP packets as outbound: " << resent;

            }
        }
        else
        {
            if (sendLen < it->packetLen)
                LOG_ERROR << "Internal Error: DivertSend truncated send packet.";

        }

        sendCnt++;
        packetsList.pop_front();
    } // while(...)

    DWORD curTime = timeGetTime();
    if (curTime - lastTime > 3000)
    {
        LOG_INFO << "send count=" << sendCnt;

        sendCnt = 0;
        lastTime = curTime;
    }
}

void DivertWorker::divertProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle, Direction dir)
{
    lagProcess(packetsList, bundle, dir);
    dropProcess(packetsList, bundle);
    throttleProcess(packetsList, bundle, dir);
    duplicateProcess(packetsList, bundle);
    outoforderProcess(packetsList, bundle);
    tamperProcess(packetsList, bundle);
}

void DivertWorker::lagProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle, Direction dir)
{
    if (!bundle.lagEnabled)
        return;

    static std::list<PacketNode> inboundTmpList;
    static std::list<PacketNode> outboundTmpList;

    std::list<PacketNode> *tmpListPtr = &inboundTmpList;
    if (dir == Direction::outbound)
        tmpListPtr = &outboundTmpList;

    // dump all valid packets to another list
    for (std::list<PacketNode>::iterator it = packetsList.begin(); it != packetsList.end();)
    {
        auto itt = it;
        it++;
        itt->lagExpiredTimestamp += bundle.lagTime.rand();
        tmpListPtr->splice(tmpListPtr->end(), packetsList, itt);
    }

    // judge the lag time
    DWORD currentTime = timeGetTime();
    for (std::list<PacketNode>::iterator it = tmpListPtr->begin(); it != tmpListPtr->end();)
    {
        if (it->lagExpiredTimestamp < currentTime)
        {
            auto itt = it;
            it++;
            packetsList.splice(packetsList.end(), *tmpListPtr, itt);
        }
        else
        {
            it++;
        }
    }
}

void DivertWorker::dropProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle)
{
    if (!bundle.dropEnabled)
        return;

    for (std::list<PacketNode>::iterator it = packetsList.begin(); it != packetsList.end();)
    {
        auto itt = it;
        it++;
        if (calcChance(bundle.dropChance.rand()))
        {
            packetsList.erase(itt);
        }
    }
}

void DivertWorker::throttleProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle, Direction dir)
{
    if (!bundle.throttleEnabled)
        return;

    static std::list<PacketNode> inboundTmpList;
    static std::list<PacketNode> outboundTmpList;
    static DWORD inboundStartTick = 0;
    static DWORD outboundStartTick = 0;
    static int inboundTimeFrame = 0;
    static int outboundTimeFrame = 0;

    std::list<PacketNode> *tmpListPtr = &inboundTmpList;
    DWORD *startTickPtr = &inboundStartTick;
    int *timeFrame = &inboundTimeFrame;
    if (dir == Direction::outbound)
    {
        tmpListPtr = &outboundTmpList;
        startTickPtr = &outboundStartTick;
        timeFrame = &outboundTimeFrame;
    }

    // dump new packets to temp list
    for (std::list<PacketNode>::iterator it = packetsList.begin(); it != packetsList.end();)
    {
        if (calcChance(bundle.dropChance.rand()))
        {
            auto itt = it;
            it++;
            tmpListPtr->splice(tmpListPtr->end(), packetsList, itt);

            // record the start time
            if (*startTickPtr == 0)
            {
                *startTickPtr = timeGetTime();
                *timeFrame = bundle.throttleTimeFrame.rand();
            }
        }
        else
        {
            it++;
        }
    }

    // restore the packets in timeframe to list
    DWORD currentTick = timeGetTime();
    if (currentTick - *startTickPtr > *timeFrame)
    {
        packetsList.splice(packetsList.end(), *tmpListPtr);
        *startTickPtr = 0;
    }
}

void DivertWorker::duplicateProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle)
{
    if (!bundle.duplicateEnabled)
        return;

    for (std::list<PacketNode>::iterator it = packetsList.begin(); it != packetsList.end();)
    {
        if (calcChance(bundle.duplicateChance.rand()))
        {
            int count = bundle.duplicateCount.rand();
            while (count--)
                packetsList.insert(it, *it);
        }
    }
}

void DivertWorker::outoforderProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle)
{
    if (!bundle.outoforderEnabled)
        return;

    for (std::list<PacketNode>::iterator it = packetsList.begin(); it != packetsList.end();)
    {

    }
}

void DivertWorker::tamperProcess(std::list<PacketNode> &packetsList, VariableBundle &bundle)
{
    if (!bundle.tamperEnabled)
        return;

    for (std::list<PacketNode>::iterator it = packetsList.begin(); it != packetsList.end();)
    {
        if (calcChance(bundle.duplicateChance.rand()))
        {
            char *data = NULL;
            UINT dataLen = 0;
            if (WinDivertHelperParsePacket(it->packet, it->packetLen, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                           (PVOID*)&data, &dataLen, NULL, NULL) && data != NULL && dataLen != 0)
            {
                // try to tamper the central part of the packet,
                // since common packets put their checksum at head or tail
                if (dataLen <= 4)
                {
                    // for short packet just tamper it all
                    tamperBuf(data, dataLen);
                }
                else
                {
                    // for longer ones process 1/4 of the lens start somewhere in the middle
                    UINT len = dataLen;
                    UINT len_d4 = len / 4;
                    tamperBuf(data + len/2 - len_d4/2 + 1, len_d4);
                }

                if (bundle.tamperRedoChecksum)
                {
                    WinDivertHelperCalcChecksums(it->packet, it->packetLen, NULL, 0);
                }
            } // if (WinDivertHelperParsePacket...)
        } // if (calcChance...)
    } // for
}
