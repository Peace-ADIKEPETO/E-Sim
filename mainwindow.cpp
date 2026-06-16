#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Circuit Simulator - Week 1");

    // Output console
    ui->m_outputConsole->setReadOnly(true);
    ui->m_outputConsole->setFont(QFont("Courier", 10));

    // Initialize ngspice
    m_ngspice = new ngspiceinterface(this);

    // Chart for plot
    m_chart = new QChart();
    m_chart->setTitle("Voltage vs Time");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    ui->centralwidget->layout()->addWidget(m_chartView);

    // Connect ngspice output to our console
    connect(m_ngspice, &ngspiceinterface::outputReceived,
            this, &MainWindow::onNgspiceOutput);

    connect(m_ngspice, &::ngspiceinterface::SimulationDataReady,
            this, &MainWindow::onSimulationDataReady);

    // Connect button
    connect(ui->m_RunBouton, &QPushButton::clicked,
            this, &MainWindow::onRunSim);

    // Initialize the library
    if (!m_ngspice->initialize()) {
        ui->m_outputConsole->append(m_ngspice->dbg_msg);
        ui->m_RunBouton->setEnabled(false);
    } else {
        ui->m_outputConsole->append(m_ngspice->dbg_msg);
    }

}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_ngspice;
}

void MainWindow::onRunDCOp()
{
        ui->m_outputConsole->clear();
        ui->m_outputConsole->append("=== Running DC Operating Point ===\n");

        m_ngspice->sendCommand("remcirc");

       // m_ngspice->sendNetlist({"V1 1 0 .dc 5\n", "R1 1 0 1000\n", ".end"});

       // ui->m_outputConsole->append(m_ngspice->dbg_msg);

        m_ngspice->sendCommand("source C:/Users/HP/Documents/progC++/Qt/ngspiceinterface/test_netlist.sp");

        m_ngspice->sendCommand("op");

        m_ngspice->sendCommand("print v(1)");      // Node 1 voltage
        m_ngspice->sendCommand("print i(V1)");
}

void MainWindow::onRunTransient()
{
    ui->m_outputConsole->clear();
    ui->m_outputConsole->append("=== Running Transient Simulation ===\n");

    // Clear previous circuit

    m_ngspice->sendCommand("remcirc");

    // Run transient: 1µs step, 2ms total → 2000 data points
    m_ngspice->sendCommand("source C:/Users/HP/Documents/progC++/Qt/ngspiceinterface/test_netlist.sp");

    //m_ngspice->sendCommand("tran 1u 2m");

    m_ngspice->sendCommand("bg_run");

    // This will trigger SendInitData, then repeated SendData calls,
    // then finally simulationDataReady signal from ControlledExit
}

void MainWindow::onRunSim()
{
    if (ui->simMode->currentText() == QString("DC Point"))
        this->onRunDCOp();
    else if(ui->simMode->currentText() == QString("Transient"))
        this->onRunTransient();
}

void MainWindow::onNgspiceOutput(const QString &text)
{
    ui->m_outputConsole->append(text);
}

void MainWindow::onSimulationDataReady()
{
    SimulationData data = m_ngspice->getLastSimulationData();

    ui->m_outputConsole->append("\n=== Extracted Data ===");
    ui->m_outputConsole->append(QString("Plot: %1").arg(data.plotName));
    ui->m_outputConsole->append(QString("Vectors: %1").arg(data.vectorNames.join(", ")));

    // ─── PLOT THE DATA ───────────────────────
    m_chart->removeAllSeries();
    // Remove old axes
    const auto axes = m_chart->axes();
    for (QAbstractAxis *axis : axes) {
        m_chart->removeAxis(axis);
    }

    // We need 'time' and 'v(2)' vectors
    QString timeVecName;
    QString voltageVecName;

    // ngspice typically names time vector "time" and node voltages "v(2)"
    for (const QString &name : data.vectorNames) {
        if (name.toLower() == "time") timeVecName = name;
        if (name.toLower() == "v(2)") voltageVecName = name;
    }

    if (timeVecName.isEmpty() || voltageVecName.isEmpty()) {
        ui->m_outputConsole->append("ERROR: Could not find time or voltage vectors!");
        ui->m_outputConsole->append("Available vectors: " + data.vectorNames.join(", "));
        return;
    }

    const QVector<double> &timeData = data.vectors["time"];
    const QVector<double> &voltData = data.vectors["V(2)"];
    ui->m_outputConsole->append(QString("Data points: %1").arg(timeData.size()));

    // Create line series
    QLineSeries *series = new QLineSeries();
    series->setName("V(2) - Capacitor Voltage");

    for (int i = 0; i < qMin(timeData.size(), voltData.size()); i++) {
        series->append(timeData[i] * 1000, voltData[i]);  // Convert time to ms
    }

    m_chart->addSeries(series);
    // Create axes
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Time (ms)");
    axisX->setLabelFormat("%.2f");
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Voltage (V)");
    axisY->setLabelFormat("%.2f");
    axisY->setRange(0, 6);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    ui->m_outputConsole->append("Plot updated successfully.");
}