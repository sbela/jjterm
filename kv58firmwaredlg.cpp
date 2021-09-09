/*
 * Copyright 2021. 09. 01. autosys
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
#include "kv58firmwaredlg.h"
#include "ui_kv58firmwaredlg.h"
#include <QFileDialog>

#define SEND_LEN  256

class KV58FirmwareDlg::FirmwareDownloadTask final : public QRunnable
{
public:
    explicit FirmwareDownloadTask(KV58FirmwareDlg *parent, QByteArray bin)
        : m_parent(parent),
          m_bin(std::move(bin))
    { }

    virtual void run() override
    {
        PRINTF("Firmware download start...");
        m_parent->m_bDownloadInProgress = true;
        int index = 0, len = SEND_LEN;
        PRINT("Sending file [%s] length %d!", m_parent->ui->lbPath->text().toUtf8().constData(), m_bin.length());
        m_parent->ui->prDownload->setMaximum(m_bin.length());
        m_parent->m_firmwareDownloadLock.lock();
        emit m_parent->sendToDevice(QString("firm:%1\n").arg(m_bin.length()).toUtf8());
        PRINTF("Firmware waiting for sending...");
        if (m_parent->m_firmwareDownloadWait.wait(&m_parent->m_firmwareDownloadLock, QDeadlineTimer(5000)))
        {
            m_parent->m_firmwareDownloadLock.unlock();
            PRINTF("Sending the rest of the data!");
            while (index < m_bin.length())
            {
                QMutexLocker m(&m_parent->m_firmwareDownloadLock);
                QByteArray send = m_bin.mid(index, len);
                if (send.length() < SEND_LEN)
                    send.append(SEND_LEN - send.length(), '0');
                emit m_parent->sendToDevice(send);
                index += len;
                //PRINTF("<< %d [%d]", index, m_bin.length());
                if (not m_parent->m_firmwareDownloadWait.wait(&m_parent->m_firmwareDownloadLock, QDeadlineTimer(5000)))
                {
                    PRINTF("Device didn't respond in time!");
                    break;
                }
            }
        }
        else
        {
            m_parent->m_firmwareDownloadLock.unlock();
            PRINTF("Device did not respond in time [5000ms]!");
        }
        m_parent->m_bDownloadInProgress = false;
        m_parent->ui->prDownload->setValue(0);
    }

private:
    KV58FirmwareDlg *m_parent { nullptr };
    QByteArray m_bin;
};

KV58FirmwareDlg::KV58FirmwareDlg(QSerialPort* com, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KV58FirmwareDlg),
    m_com(com)
{
    ui->setupUi(this);
    QSettings s("jjterm.ini", QSettings::IniFormat);
    ui->lbPath->setText(s.value("FirmwarePath").toString());
    ui->lbPath->setToolTip(s.value("FirmwarePath").toString());
    connect(this, SIGNAL(sendToDevice(QByteArray)), this, SLOT(SendToDevice(QByteArray)), Qt::QueuedConnection);
}

KV58FirmwareDlg::~KV58FirmwareDlg()
{
    QSettings s("jjterm.ini", QSettings::IniFormat);
    s.setValue("FirmwarePath", ui->lbPath->text());
    delete ui;
}

void KV58FirmwareDlg::readReady(const QByteArray data)
{
    if (m_bDownloadInProgress)
    {
        m_data.append(data);
        if (m_data.contains("FIRMWARE-START:"))
        {
            ui->lvCommText->insertPlainText(m_data);
            m_data.clear();
            QMutexLocker m(&m_firmwareDownloadLock);
            ui->prDownload->setValue(0);
            m_firmwareDownloadWait.notify_one();
        }
        else if (m_data.contains("ACK"))
        {
            m_data.remove(m_data.indexOf("ACK"), strlen("ACK"));
            QMutexLocker m(&m_firmwareDownloadLock);
            ui->prDownload->setValue(ui->prDownload->value() + SEND_LEN);
            if (ui->prDownload->value() % 10240 == 0)
                ui->lvCommText->insertPlainText("*");
            m_firmwareDownloadWait.notify_one();
        }
    }
    else
        ui->lvCommText->insertPlainText(data);
    if (m_bScroll) ui->lvCommText->scrollToBottom();
}

void KV58FirmwareDlg::SendToDevice(QByteArray data)
{
    if (m_com)
        m_com->write(data);
}

void KV58FirmwareDlg::on_pbClose_clicked()
{
    accept();
}

void KV58FirmwareDlg::on_pbDownload_clicked()
{
    if (QFile::exists(ui->lbPath->text()))
    {
        QFile file(ui->lbPath->text());
        if (file.open(QIODevice::ReadOnly))
        {
            if (m_com)
                QThreadPool::globalInstance()->start(new FirmwareDownloadTask(this, file.readAll()));
            else
                PRINT("COM is not open!");
        }
        else
            PRINT("Can not open file: [%s]", ui->lbPath->text().toUtf8().constData());
    }
    else
        PRINT("File not exists[%s]", ui->lbPath->text().toUtf8().constData());
}

void KV58FirmwareDlg::on_pbClearCommList_clicked()
{
    ui->lvCommText->clear();
}

void KV58FirmwareDlg::on_pbStopScrollCommText_clicked()
{
    if (ui->pbStopScrollCommText->text() == "Stop scroll")
    {
        ui->pbStopScrollCommText->setText("Start scroll");
        m_bScroll = false;
    }
    else
    {
        ui->pbStopScrollCommText->setText("Stop scroll");
        m_bScroll = true;
    }
}

void KV58FirmwareDlg::on_pbFirmwarePath_clicked()
{
    QSettings s("jjterm.ini", QSettings::IniFormat);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QString fileName = QFileDialog::getOpenFileName(this, ("Select the .bin file!"),
                                                    s.value("FirmwarePath", QDir::currentPath()).toString(), tr("Bin files (*.bin)"));
    if (QFile::exists(fileName))
    {
        QString path = QFileInfo(fileName).absoluteFilePath();
        s.setValue("FirmwarePath", path);
        ui->lbPath->setText(path);
        ui->lbPath->setToolTip(path);
    }
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
}

void KV58FirmwareDlg::on_pbReboot_clicked()
{
    Send("bootldr\n");
}

void KV58FirmwareDlg::on_pbBootApp_clicked()
{
    Send("bootapp\n");
}

void KV58FirmwareDlg::on_pbVersion_clicked()
{
    Send("ver\n");
}

void KV58FirmwareDlg::Send(const char *msg)
{
    if (m_com)
        m_com->write(QByteArray(msg));
}

void KV58FirmwareDlg::Send(const QString &msg)
{
    if (m_com)
        m_com->write(msg.toUtf8());
}
