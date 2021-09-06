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

KV58FirmwareDlg::KV58FirmwareDlg(QSerialPort* com, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KV58FirmwareDlg),
    m_com(com)
{
    ui->setupUi(this);
    QSettings s("jjterm.ini", QSettings::IniFormat);
    ui->lbPath->setText(s.value("FirmwarePath").toString());
    ui->lbPath->setToolTip(s.value("FirmwarePath").toString());
}

KV58FirmwareDlg::~KV58FirmwareDlg()
{
    QSettings s("jjterm.ini", QSettings::IniFormat);
    s.setValue("FirmwarePath", ui->lbPath->text());
    delete ui;
}

void KV58FirmwareDlg::readReady(const QByteArray data)
{
    if (data.contains("FIRMWARE-START:"))
    {

    }
    ui->lvCommText->insertPlainText(data);
    if (m_bScroll) ui->lvCommText->scrollToBottom();
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
            QByteArray bin = file.readAll();
            if (m_com)
            {
                int index = 0, len = 8;
                m_com->write(QString("firm:%1").arg(bin.length()).toUtf8());
                m_com->waitForBytesWritten();
                while (index < bin.length())
                {
                    m_com->write(bin.mid(index, len));
                    m_com->waitForBytesWritten();
                    index += len;
                }
            }
        }
    }
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

}

void KV58FirmwareDlg::on_pbReboot_clicked()
{
    if (m_com)
        m_com->write(QByteArray("bootldr:\n"));
}

void KV58FirmwareDlg::on_pbBootApp_clicked()
{
    if (m_com)
        m_com->write(QByteArray("bootapp:\n"));
}

