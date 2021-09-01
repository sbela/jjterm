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
#pragma once

#include "qcustomplot.h"
#include <QDebug>
#include <QDialog>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>

namespace Ui {
class ASGraph;
}

class ASGraph : public QDialog
{
    Q_OBJECT
    typedef struct tagINDEX {
        QColor clr;
        int currIdx;
    } INDEX;

public:
    explicit ASGraph(int index, QWidget* parent = nullptr);
    ~ASGraph();
    void addPoint(int x, int y, QColor clr);
    void addScatteredPoint(int y, QColor clr, bool replot = true);
    void replot();

    int index() const;

private slots:
    void on_ASGraph_rejected();

    void on_cbLineStyles_currentIndexChanged(int index);

    void on_pbResetZoom_clicked();

    void on_pbSet_clicked();

    void on_pbClear_clicked();

    void on_pbSetXY_clicked();

    void on_pbPause_clicked();

private:
    Ui::ASGraph *ui;
    int m_index { 0 };
    QList<INDEX> m_graphIndex;
    int m_maxPointNr { 10000 };
    int m_xWindow { 100 };
    bool m_bPause { false };
    QMutex m_mutex;
    int m_x_min;
    int m_x_max;
};

