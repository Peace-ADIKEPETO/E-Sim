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
#include <QtCharts/QLogValueAxis>
#include <QCheckBox>
#include <QHBoxLayout>

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
    void onRunAC();

    void plotTransient(const SimulationData &data);
    void plotAC(const SimulationData &data);

private slots:
    void onRunSim();
    void onNgspiceOutput(const QString &text);
    void onSimulationDataReady();

    void showPhase();
    void showMag();

private:
    Ui::MainWindow *ui;
    ngspiceinterface *m_ngspice;

    // Toggle checkboxes
    QCheckBox *m_showMagCheck;
    QCheckBox *m_showPhaseCheck;

    // Chart components
    QChartView *m_chartView;
    QChart *m_chart;

    QChartView *m_phaseChartView; // phase Bode
    QChart *m_phaseChart;

    bool  visible = true;
};
#endif // MAINWINDOW_H
