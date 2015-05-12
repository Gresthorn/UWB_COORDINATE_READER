#ifndef SERVER_PIPE_H
#define SERVER_PIPE_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QFile>
#include <windows.h>
#include <QMutex>
#include <QMessageBox>

#include "mainwindow.h"
#include "rs232.h"
#include "uwbpacketclass.h"

class server_pipe : public QThread
{
    Q_OBJECT

public:
    server_pipe(class MainWindow * parent, double time_break, QStringList * list, bool * statusT, bool * pausedT, QMutex * statusMutexT, QMutex * pausedMutexT, bool comport_enabled = false, int port_number = 0);
    int closeRS232port(void);
    ~server_pipe();

private:
    const int waitTimeConstant = 15000;


    bool comport;
    int port;

    bool * status;
    bool * paused;

    double timeInterval;
    QTimer * timer;

    QMutex * statusMutex;
    QMutex * pausedMutex;
    QMutex * RS232Mutex;

    int iterator;

    HANDLE pipeConnection;

    QStringList * fileList;
    QList <QTextStream * > * openedFileList;
    QList <uwbPacketTx * > * packetHandlerList;
    class MainWindow * parentWindow;

    HANDLE createPipeConnection(int waitTime);
    void closePipeConnection(HANDLE connection);
    bool sendToClient(HANDLE connection, char * data);

public slots:
signals:
    void comportCouldNotBeOpened(void);
    void updateProgressBarSignal(int);
    void allFilesFinished(void);

    void fileNotOpened(QString);

    void pipeCreationFailure(void);
    void creatingEvents(void);
    void waitingForClient(void);
    void clientConnected(void);
    void clientNotResponding(void);
    void clientDisconnected(void);

protected:
    void run();
};

#endif // SERVER_PIPE_H
