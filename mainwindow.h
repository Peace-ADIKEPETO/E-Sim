#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QtCharts>
#include <QtCharts/QValueAxis>

#include "ngspiceinterface.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void onRunDCOp();
    void onRunTransient();

private slots:
    void onRunSim();
    void onNgspiceOutput(const QString &text);
    void onSimulationDataReady();

private:
    Ui::MainWindow *ui;
    ngspiceinterface *m_ngspice;

    // Chart components
    QChartView *m_chartView;
    QChart *m_chart;
};
#endif // MAINWINDOW_H
