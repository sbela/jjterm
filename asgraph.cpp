/*
 * Copyright 2019. 12. 04. sbela
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
#include "asgraph.h"
#include "ui_asgraph.h"

ASGraph::ASGraph(int index, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ASGraph)
{
    ui->setupUi(this);
    QSettings s("jjterm.ini", QSettings::IniFormat);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    ui->qcGraph->addGraph();
    ui->qcGraph->xAxis->setLabel("X Axis");
    ui->qcGraph->yAxis->setLabel("Y Axis");
    ui->qcGraph->setInteraction(QCP::iRangeDrag, true);
    ui->qcGraph->setInteraction(QCP::iRangeZoom, true);
    setWindowTitle(QString("Graph: %1").arg(index));
    ui->cbLineStyles->setCurrentIndex(1);
    m_index = index;
    m_x_min = s.value(QString("XMIN%1").arg(index), 0).toInt();
    m_x_max = s.value(QString("XMAX%1").arg(index), 10000).toInt();
    int y_min = s.value(QString("YMIN%1").arg(index), 0).toInt();
    int y_max = s.value(QString("YMAX%1").arg(index), 10000).toInt();
    ui->sbXFrom->setValue(m_x_min);
    ui->sbXTo->setValue(m_x_max);
    ui->sbYFrom->setValue(y_min);
    ui->sbYTo->setValue(y_max);
    int samples = s.value(QString("SAMPLES%1").arg(index), 10000).toInt();
    ui->sbItemCount->setValue(samples);
    m_maxPointNr = samples;
    ui->qcGraph->xAxis->setRange(m_x_min, m_x_max);
    ui->qcGraph->yAxis->setRange(y_min, y_max);
    ui->qcGraph->replot();
    m_xWindow = m_x_max - m_x_min;
}

ASGraph::~ASGraph()
{
    qDebug() << "~ASGraph CLOSE";
    delete ui;
}

void ASGraph::addPoint(int x, int y, QColor clr)
{
    QMutexLocker lock(&m_mutex);
    int index = 0;
    if (m_graphIndex.count()) {
        bool bFound = false;
        for (int i = 0; i < m_graphIndex.count(); i++) {
            if (m_graphIndex.at(i).clr == clr) {
                index = i;
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            index = m_graphIndex.count();
            m_graphIndex.append({ clr, 0 });
            ui->qcGraph->addGraph();
            ui->qcGraph->graph(index)->setPen(clr);
        }
    } else {
        m_graphIndex.append({ clr, 0 });
        ui->qcGraph->graph(index)->setPen(clr);
    }

    ui->qcGraph->graph(index)->data()->remove(x);
    ui->qcGraph->graph(index)->addData(x, y);
    if (ui->qcGraph->graph(index)->dataCount() > m_maxPointNr) {
        ui->qcGraph->graph(index)->data()->removeBefore(ui->qcGraph->graph(index)->dataMainKey(0));
    }
    ui->qcGraph->replot();
}

void ASGraph::addScatteredPoint(int y, QColor clr, bool replot)
{
    QMutexLocker lock(&m_mutex);
    int index = 0;
    if (m_graphIndex.count()) {
        bool bFound = false;
        for (int i = 0; i < m_graphIndex.count(); i++) {
            if (m_graphIndex.at(i).clr == clr) {
                index = i;
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            index = m_graphIndex.count();
            m_graphIndex.append({ clr, 0 });
            ui->qcGraph->addGraph();
            ui->qcGraph->graph(index)->setPen(clr);
            ui->qcGraph->graph(index)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
        }
    } else {
        m_graphIndex.append({ clr, 0 });
        ui->qcGraph->graph(index)->setPen(clr);
        ui->qcGraph->graph(index)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
    }

    //qDebug() << "g:" << m_index << " c:" << clr << " idx:" << index;
    if (ui->qcGraph->graph(index)->data()->size() > (m_maxPointNr - 1))
        ui->qcGraph->graph(index)->data()->remove(m_graphIndex.at(index).currIdx);
    ui->qcGraph->graph(index)->addData(m_graphIndex.at(index).currIdx, y);

    if (!m_bPause) {
        if (m_graphIndex.at(index).currIdx > (m_x_min + m_xWindow))
            ui->qcGraph->xAxis->setRange(m_graphIndex.at(index).currIdx - m_xWindow, m_graphIndex.at(index).currIdx);
        else {
            double x_min = ui->qcGraph->xAxis->range().lower;
            if (m_graphIndex.at(index).currIdx < x_min) {
                if (m_graphIndex.at(index).currIdx - m_xWindow >= 0)
                    ui->qcGraph->xAxis->setRange(m_graphIndex.at(index).currIdx - m_xWindow, m_graphIndex.at(index).currIdx);
                else
                    ui->qcGraph->xAxis->setRange(0, m_xWindow);
            }
        }
    }

    if (replot) ui->qcGraph->replot();
    ++m_graphIndex[index].currIdx %= m_maxPointNr;
}

void ASGraph::replot()
{
    ui->qcGraph->replot();
}

void ASGraph::on_ASGraph_rejected()
{
    qDebug() << "CLOSE";
}

int ASGraph::index() const
{
    return m_index;
}

void ASGraph::on_cbLineStyles_currentIndexChanged(int index)
{
    QMutexLocker lock(&m_mutex);
    for (int i = 0; i < m_graphIndex.count(); i++) {
        ui->qcGraph->graph(i)->setLineStyle(static_cast<QCPGraph::LineStyle>(index));
        if (index == 0) {
            ui->qcGraph->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 3));
        } else {
            ui->qcGraph->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone));
        }
    }
    ui->qcGraph->replot();
}

void ASGraph::on_pbResetZoom_clicked()
{
    QMutexLocker lock(&m_mutex);
    if (m_graphIndex.count()) {
        ui->qcGraph->graph(0)->rescaleAxes();
        for (int i = 0; i < m_graphIndex.count(); i++) {
            ui->qcGraph->graph(i)->rescaleAxes(true);
        }
        ui->qcGraph->replot();
    }
}

void ASGraph::on_pbSet_clicked()
{
    QMutexLocker lock(&m_mutex);
    int val = ui->sbItemCount->value();
    if (val < m_maxPointNr) {
        for (int i = 0; i < m_graphIndex.count(); i++) {
            ui->qcGraph->graph(i)->data()->removeAfter(val - 0.001);
            m_graphIndex[i].currIdx = 0;
        }
        ui->qcGraph->replot();
        QSettings s("jjterm.ini", QSettings::IniFormat);
        s.setValue(QString("SAMPLES%1").arg(m_index), val);
    }
    m_maxPointNr = val;
}

void ASGraph::on_pbClear_clicked()
{
    QMutexLocker lock(&m_mutex);
    for (int i = 0; i < m_graphIndex.count(); i++) {
        ui->qcGraph->graph(i)->data()->clear();
        m_graphIndex[i].currIdx = 0;
    }
    ui->qcGraph->replot();
}

void ASGraph::on_pbSetXY_clicked()
{
    QMutexLocker lock(&m_mutex);
    QSettings s("jjterm.ini", QSettings::IniFormat);
    m_x_min = ui->sbXFrom->value();
    m_x_max = ui->sbXTo->value();
    m_xWindow = m_x_max - m_x_min;
    s.setValue(QString("XMIN%1").arg(m_index), m_x_min);
    s.setValue(QString("XMAX%1").arg(m_index), m_x_max);
    s.setValue(QString("YMIN%1").arg(m_index), ui->sbYFrom->value());
    s.setValue(QString("YMAX%1").arg(m_index), ui->sbYTo->value());
    ui->qcGraph->xAxis->setRange(m_x_min, m_x_max);
    ui->qcGraph->yAxis->setRange(ui->sbYFrom->value(), ui->sbYTo->value());
    ui->qcGraph->replot();
}

void ASGraph::on_pbPause_clicked()
{
    m_bPause = !m_bPause;
    ui->pbPause->setText(m_bPause ? "Scroll" : "Pause");
}
