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
#ifndef KV58FIRMWAREDLG_H
#define KV58FIRMWAREDLG_H

#include <QDialog>
#include <QSerialPort>
#include <QPointer>
#include <QSettings>

namespace Ui {
    class KV58FirmwareDlg;
}

class KV58FirmwareDlg : public QDialog
{
    Q_OBJECT

public:
    explicit KV58FirmwareDlg(QSerialPort* com, QWidget *parent = nullptr);
    ~KV58FirmwareDlg();
    void readReady(const QByteArray data);
private slots:
    void on_pbClose_clicked();

    void on_pbDownload_clicked();

    void on_pbClearCommList_clicked();

    void on_pbStopScrollCommText_clicked();

    void on_pbFirmwarePath_clicked();

    void on_pbReboot_clicked();

    void on_pbBootApp_clicked();

    void on_pbVersion_clicked();

private:
    Ui::KV58FirmwareDlg *ui;
    QPointer<QSerialPort> m_com;
    bool m_bScroll { true };

    inline void Send(const char* msg);
    inline void Send(const QString &msg);
};

#endif // KV58FIRMWAREDLG_H
