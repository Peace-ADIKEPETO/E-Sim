#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("E-Sim - Week 3");

    // Output console
    ui->m_outputConsole->setReadOnly(true);
    ui->m_outputConsole->setFont(QFont("Courier", 10));

    // Initialize ngspice
    m_ngspice = new ngspiceinterface(this);

    // ─── AC chart controls ───────────────────
    m_showMagCheck = new QCheckBox("Show Magnitude (dB)");
    m_showMagCheck->setChecked(true);
    m_showPhaseCheck = new QCheckBox("Show Phase (°)");
    m_showPhaseCheck->setChecked(true);

    // Chart for plot
    m_chart = new QChart();
    m_chart->setTitle("Voltage vs Time");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);

    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);


    // ─── AC Phase chart ──────────────────────
    m_phaseChart = new QChart();
    m_phaseChart->setTitle("Phase (°)");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);

    m_phaseChartView = new QChartView(m_phaseChart);
    m_phaseChartView->setRenderHint(QPainter::Antialiasing);


    ui->centralwidget->layout()->addWidget(m_phaseChartView);
    ui->centralwidget->layout()->addWidget(m_chartView);


    ui->centralwidget->layout()->addWidget(m_showMagCheck);
    ui->centralwidget->layout()->addWidget(m_showPhaseCheck);



    // Connect ngspice output to our console
    connect(m_ngspice, &ngspiceinterface::outputReceived,
            this, &MainWindow::onNgspiceOutput);

    connect(m_ngspice, &::ngspiceinterface::SimulationDataReady,
            this, &MainWindow::onSimulationDataReady);

    // Connect button
    connect(ui->m_RunBouton, &QPushButton::clicked,
            this, &MainWindow::onRunSim);

    connect(m_showMagCheck, &QCheckBox::clicked, this, &MainWindow::showMag);

    connect(m_showPhaseCheck, &QCheckBox::clicked, this, &MainWindow::showPhase);

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

void MainWindow::onRunAC()
{
        ui->m_outputConsole->clear();
        ui->m_outputConsole->append("=== Running AC Sweep ===\n");

        m_ngspice->sendCommand("remcirc");

        m_ngspice->sendCommand("source C:/Users/HP/Documents/progC++/Qt/ngspiceinterface/test.sp");

        m_ngspice->sendCommand("bg_run");  // triggers callbacks
}

void MainWindow::plotTransient(const SimulationData &data)
{
    // We need 'time' and 'v(2)' vectors
    QString timeVecName;
    QString voltageVecName;

    m_chart->removeAllSeries();
    const auto Axes = m_chart->axes();
    for (QAbstractAxis *axis : Axes) {
        m_chart->removeAxis(axis);
    }

    m_chart->setTitle("Transient Analysis");

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

void MainWindow::plotAC(const SimulationData &data)
{
    // We need 'time' and 'v(2)' vectors
    QString FreqVecName;
    QString voltageVecName;

    m_chart->removeAllSeries();
    const auto Axes = m_chart->axes();
    for (QAbstractAxis *axis : Axes) {
        m_chart->removeAxis(axis);
    }

    m_chart->setTitle("AC Sweep Analysis");

    // ngspice typically names time vector "time" and node voltages "v(2)"
    for (const QString &name : data.vectorNames) {
        if (name.toLower() == "frequency") FreqVecName = name;
        if (name.toLower() == "v(2)") voltageVecName = name;
    }

    if (FreqVecName.isEmpty() || voltageVecName.isEmpty()) {
        ui->m_outputConsole->append("ERROR: Could not find time or voltage vectors!");
        ui->m_outputConsole->append("Available vectors: " + data.vectorNames.join(", "));
        return;
    }
        const QVector<double> &freq = data.vectors["frequency"];
        const QVector<double> &v2 = data.vectors["V(2)"];
        const QVector<double> &Ph_v2 = data.Phasevectors["V(2)"];

        // Convert magnitude to dB: 20*log10(Vout/Vin), Vin=1 -> dB = 20*log10(Vout)
        QLineSeries *magSeries = new QLineSeries();
        magSeries->setName("V(2) Magnitude (dB)");
        magSeries->setPen(QPen(Qt::blue, 2));

        for (int i = 0; i < qMin(freq.size(), v2.size()); i++) {
            double db = 20.0 * std::log10(v2[i]);
            if (db < -200) db = -200;
            magSeries->append(freq[i], db);
        }

        m_chart->addSeries(magSeries);

        QLogValueAxis *axisX = new QLogValueAxis();
        axisX->setTitleText("Frequency (Hz)");
        axisX->setLabelFormat("%.0f");
        axisX->setBase(10);
        axisX->setRange(1, 100000);
        axisX->setMinorTickCount(8);
        m_chart->addAxis(axisX, Qt::AlignBottom);
        magSeries->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        axisY->setTitleText("Magnitude (dB)");
        axisX->setLabelFormat("%.1f");
        axisY->setRange(-40, 5);
        m_chart->addAxis(axisY, Qt::AlignLeft);
        magSeries->attachAxis(axisY);


        // ─── Phase chart ─────────────────────────
        m_phaseChart->removeAllSeries();
        const auto phaseAxes = m_phaseChart->axes();
        for (QAbstractAxis *axis : phaseAxes) {
            m_phaseChart->removeAxis(axis);
        }

        QLineSeries *phaseSeries = new QLineSeries();
        phaseSeries->setName("V(2) Phase (°)");
        phaseSeries->setPen(QPen(Qt::red, 2));

        for (int i = 0; i < qMin(freq.size(), Ph_v2.size()); i++) {
            phaseSeries->append(freq[i], Ph_v2[i]);
        }
        m_phaseChart->addSeries(phaseSeries);

        QLogValueAxis *axisX_phase = new QLogValueAxis();
        axisX_phase->setTitleText("Frequency (Hz)");
        axisX_phase->setLabelFormat("%.0f");
        axisX_phase->setBase(10);
        axisX_phase->setRange(1, 100000);
        axisX_phase->setMinorTickCount(8);

        // ─── Right Y-axis: Phase (°) ──────────────────
        QValueAxis *axisY_phase = new QValueAxis();
        axisY_phase->setTitleText("Phase (°)");
        axisY_phase->setRange(-180, 0);      // typical RC phase range
        axisY_phase->setLabelFormat("%.0f");
        m_phaseChart->addAxis(axisX_phase, Qt::AlignBottom);
        m_phaseChart->addAxis(axisY_phase, Qt::AlignLeft);
        phaseSeries->attachAxis(axisX_phase);
        phaseSeries->attachAxis(axisY_phase);

        ui->m_outputConsole->append(QString("Data points: %1").arg(freq.size()));
        ui->m_outputConsole->append("AC plot updated.");

}

void MainWindow::onRunSim()
{
    if (ui->simMode->currentText() == QString("DC Point"))
        this->onRunDCOp();
    else if(ui->simMode->currentText() == QString("Transient"))
        this->onRunTransient();
    else if(ui->simMode->currentText() == QString("AC Sweep"))
        this->onRunAC();
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

    if (data.vectors.contains("frequency")) {
        // AC analysis
        plotAC(data);
    } else if (data.vectors.contains("time")) {
        // Transient analysis (unchanged)
        plotTransient(data);
    } else {
        ui->m_outputConsole->append("ERROR: Unknown simulation type.");
    }
}

void MainWindow::showPhase()
{
  m_phaseChartView->setVisible(visible);
    visible = !visible;
}

void MainWindow::showMag()
{
 m_chartView->setVisible(visible);
    visible = !visible;
}