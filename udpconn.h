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
#pragma once

#include <QGuiApplication>
#include <QBuffer>
#include <QByteArray>
#include <QCryptographicHash>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QtNetwork/QUdpSocket>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QTimer>
#include <QDateTime>
#include <QDir>

#define PRINT(format, ...)         do { { printf("%s : %s - %d # ", QDateTime::currentDateTime().toString("yy.MM.dd hh:mm:ss.zzz").toUtf8().constData(), __FILE__, __LINE__); printf(format, ##__VA_ARGS__); printf("\n"); fflush(stdout); } } while (0)
#define PRINTF(format, ...)                                                                                                                                                                           \
    do {                                                                                                                                                                                              \
        QString strDate = QDateTime::currentDateTime().toString("yy.MM.dd hh:mm:ss.zzz");                                                                                                             \
        QString strPath = QString("log/%1/%2/%3").arg(QDateTime::currentDateTime().toString("yy")).arg(QDateTime::currentDateTime().toString("MM")).arg(QDateTime::currentDateTime().toString("dd")); \
        QDir d;                                                                                                                                                                                       \
        d.mkpath(strPath);                                                                                                                                                                            \
        FILE* f = fopen(QString("%1/log.log").arg(strPath).toUtf8().constData(), "a+");                                                                                                               \
        fprintf(f, "%s : %s - %d # ", strDate.toUtf8().constData(), __FILE__, __LINE__);                                                                                                              \
        fprintf(f, format, ##__VA_ARGS__);                                                                                                                                                            \
        fprintf(f, "\n");                                                                                                                                                                             \
        fflush(f);                                                                                                                                                                                    \
        fclose(f);                                                                                                                                                                                    \
        printf("%s : %s - %d # ", strDate.toUtf8().constData(), __FILE__, __LINE__);                                                                                                                  \
        printf(format, ##__VA_ARGS__);                                                                                                                                                                \
        printf("\n");                                                                                                                                                                                 \
        fflush(stdout);                                                                                                                                                                               \
    } while (0)

#define CROSSTABLE_DATAPORT     8266
#define CROSSTABLE_SVCPORT      18266
#define CROSSTABLE_DBGPORT      28266

class UDPConn : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(UDPConn)
public:
    explicit UDPConn(QObject *parent = nullptr);
    virtual ~UDPConn() override;

    void Connect(const QString ip);
    void SendTo(const QByteArray *ba, quint16 port = CROSSTABLE_SVCPORT);

    const QString IP() const { return m_ip; }

    void processTheSVCDatagram(const QByteArray &ba);
    void processTheDataDatagram(const QByteArray &ba);
    void processTheDbgMsgDatagram(const QByteArray &ba);

    QString listenSubNet() const { return m_listen_ip.mid(0, m_listen_ip.lastIndexOf('.')); }
    int connTimeOut() const;
    void setConnTimeOut(int connTimeOut);

signals:
    void notificationChanged();
    void dbgMsgReceived(const QByteArray &msg);
    void optionsMsgRecv(const QByteArray &msg);
    void connectionStateChanged();

private slots:
    void updateConnTimer();
    void readPendingDataDatagrams();
    void readPendingSvcDatagrams();
    void readPendingDbgMsgDatagrams();
    void sendToClient(QByteArray *ba);

private:
    QUdpSocket *m_pDbgMsgSocket { nullptr };
    QUdpSocket *m_pDataSocket { nullptr };
    QUdpSocket *m_pSvcSocket { nullptr };
    QString m_notification { "" };
    QString m_ip { "" };
    QString m_listen_ip { "192.168.170.2" };
    QTimer m_connTimer;
    int m_connTimeOut { 5000000 };
    QStringList m_ipList;

    void updateIPList();

    bool m_dataSocketBound { false };
    bool m_dbgMsgSocketBound { false };
    void bindDataSocket(const QHostAddress &ipaddr);
    void bindDbgMsgSocket(const QHostAddress &ipaddr);
    void bindSockets();
    void unbindSockets();
};
