#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QMutex>
#include <QMutexLocker>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>

#include "udpconn.h"
#include "asgraph.h"
#include "kv58firmwaredlg.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget* parent = nullptr);
    ~Dialog();

private slots:
    void dbgMsgReceived(const QByteArray &msg);

    void writeData(const QByteArray &data);

    void on_btRefreshPorts_clicked();

    void on_btClose_clicked();

    void on_btClear_clicked();

    void errorOccurred(QSerialPort::SerialPortError error);

    void readyRead();

    void on_btOpen_clicked();

    void on_btScroll_clicked();

    void on_btGraph1_clicked();

    void on_btGraph2_clicked();

    void on_btGraph3_clicked();

    void on_btKV58FirmwareDownload_clicked();

private:
    Ui::Dialog *ui;
    QScopedPointer<QSerialPort> m_serial { new QSerialPort(this) };
    QScopedPointer<KV58FirmwareDlg> m_firmware { new KV58FirmwareDlg(m_serial.data(), this) };
    QByteArray m_data;
    bool m_bScroll;
    QVector<ASGraph*> m_graphs;
    void ProcessEditData(QByteArray& data);
    void ProcessGraphData(QByteArray& data);
    void ProcessScatteredGraphData(QByteArray& data);
    void ProcessMotionGraphData(QByteArray& data);
    QColor color(int index);
    int m_RX[3] { 0 };
    QMutex m_mutex;
    UDPConn m_con;
    void processData();
};

#endif // DIALOG_H
