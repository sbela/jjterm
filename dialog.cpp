#include "dialog.h"
#include "ui_dialog.h"
#include <QDateTime>
#include <QDebug>
#include <QSettings>
#include <QThread>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    QSettings s("jjterm.ini", QSettings::IniFormat);
    QString port_name = s.value("PortName", "COM4").toString();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    ui->cbPorts->clear();
    foreach (QSerialPortInfo port, ports)
    {
        ui->cbPorts->addItem(port.portName());
        if (port_name == port.portName())
            ui->cbPorts->setCurrentIndex(ui->cbPorts->count() - 1);
    }
    m_bScroll = true;
    connect(m_serial.data(), SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(errorOccurred(QSerialPort::SerialPortError)));
    connect(m_serial.data(), SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(&m_con, &UDPConn::dbgMsgReceived, this, &Dialog::dbgMsgReceived);
    connect(ui->lvTexts, &Console::getData, this, &Dialog::writeData);
    m_graphs.append(new ASGraph(0, this));
    m_graphs.append(new ASGraph(1, this));
    m_graphs.append(new ASGraph(2, this));
    ui->leIP->setText(s.value("ip", "192.168.100.100").toString());
    bool isSerialChecked = s.value("Serial", false).toBool();
    isSerialChecked ? ui->rbSerial->setChecked(isSerialChecked) : ui->rbTCPIP->setChecked(not isSerialChecked);
    restoreGeometry(s.value("geometry").toByteArray());
    ui->btKV58FirmwareDownload->setVisible(false);
}

Dialog::~Dialog()
{
    QSettings s("jjterm.ini", QSettings::IniFormat);
    s.setValue("Serial", ui->rbSerial->isChecked());
    s.setValue("geometry", saveGeometry());
    QByteArray *ba = new QByteArray("TEL:1\n");
    m_con.SendTo(ba);

    foreach (ASGraph* p, m_graphs)
        delete p;
    m_graphs.clear();

    if (m_serial->isOpen())
        m_serial->close();
    delete ui;
}

void Dialog::dbgMsgReceived(const QByteArray &msg)
{
    m_data.append(msg);
    processData();
    ui->lvTexts->insertPlainText(msg);
    if (m_bScroll)
        ui->lvTexts->scrollToBottom();
}

void Dialog::writeData(const QByteArray &data)
{
    if (ui->rbSerial->isChecked())
        m_serial->write(data);
    else if (ui->rbTCPIP->isChecked())
    {
        QByteArray *ba = new QByteArray;
        ba->append("T:");
        ba->append(data);
        ba->append('\0');
        m_con.SendTo(ba);
    }
}

void Dialog::on_btRefreshPorts_clicked()
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    ui->cbPorts->clear();
    foreach (QSerialPortInfo port, ports)
    {
        ui->cbPorts->addItem(port.portName());
    }
}

void Dialog::on_btClose_clicked()
{
    if (m_serial->isOpen())
        m_serial->close();
    close();
}

void Dialog::on_btClear_clicked()
{
    m_data.clear();
    ui->lvTexts->clear();
}

void Dialog::errorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
    {
        ui->lvTexts->insertPlainText("ERROR: [" + m_serial->errorString() + "]");
        if (m_bScroll)
            ui->lvTexts->scrollToBottom();
    }
}

void Dialog::readyRead()
{
    QMutexLocker lock(&m_mutex);
    if (m_firmware->isVisible())
    {
        m_firmware->readReady(m_serial->readAll());
        return;
    }

    QByteArray data = m_serial->readAll();
    m_data.append(data);

    processData();

    //qDebug() << "(" << data << ")";
    if (data.length() > 0)
    {
        ui->lvTexts->insertPlainText(data);
        if (m_bScroll)
            ui->lvTexts->scrollToBottom();
    }
    //qDebug() << "QQ";
}

#define SET_TEXT(nr) ui->le##nr->setText(data.mid(hastag_space + 1))
void Dialog::ProcessEditData(QByteArray& data)
{
    qDebug() << "ProcessEditData [" << data << "]";
    int hastag_pos = data.indexOf('#');
    int hastag_space = data.indexOf(' ', hastag_pos);
    int number = data.mid(hastag_pos + 1, hastag_space - hastag_pos - 1).toInt();
    switch (number)
    {
        case 0:
            SET_TEXT(0);
            break;
        case 1:
            SET_TEXT(1);
            break;
        case 2:
            SET_TEXT(2);
            break;
        case 3:
            SET_TEXT(3);
            break;
        case 4:
            SET_TEXT(4);
            break;
        case 5:
            SET_TEXT(5);
            break;
        case 6:
            SET_TEXT(6);
            break;
        case 7:
            SET_TEXT(7);
            break;
        case 8:
            SET_TEXT(8);
            break;
        case 9:
            SET_TEXT(9);
            break;
        default:
            ui->lvTexts->insertPlainText(QString("Invalid EDIT index : %1 [%2]").arg(number).arg(data.constData()));
            if (m_bScroll)
                ui->lvTexts->scrollToBottom();
            break;
    }
}

void Dialog::on_btOpen_clicked()
{
    if (ui->rbSerial->isChecked())
    {
        if (ui->btOpen->text() == "Connect")
        {
            m_serial->setPortName(ui->cbPorts->currentText());
            m_serial->setBaudRate(115200);
            m_serial->setDataBits(QSerialPort::Data8);
            m_serial->setParity(QSerialPort::NoParity);
            m_serial->setStopBits(QSerialPort::OneStop);
            m_serial->setFlowControl(QSerialPort::NoFlowControl);
            if (m_serial->open(QIODevice::ReadWrite))
            {
                ui->lvTexts->insertPlainText("Connected to : " + ui->cbPorts->currentText());
                if (m_bScroll)
                    ui->lvTexts->scrollToBottom();
                ui->btOpen->setText("Disconnect");
                if (m_bScroll) ui->lvTexts->scrollToBottom();
                QSettings s("jjterm.ini", QSettings::IniFormat);
                s.setValue("PortName", ui->cbPorts->currentText());
                ui->cbPorts->setEnabled(false);
                ui->rbSerial->setEnabled(false);
                ui->rbTCPIP->setEnabled(false);
                ui->btKV58FirmwareDownload->setVisible(true);
            }
            else
            {
                ui->lvTexts->insertPlainText("Error : " + m_serial->errorString());
                if (m_bScroll) ui->lvTexts->scrollToBottom();
            }
        }
        else
        {
            ui->btKV58FirmwareDownload->setVisible(false);
            ui->rbSerial->setEnabled(true);
            ui->rbTCPIP->setEnabled(true);
            ui->cbPorts->setEnabled(true);
            ui->btOpen->setText("Connect");
            if (m_serial->isOpen())
            {
                m_serial->close();
                ui->lvTexts->insertPlainText("Disconnected from : " + ui->cbPorts->currentText());
                if (m_bScroll) ui->lvTexts->scrollToBottom();
            }
        }
    }
    else if (ui->rbTCPIP->isChecked())
    {
        if (ui->btOpen->text() == "Connect")
        {
            m_con.Connect(ui->leIP->text());
            QByteArray *ba = new QByteArray("TEL:0\n");
            m_con.SendTo(ba);
            ui->rbSerial->setEnabled(false);
            ui->rbTCPIP->setEnabled(false);
            ui->btOpen->setText("Disconnect");
            QSettings s("jjterm.ini", QSettings::IniFormat);
            s.setValue("ip", ui->leIP->text());
        }
        else
        {
            QByteArray *ba = new QByteArray("TEL:1\n");
            m_con.SendTo(ba);
            m_con.Connect("");
            ui->rbSerial->setEnabled(true);
            ui->rbTCPIP->setEnabled(true);
            ui->btOpen->setText("Connect");
        }
    }
}

void Dialog::on_btScroll_clicked()
{
    if (ui->btScroll->text() == "Stop scroll")
    {
        ui->btScroll->setText("Start scroll");
        m_bScroll = false;
    }
    else
    {
        ui->btScroll->setText("Stop scroll");
        m_bScroll = true;
    }
}

void Dialog::ProcessGraphData(QByteArray& data)
{
    qDebug() << "ProcessGraphData [" << data << "]";
    int pos = data.indexOf('$');
    int graph_index = data.at(pos + 1) - '0';
    if ((graph_index > -1) && (graph_index < 3)) {
        ASGraph* p = m_graphs[graph_index];
        int apostrophe_pos = data.indexOf('\'', pos);
        int y = data.mid(pos + 3, apostrophe_pos - pos - 3).toInt();
        int at_pos = data.indexOf('@', apostrophe_pos);
        int x = data.mid(apostrophe_pos + 1, at_pos - apostrophe_pos - 1).toInt();
        int clr = data.mid(at_pos + 1).at(0);
        qDebug() << "x:" << x << " y:" << y << " c:" << color(clr);
        p->addPoint(x, y, color(clr));
        switch (graph_index) {
        case 0:
            ui->frG0->setStyleSheet(++m_RX[0] % 2 ? "background-color: rgb(170, 0, 0);" : "background-color: rgb(255, 0, 0);");
            break;
        case 1:
            ui->frG1->setStyleSheet(++m_RX[1] % 2 ? "background-color: rgb(170, 0, 0);" : "background-color: rgb(255, 0, 0);");
            break;
        case 2:
            ui->frG2->setStyleSheet(++m_RX[2] % 2 ? "background-color: rgb(170, 0, 0);" : "background-color: rgb(255, 0, 0);");
            break;
        }
    } else {
        ui->lvTexts->insertPlainText(QString("Invalid GRAPH index : %1 [%2]").arg(graph_index).arg(data.constData()));
        if (m_bScroll)
            ui->lvTexts->scrollToBottom();
    }
}

void Dialog::ProcessScatteredGraphData(QByteArray& data)
{
    qDebug() << "ProcessScatteredGraphData [" << data << "]";
    int pos = data.indexOf('$');
    int graph_index = data.at(pos + 1) - '0';
    if ((graph_index > -1) && (graph_index < 3))
    {
        ASGraph* p = m_graphs[graph_index];
        int at_pos = data.indexOf('@', pos + 3);
        int y = data.mid(pos + 3, at_pos - pos - 3).trimmed().toInt();
        int clr = data.mid(at_pos + 1).at(0);
        qDebug() << " y:" << y << " c:" << color(clr);
        p->addScatteredPoint(y, color(clr));
        switch (graph_index)
        {
            case 0:
                ui->frG0->setStyleSheet(++m_RX[0] % 2 ? "background-color: rgb(170, 0, 0);" : "background-color: rgb(255, 0, 0);");
                break;
            case 1:
                ui->frG1->setStyleSheet(++m_RX[1] % 2 ? "background-color: rgb(170, 0, 0);" : "background-color: rgb(255, 0, 0);");
                break;
            case 2:
                ui->frG2->setStyleSheet(++m_RX[2] % 2 ? "background-color: rgb(170, 0, 0);" : "background-color: rgb(255, 0, 0);");
                break;
        }
    }
    else
    {
        ui->lvTexts->insertPlainText(QString("Invalid GRAPH index : %1 [%2]").arg(graph_index).arg(data.constData()));
        if (m_bScroll)
            ui->lvTexts->scrollToBottom();
    }
}

void Dialog::ProcessMotionGraphData(QByteArray &data)
{
    ASGraph* p = m_graphs[0];
    for (int i = 1; i < data.length(); i+=4)
    {
        int y = data.mid(i, 4).trimmed().toUpper().toInt(nullptr, 16);
        p->addScatteredPoint(y, Qt::red, false);
    }
    p->replot();
    ui->frG0->setStyleSheet(++m_RX[0] % 2 ? "background-color: rgb(170, 0, 0);" : "background-color: rgb(255, 0, 0);");
}

QColor Dialog::color(int index)
{
    QColor color(Qt::black);
    switch (index) {
    case 'R':
        color = Qt::red;
        break;
    case 'G':
        color = Qt::green;
        break;
    case 'B':
        color = Qt::blue;
        break;
    case 'Y':
        color = Qt::yellow;
        break;
    case 'M':
        color = Qt::magenta;
        break;
    case 'C':
        color = Qt::cyan;
        break;
    }
    return color;
}

void Dialog::processData()
{
    if (m_data.contains('\r'))
    {
        //qDebug() << ">" << data << "<";
        while (m_data.length() && m_data.contains('\r'))
        {
            QByteArray line = m_data.mid(0, m_data.indexOf('\r'));
            m_data.remove(0, m_data.indexOf('\r') + 1);
            line = line.trimmed();
            if (line.length())
            {
                //PRINTF("{%s}", line.constData());
                //qDebug() << "{" << line << "}";
                if (line.contains('$') && line.contains('\'') && line.contains('@'))
                    ProcessGraphData(line);
                else if (line.contains('$') && !line.contains('\'') && line.contains('@'))
                    ProcessScatteredGraphData(line);
                else if (line.contains('#'))
                    ProcessEditData(line);
            }
        }
    }
}

void Dialog::on_btGraph1_clicked()
{
    if (m_graphs.at(0)->isHidden())
        m_graphs.at(0)->show();
    else
        m_graphs.at(0)->hide();
}

void Dialog::on_btGraph2_clicked()
{
    if (m_graphs.at(1)->isHidden())
        m_graphs.at(1)->show();
    else
        m_graphs.at(1)->hide();
}

void Dialog::on_btGraph3_clicked()
{
    if (m_graphs.at(2)->isHidden())
        m_graphs.at(2)->show();
    else
        m_graphs.at(2)->hide();
}

void Dialog::on_btKV58FirmwareDownload_clicked()
{
    m_firmware->setVisible(true);
}
