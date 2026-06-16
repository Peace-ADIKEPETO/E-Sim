#ifndef NGSPICEINTERFACE_H
#define NGSPICEINTERFACE_H

#include <QObject>

#include <QLibrary>
#include <QMap>
#include <QVector>

extern "C" {

#include <ngspice/sharedspice.h>

}

struct SimulationData {
    QString plotName;
    QStringList vectorNames;
    QMap<QString, QVector<double>> vectors; // vectorName → data points
    bool running = false;
};


class ngspiceinterface : public QObject
{
    Q_OBJECT

public:
    explicit ngspiceinterface(QObject *parent = nullptr);
    ~ngspiceinterface();

    bool initialize();
    void sendCommand(const QString &command);
    bool isInitialized() const { return m_initialized; }

    QString dbg_msg;

    SimulationData getLastSimulationData() const { return m_simData; }

signals:
    void outputReceived(const QString &text);
    void errorReceived(const QString &error);
    void SimulationDataReady();

private:
    // Static callback functions for ngspice C API
    static int sendChar(char *output, int id, void *user);
    static int sendStat(char *output, int id, void *user);
    static int controlledExit(int status, bool immediate, bool exitOnQuit, int id, void *user);
    static int sendData(pvecvaluesall vdata, int numvecs, int id, void *user);
    static int sendInitData(pvecinfoall intdata, int id, void *user);
    static int bgThreadRunning(bool isRunning, int id, void *user);


    // Instance method for processing callbacks
    int processSendChar(char *output);
    int processSendStat(char *output);
    int processControlledExit(int status, bool immediate, bool exitOnQuit);
    int processSendData(pvecvaluesall vdata, int numvecs);
    int processSendInitData(pvecinfoall intdata);
    int processBgThreadRunning(bool isRunning);

    // Function pointers for ngspice DLL
    using NgSpice_InitFunc = int(*)(SendChar*, SendStat*, ControlledExit*,
                                     SendData*, SendInitData*, BGThreadRunning*, void*);
    using NgSpice_CommandFunc = int(*)(char*);

    NgSpice_InitFunc m_ngSpice_Init;
    NgSpice_CommandFunc m_ngSpice_Command;


    QLibrary m_library;
    bool m_initialized;

    SimulationData m_simData;

    // Static pointer to instance for callback routing
    static ngspiceinterface *s_instance;
};

#endif // NGSPICEINTERFACE_H

