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
#include <QDebug>

#define SEND_LEN  256

class KV58FirmwareDlg::FirmwareDownloadTask final : public QRunnable
{
public:
    explicit FirmwareDownloadTask(KV58FirmwareDlg *parent, QByteArray bin, bool firmwareOrLoader)
        : m_parent(parent),
          m_bin(std::move(bin)),
          m_firmwareOrLoader(firmwareOrLoader)
    { }

    virtual void run() override
    {
        PRINTF("Firmware download start...");
        m_parent->m_data.clear();
        m_parent->m_bDownloadInProgress = true;
        int index = 0, len = SEND_LEN;
        PRINT("Sending file [%s] length %d!", m_parent->ui->lbPath->text().toUtf8().constData(), m_bin.length());
        m_parent->ui->prDownload->setMaximum(m_bin.length());
        m_parent->m_firmwareDownloadLock.lock();
        emit m_parent->sendToDevice(QString("%1:%2\n").arg(m_firmwareOrLoader ? "firm" : "load").arg(m_bin.length()).toUtf8());
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
        m_parent->m_notACKSession = true;
        m_parent->ui->prDownload->setValue(0);
    }

private:
    KV58FirmwareDlg *m_parent { nullptr };
    QByteArray m_bin;
    bool m_firmwareOrLoader;
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
    connect(&m_downloadSessionTimer, SIGNAL(timeout()), this, SLOT(DownloadTimeout()));
}

KV58FirmwareDlg::~KV58FirmwareDlg()
{
    QSettings s("jjterm.ini", QSettings::IniFormat);
    s.setValue("FirmwarePath", ui->lbPath->text());
    delete ui;
}

void KV58FirmwareDlg::readReady(const QByteArray data)
{
    //PRINTF("%s", data.constData());
    if (m_bDownloadInProgress)
    {
        m_data.append(data);
        if (m_data.contains("FIRMWARE-START:"))
        {
            ui->lvCommText->insertPlainText(data);
            m_data.clear();
            QMutexLocker m(&m_firmwareDownloadLock);
            ui->prDownload->setValue(0);
            m_firmwareDownloadWait.notify_one();
            m_notACKSession = false;
        }
        else if (m_data.contains("ACK"))
        {
            m_data.remove(m_data.indexOf("ACK"), strlen("ACK"));
            if (m_data.length())
            {
                ui->lvCommText->insertPlainText("[");
                ui->lvCommText->insertPlainText(m_data);
                ui->lvCommText->insertPlainText("]");
            }
            QMutexLocker m(&m_firmwareDownloadLock);
            ui->prDownload->setValue(ui->prDownload->value() + SEND_LEN);
            if (ui->prDownload->value() % 10240 == 0)
                ui->lvCommText->insertPlainText("*");
            m_firmwareDownloadWait.notify_one();
        }

        if (m_notACKSession)
            ui->lvCommText->insertPlainText(data);
    }
    else
    {
        if (m_downloadSession)
        {
            m_data.append(data);
            ui->lvCommText->insertPlainText("*");
            m_downloadSessionTimer.start(m_downloadSessionTimeout);
        }
        else if (data.contains('\0'))
        {
            QList<QByteArray> ls = data.split('\0');
            ui->lvCommText->insertPlainText(ls.join());
        }
        else
            ui->lvCommText->insertPlainText(data);
    }
    if (m_bScroll) ui->lvCommText->scrollToBottom();
}

void KV58FirmwareDlg::DownloadTimeout()
{
    m_downloadSessionTimer.stop();
    m_downloadSession = false;
    QFile file(m_downloadSessionFileName);
    if (file.open(QIODevice::WriteOnly))
    {
        if (m_data.contains("SEND:"))
        {
            int send_length = QString("SEND:").length();
            int index = m_data.indexOf("SEND:");
            PRINTF("index:%d", index);
            int lastIndex = m_data.indexOf(":", index  + send_length + 1);
            PRINTF("lastIndex:%d", lastIndex);
            QByteArray len = m_data.mid(index + send_length, lastIndex - (index + send_length));
            PRINTF("len:%s", len.constData());
            int length = len.toInt();
            PRINTF("length:%d", length);
            int max_length = m_receivingLoader ? FLASH_LOADER_LENGTH : FLASH_APP_LENGTH;
            PRINTF("max_length:%d", max_length);
            if ((length > 0) and (length <= max_length))
            {
                int data_length = m_data.length() - lastIndex - 1;
                PRINTF("data_length:%d", data_length);
                if (data_length == length)
                {
                    file.write(m_data.mid(lastIndex + 1));
                    ui->lvCommText->insertPlainText(QString("\nReceived data! Saved to [%1][%2 byte]").arg(m_downloadSessionFileName).arg(length));
                }
                else
                {
                    file.write(m_data);
                    ui->lvCommText->insertPlainText(QString("\nReceived data with invalid length [%1 != %2(data_length)]!\nSaved to [%1]").arg(length).arg(data_length).arg(m_downloadSessionFileName));
                }
            }
            else
            {
                file.write(m_data);
                ui->lvCommText->insertPlainText(QString("\nReceived data with invalid length [%1 != %2(max_length)]!\nSaved to [%3]").arg(length).arg(max_length).arg(m_downloadSessionFileName));
            }
        }
        else
        {
            file.write(m_data);
            ui->lvCommText->insertPlainText(QString("\nReceived invalid data! Saved to [%1]").arg(m_downloadSessionFileName));
        }
    }

    if (m_bScroll) ui->lvCommText->scrollToBottom();
    m_data.clear();
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
            {
                if ((file.size() > 0) and (file.size() <= FLASH_APP_LENGTH))
                    QThreadPool::globalInstance()->start(new FirmwareDownloadTask(this, file.readAll(), true));
                else
                    QMessageBox::information(this, QStringLiteral("Error"), QString("Invalid file size! [%1] (max:%2)").arg(file.size()).arg(FLASH_APP_LENGTH));
            }
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
    QString fileName = QFileDialog::getOpenFileName(this, ("Select the .bin file!"),
                                                    s.value("FirmwarePath", QDir::currentPath()).toString(), tr("Bin files (*.bin)"));
    if (QFile::exists(fileName))
    {
        QString path = QFileInfo(fileName).absoluteFilePath();
        s.setValue("FirmwarePath", path);
        ui->lbPath->setText(path);
        ui->lbPath->setToolTip(path);
    }
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

void KV58FirmwareDlg::on_pbDownloadBootLoader_clicked()
{
    if (QFile::exists(ui->lbPath->text()))
    {
        QFile file(ui->lbPath->text());
        if (file.open(QIODevice::ReadOnly))
        {
            if (m_com)
            {
                if ((file.size() > 0) and (file.size() <= FLASH_LOADER_LENGTH))
                    QThreadPool::globalInstance()->start(new FirmwareDownloadTask(this, file.readAll(), false));
                else
                    QMessageBox::information(this, QStringLiteral("Error"), QString("Invalid file size! [%1] (max:%2)").arg(file.size()).arg(FLASH_LOADER_LENGTH));
            }
            else
                PRINT("COM is not open!");
        }
        else
            PRINT("Can not open file: [%s]", ui->lbPath->text().toUtf8().constData());
    }
    else
        PRINT("File not exists[%s]", ui->lbPath->text().toUtf8().constData());
}

void KV58FirmwareDlg::on_pbSizes_clicked()
{
    Send("size\n");
}

void KV58FirmwareDlg::on_pbGetApplication_clicked()
{
    QSettings s("jjterm.ini", QSettings::IniFormat);
    QString fileName = QFileDialog::getSaveFileName(this, ("Set the save .bin file name!"),
                                                    s.value("FirmwareAppPath", QDir::currentPath()).toString(),
                                                    tr("Bin files (*.bin)"), nullptr, QFileDialog::DontConfirmOverwrite);
    if (fileName.length())
    {
        QString path = QFileInfo(fileName).absoluteFilePath();
        if (QFile::exists(path))
        {
            if (QMessageBox::question(this, "Overwrite ?", QString("File exists ?\n[%1]\nOverwrite ?").arg(path)) == QMessageBox::No)
                return;
        }

        s.setValue("FirmwareAppPath", path);
        m_downloadSessionFileName = path;
        m_downloadSession = true;
        m_data.clear();
        ui->lvCommText->insertPlainText("\nAPP:");
        if (m_bScroll) ui->lvCommText->scrollToBottom();
        m_downloadSessionTimer.start(m_downloadSessionTimeout);
        m_receivingLoader = false;
        Send("getfirm\n");
    }
}

void KV58FirmwareDlg::on_pbGetROM_clicked()
{
    QSettings s("jjterm.ini", QSettings::IniFormat);
    QString fileName = QFileDialog::getSaveFileName(this, ("Set the save .bin file name!"),
                                                    s.value("FirmwareROMPath", QDir::currentPath()).toString(),
                                                    tr("Bin files (*.bin)"), nullptr, QFileDialog::DontConfirmOverwrite);
    if (fileName.length())
    {
        QString path = QFileInfo(fileName).absoluteFilePath();
        if (QFile::exists(path))
        {
            if (QMessageBox::question(this, "Overwrite ?", QString("File exists ?\n[%1]\nOverwrite ?").arg(path)) == QMessageBox::No)
                return;
        }

        s.setValue("FirmwareROMPath", path);
        m_downloadSessionFileName = path;
        m_downloadSession = true;
        m_data.clear();
        ui->lvCommText->insertPlainText("\nROM:");
        if (m_bScroll) ui->lvCommText->scrollToBottom();
        m_downloadSessionTimer.start(m_downloadSessionTimeout);
        m_receivingLoader = false;
        Send("getrom\n");
    }
}

void KV58FirmwareDlg::on_pbGetLoader_clicked()
{
    QSettings s("jjterm.ini", QSettings::IniFormat);
    QString fileName = QFileDialog::getSaveFileName(this, ("Set the save .bin file name!"),
                                                    s.value("FirmwareROMPath", QDir::currentPath()).toString(),
                                                    tr("Bin files (*.bin)"), nullptr, QFileDialog::DontConfirmOverwrite);
    if (fileName.length())
    {
        QString path = QFileInfo(fileName).absoluteFilePath();
        if (QFile::exists(path))
        {
            if (QMessageBox::question(this, "Overwrite ?", QString("File exists ?\n[%1]\nOverwrite ?").arg(path)) == QMessageBox::No)
                return;
        }

        s.setValue("FirmwareROMPath", path);
        m_downloadSessionFileName = path;
        m_downloadSession = true;
        m_data.clear();
        ui->lvCommText->insertPlainText("\nLDR:");
        if (m_bScroll) ui->lvCommText->scrollToBottom();
        m_downloadSessionTimer.start(m_downloadSessionTimeout);
        m_receivingLoader = true;
        Send("getload\n");
    }
}
