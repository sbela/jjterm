/*
 * Copyright 2020. 04. 12. sbela
 * @ AutoSys KnF Kft.
 * web: http://www.autosys.hu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
*/
#include "udpconn.h"

UDPConn::UDPConn(QObject *parent) :
    QObject(parent)
{
    updateIPList();
    bindSockets();
    connect(&m_connTimer, SIGNAL(timeout()), this, SLOT(updateConnTimer()));
}

UDPConn::~UDPConn()
{
    if (m_pSvcSocket) delete m_pSvcSocket;
    m_pSvcSocket = nullptr;

    if (m_pDataSocket) delete m_pDataSocket;
    m_pDataSocket = nullptr;

    if (m_pDbgMsgSocket) delete m_pDbgMsgSocket;
    m_pDbgMsgSocket = nullptr;
}

void UDPConn::Connect(const QString ip)
{
    PRINTF("Connecting to : [%s]", ip.toUtf8().constData());
    if (ip.length())
    {
        QString subNet = ip.mid(0, ip.lastIndexOf('.'));
        QString listenSubNet = m_listen_ip.mid(0, m_listen_ip.lastIndexOf('.'));
        if (subNet == listenSubNet)
        {
            m_ip = ip;
            bindSockets();
            m_connTimer.start(m_connTimeOut);
        }
        else
        {
            PRINTF("Subnet mismatch : [%s] != [%s]! Rebound!", subNet.toUtf8().constData(), listenSubNet.toUtf8().constData());
            updateIPList();
            for (const QString &sn : qAsConst(m_ipList))
            {
                QString lsn = sn.mid(0, sn.lastIndexOf('.'));
                if (lsn == subNet)
                {
                    m_listen_ip = sn;
                    bindSockets();
                    m_ip = ip;
                    m_connTimer.start(m_connTimeOut);
                }
            }
        }
    }
    else
    {
        unbindSockets();
        m_connTimer.stop();
        m_ip = ip;
    }
    emit connectionStateChanged();
}

void UDPConn::SendTo(const QByteArray *ba, quint16 port)
{
    if (ba)
    {
        if (m_pSvcSocket and m_ip.length())
        {
            int sent = m_pSvcSocket->writeDatagram(*ba, QHostAddress(m_ip), port);
            if (sent != ba->size())
                PRINTF("UDPConn pkt sent size missmatch - sent [%d] != size [%d] [%s:%d]", sent, ba->size(), m_ip.toUtf8().constData(), port);
        }
        delete ba;
    }
}

void UDPConn::processTheSVCDatagram(const QByteArray &ba)
{
    if (ba.startsWith("CT:") or ba.startsWith("CTX:"))
    {
        int start = ba.startsWith("CTX:") ? 4 : 3;
        int len = ba.lastIndexOf(':') - start;
        QList<QByteArray> _ls = ba.mid(start, len).split('\n');
        PRINTF("P[%d][%d][%d]", start, len, _ls.size());
    }
    else if (ba.startsWith("AP:"))
        emit optionsMsgRecv(ba);
}

void UDPConn::processTheDataDatagram(const QByteArray &ba)
{
    Q_UNUSED(ba)
    m_connTimer.start(m_connTimeOut);
}

void UDPConn::processTheDbgMsgDatagram(const QByteArray &ba)
{
    emit dbgMsgReceived(ba);
}

void UDPConn::updateConnTimer()
{
    Connect("");
}

void UDPConn::readPendingDataDatagrams()
{
    if (m_pDataSocket)
    {
        while (m_pDataSocket->hasPendingDatagrams())
        {
            int size = m_pDataSocket->pendingDatagramSize();
            if (size > 0)
            {
                static QByteArray datagram;
                static QHostAddress sender;
                static quint16 senderPort;
                datagram.resize(size);

                if (m_pDataSocket->readDatagram(datagram.data(), size, &sender, &senderPort) > -1)
                {
                    //PRINTF("DATA RECV[%d][%s:%d]", size, sender.toString().toUtf8().constData(), senderPort);
                    if (datagram.size() != size)
                        PRINTF("readPendingDataDatagrams !!SIZE ERROR!! [senderPort: %d != %d] !! (datagram.size() [%d])",
                                senderPort, CROSSTABLE_DATAPORT, datagram.size());

                    if (datagram.size() > 0)
                        processTheDataDatagram(datagram);
                    else
                        PRINTF("readPendingDataDatagrams !!PORT ERROR!! [senderPort: %d != %d] !! (datagram.size() [%d])",
                                senderPort, CROSSTABLE_DATAPORT, datagram.size());
                }
                else
                    PRINTF("readPendingDataDatagrams !!ERROR!! [m_pDataSocket->readDatagram !> -1]");
            }
            else
                PRINTF("readPendingDataDatagrams !!ERROR!! [size: %d <= 0]", size);
        }
    }
}

void UDPConn::readPendingSvcDatagrams()
{
    if (m_pSvcSocket)
    {
        while (m_pSvcSocket->hasPendingDatagrams())
        {
            int size = m_pSvcSocket->pendingDatagramSize();            
            if (size > 0)
            {
                static QByteArray datagram;
                static QHostAddress sender;
                static quint16 senderPort;
                datagram.resize(size);

                if (m_pSvcSocket->readDatagram(datagram.data(), size, &sender, &senderPort) > -1)
                {
                    //PRINTF("SVC RECV[%d][%s:%d]", size, sender.toString().toUtf8().constData(), senderPort);
                    if (datagram.size() != size)
                        PRINTF("readPendingSvcDatagrams !!SIZE ERROR!! [senderPort: %d != %d] !! (datagram.size() [%d])",
                                senderPort, CROSSTABLE_SVCPORT, datagram.size());

                    if (datagram.size() > 0)
                        processTheSVCDatagram(datagram);
                    else
                        PRINTF("readPendingSvcDatagrams !!PORT ERROR!! [senderPort: %d != %d] !! (datagram.size() [%d])",
                                senderPort, CROSSTABLE_SVCPORT, datagram.size());
                }
                else
                    PRINTF("readPendingSvcDatagrams !!ERROR!! [m_pSvcSocket->readDatagram !> -1]");
            }
            else
                PRINTF("readPendingSvcDatagrams !!ERROR!! [size: %d <= 0]", size);
        }
    }
}

void UDPConn::readPendingDbgMsgDatagrams()
{
    if (m_pDbgMsgSocket)
    {
        while (m_pDbgMsgSocket->hasPendingDatagrams())
        {
            int size = m_pDbgMsgSocket->pendingDatagramSize();
            if (size > 0)
            {
                static QByteArray datagram;
                static QHostAddress sender;
                static quint16 senderPort;
                datagram.resize(size);

                if (m_pDbgMsgSocket->readDatagram(datagram.data(), size, &sender, &senderPort) > -1)
                {
                    //PRINTF("DBG RECV[%d][%s:%d]", size, sender.toString().toUtf8().constData(), senderPort);
                    if (datagram.size() != size)
                        PRINTF("readPendingDbgMsgDatagrams !!SIZE ERROR!! [senderPort: %d != %d] !! (datagram.size() [%d])",
                               senderPort, CROSSTABLE_DBGPORT, datagram.size());

                    if (datagram.size() > 0)
                        processTheDbgMsgDatagram(datagram);
                    else
                        PRINTF("readPendingDbgMsgDatagrams !!PORT ERROR!! [senderPort: %d != %d] !! (datagram.size() [%d])",
                               senderPort, CROSSTABLE_DBGPORT, datagram.size());
                }
                else
                {
                    PRINTF("readPendingDbgMsgDatagrams !!ERROR!! [m_pSvcSocket->readDatagram !> -1]");
                    break;
                }
            }
            else
            {
                PRINTF("readPendingDbgMsgDatagrams !!ERROR!! [size: %d <= 0]", size);
                break;
            }
        }
    }
}

void UDPConn::sendToClient(QByteArray *ba)
{
    SendTo(ba);
}

void UDPConn::updateIPList()
{
    m_ipList.clear();
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const auto &address: QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
        {
            if (!m_ipList.contains(address.toString()))
                m_ipList.append(address.toString());
        }
    }

    for (QNetworkInterface &iface : QNetworkInterface::allInterfaces())
    {
        QString name = iface.humanReadableName();
        PRINTF("i:[%s][%s]", name.toUtf8().constData(),
               iface.hardwareAddress().toUtf8().constData());
        for (QNetworkAddressEntry &entry: iface.addressEntries())
        {
            PRINTF("\te:[%s]", entry.ip().toString().toUtf8().constData());
        }
    }
}

void UDPConn::bindDataSocket(const QHostAddress &ipaddr)
{
    bool bConnect = false;
    PRINTF("BIND m_pDataSocket : %s:%d", ipaddr.toString().toUtf8().constData(), CROSSTABLE_DATAPORT);
    if (!m_pDataSocket->bind(QHostAddress::AnyIPv4, CROSSTABLE_DATAPORT))
    {
        PRINTF("m_pDataSocket : %s:%d FAILED TRYING AGAIN", ipaddr.toString().toUtf8().constData(), CROSSTABLE_DATAPORT);
        QHostAddress ip("1.1.1.1");
        m_pDataSocket->bind(ip, CROSSTABLE_DATAPORT);
        QThread::msleep(50);
        if (!m_pDataSocket->bind(QHostAddress::AnyIPv4, CROSSTABLE_DATAPORT))
        {
            PRINTF("m_pDataSocket : %s:%d FAILED GIVING UP!", ipaddr.toString().toUtf8().constData(), CROSSTABLE_DBGPORT);
        }
        else
            bConnect = true;
    }
    else
        bConnect = true;

    if (bConnect)
    {
        m_dataSocketBound = true;
        connect(m_pDataSocket, SIGNAL(readyRead()), this, SLOT(readPendingDataDatagrams()));
    }
}

void UDPConn::bindDbgMsgSocket(const QHostAddress &ipaddr)
{
    bool bConnect = false;
    PRINTF("BIND m_pDbgMsgSocket : %s:%d", ipaddr.toString().toUtf8().constData(), CROSSTABLE_DBGPORT);
    if (!m_pDbgMsgSocket->bind(QHostAddress::AnyIPv4, CROSSTABLE_DBGPORT))
    {
        PRINTF("m_pDbgMsgSocket : %s:%d FAILED TRYING AGAIN!", ipaddr.toString().toUtf8().constData(), CROSSTABLE_DBGPORT);
        QHostAddress ip("1.1.1.1");
        m_pDbgMsgSocket->bind(ip, CROSSTABLE_DBGPORT);
        QThread::msleep(50);
        if (!m_pDbgMsgSocket->bind(QHostAddress::AnyIPv4, CROSSTABLE_DBGPORT))
        {
            PRINTF("m_pDbgMsgSocket : %s:%d FAILED GIVING UP!", ipaddr.toString().toUtf8().constData(), CROSSTABLE_DBGPORT);
        }
        else
            bConnect = true;
    }
    else
        bConnect = true;

    if (bConnect)
    {
        m_dbgMsgSocketBound = true;
        connect(m_pDbgMsgSocket, SIGNAL(readyRead()), this, SLOT(readPendingDbgMsgDatagrams()));
    }
}

void UDPConn::bindSockets()
{
    unbindSockets();
    QGuiApplication::processEvents();
    QThread::msleep(100);
    QGuiApplication::processEvents();
    m_pSvcSocket = new QUdpSocket;
    m_pDataSocket = new QUdpSocket;
    m_pDbgMsgSocket = new QUdpSocket;
    QHostAddress ipaddr = QHostAddress(m_listen_ip);
    if (not m_dataSocketBound)
        bindDataSocket(ipaddr);
    if (not m_dbgMsgSocketBound)
        bindDbgMsgSocket(ipaddr);
    connect(m_pSvcSocket, SIGNAL(readyRead()), this, SLOT(readPendingSvcDatagrams()));
}

void UDPConn::unbindSockets()
{
    if (m_pSvcSocket)
    {
        disconnect(m_pSvcSocket, SIGNAL(readyRead()), this, SLOT(readPendingSvcDatagrams()));
        m_pSvcSocket->close();
        delete m_pSvcSocket;
        m_pSvcSocket = nullptr;
    }
    if (m_pDataSocket)
    {
        disconnect(m_pDataSocket, SIGNAL(readyRead()), this, SLOT(readPendingDataDatagrams()));
        m_pDataSocket->close();
        delete m_pDataSocket;
        m_dataSocketBound = false;
        m_pDataSocket = nullptr;
    }
    if (m_pDbgMsgSocket)
    {
        disconnect(m_pDbgMsgSocket, SIGNAL(readyRead()), this, SLOT(readPendingDbgMsgDatagrams()));
        m_pDbgMsgSocket->close();
        delete m_pDbgMsgSocket;
        m_dbgMsgSocketBound = false;
        m_pDbgMsgSocket = nullptr;
    }
}

int UDPConn::connTimeOut() const
{
    return m_connTimeOut;
}

void UDPConn::setConnTimeOut(int connTimeOut)
{
    m_connTimeOut = connTimeOut;
}
