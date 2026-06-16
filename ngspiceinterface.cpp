#include "ngspiceinterface.h"
#include <QDebug>
#include <cstring>

// Initialize static instance pointer
ngspiceinterface *ngspiceinterface::s_instance = nullptr;

ngspiceinterface::ngspiceinterface(QObject *parent)
    : QObject(parent)
    , m_ngSpice_Init(nullptr)
    , m_ngSpice_Command(nullptr)
    , m_initialized(false)
{
}

ngspiceinterface::~ngspiceinterface()
{
    if (m_library.isLoaded()) {
        m_library.unload();
    }
    s_instance = nullptr;
}

bool ngspiceinterface::initialize()
{
    s_instance = this;

    // Load the shared library
    m_library.setFileName("ngspice");

    if (!m_library.load()) {
        qDebug() << "Failed to load ngspice.dll:" << m_library.errorString();
        dbg_msg = m_library.errorString();
        return false;
    }

    // Resolve function pointers
    m_ngSpice_Init = (NgSpice_InitFunc)m_library.resolve("ngSpice_Init");
    m_ngSpice_Command = (NgSpice_CommandFunc)m_library.resolve("ngSpice_Command");

    if (!m_ngSpice_Init || !m_ngSpice_Command) {
        qCritical() << "Failed to resolve ngspice functions";
        dbg_msg = "Failed to resolve ngspice functions";
        m_library.unload();
        return false;
    }

    // Initialize ngspice with callbacks
    int ret = m_ngSpice_Init(sendChar, sendStat, controlledExit,
                            sendData, sendInitData, bgThreadRunning, this);

    if (ret != 0) {
        qCritical() << "ngSpice_Init failed with code:" << ret;
        m_library.unload();
        dbg_msg = "ngSpice_Init failed with code:";
        return false;
    }

    m_initialized = true;
    qDebug() << "ngspice initialized successfully";
    dbg_msg = "ngspice initialized successfully";
    return true;
}

void ngspiceinterface::sendCommand(const QString &command)
{
    if (!m_initialized || !m_ngSpice_Command) {
        emit errorReceived("ngspice not initialized");
        return;
    }

    QByteArray cmdBytes = command.toLocal8Bit();
    char *cmd = cmdBytes.data();

    qDebug() << "Sending command:" << command;
    int ret = m_ngSpice_Command(cmd);

    if (ret != 0) {
        emit errorReceived(QString("Command failed: %1 (error %2)").arg(command).arg(ret));
    }
}

// Static callback implementations
int ngspiceinterface::sendChar(char *output, int id, void *user)
{
    Q_UNUSED(id)
    ngspiceinterface *instance = static_cast<ngspiceinterface*>(user);
    if (!instance) instance = s_instance;
    if (instance) return instance->processSendChar(output);
    return 0;
}

int ngspiceinterface::sendStat(char *output, int id, void *user)
{
    Q_UNUSED(id)
    ngspiceinterface *instance = static_cast<ngspiceinterface*>(user);
    if (!instance) instance = s_instance;
    if (instance) return instance->processSendStat(output);
    return 0;
}
int ngspiceinterface::controlledExit(int status, bool immediate,
                                     bool exitOnQuit, int id, void *user)
{
    Q_UNUSED(id)
    ngspiceinterface *instance = static_cast<ngspiceinterface*>(user);
    if (!instance) instance = s_instance;
    if (instance) return instance->processControlledExit(status, immediate, exitOnQuit);
    return 0;
}

int ngspiceinterface::sendData(pvecvaluesall vdata, int numvecs, int id, void *user)
{
    Q_UNUSED(id)
    ngspiceinterface *instance = static_cast<ngspiceinterface*>(user);
    if (!instance) instance = s_instance;
    if (instance) return instance->processSendData(vdata, numvecs);
    return 0;
}

int ngspiceinterface::sendInitData(pvecinfoall intdata, int id, void *user)
{
    Q_UNUSED(id)
    ngspiceinterface *instance = static_cast<ngspiceinterface*>(user);
    if (!instance) instance = s_instance;
    if (instance) return instance->processSendInitData(intdata);
    return 0;
}

int ngspiceinterface::bgThreadRunning(bool isRunning, int id, void *user)
{
    Q_UNUSED(id)
    qDebug() << "CALLBACK: bgThreadRunning - isRunning:" << isRunning;
    ngspiceinterface *instance = static_cast<ngspiceinterface*>(user);
    if (!instance) instance = s_instance;
    if (instance) return instance->processBgThreadRunning(isRunning);
    return 0;
}

// Instance callback processors
int ngspiceinterface::processSendChar(char *output)
{
    if (output) {
        QString text = QString::fromLocal8Bit(output);
        emit outputReceived(text);
    }
    return 0;
}

int ngspiceinterface::processSendStat(char *output)
{
    if (output) {
        QString text = QString::fromLocal8Bit(output);
        emit outputReceived(text);
    }
    return 0;
}
int ngspiceinterface::processControlledExit(int status, bool immediate, bool exitOnQuit)
{
    qDebug() << "ngspice exit - status:" << status
             << "immediate:" << immediate
             << "exitOnQuit:" << exitOnQuit;
    emit outputReceived(QString("\n=== Simulation finished (status: %1) ===").arg(status));
    emit SimulationDataReady();
    return 0;
}

int ngspiceinterface::processSendData(pvecvaluesall vdata, int numvecs)
{
    Q_UNUSED(numvecs)

    // Called repeatedly with time steps. Appends data to vectors.
    if (!vdata || !vdata->vecsa) return 0;
    pvecvalues vec; QString vecName; double value;

    for (int i = 0; i < vdata->veccount; i++) {
        vec = vdata->vecsa[i];
        if(!vec) continue;
        vecName = QString::fromLocal8Bit(vec->name);
        value = vec->creal;  // Real part of complex value
        //if (m_simData.vectors.contains(vecName)) {
            m_simData.vectors[vecName].append(value);
       // }
    }

    /*if (m_simData.time.size() % 10 == 0) {
        emit dataUpdated(m_simData.time, m_simData.vectors["v(2)"]);
    }*/

    return 0;
}

int ngspiceinterface::processSendInitData(pvecinfoall intdata)
{
    // Called once per simulation run. Contains vector metadata.
    m_simData = SimulationData();  // Clear previous data

    if (!intdata) return 0;

    m_simData.plotName = QString::fromLocal8Bit(intdata->name);

    // Extract vector names from linked list

    for (int i = 0; i < intdata->veccount; i++) {
        QString vecName = QString::fromLocal8Bit(intdata->vecs[i]->vecname);
        m_simData.vectorNames.append(vecName);
        m_simData.vectors[vecName] = QVector<double>();  // Initialize empty
    }

    qDebug() << "Init data received. Plot:" << m_simData.plotName
             << "Vectors:" << m_simData.vectorNames;

    return 0;
}

int ngspiceinterface::processBgThreadRunning(bool isRunning)
{
    qDebug() << "processBgThreadRunning:" << isRunning;
    if (isRunning) {
        // Background thread finished → data is ready
        emit SimulationDataReady();
    }
    return 0;
}