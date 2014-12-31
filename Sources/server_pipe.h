#ifndef SERVER_PIPE_H
#define SERVER_PIPE_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QFile>
#include <windows.h>

#include "mainwindow.h"

class server_pipe : public QThread
{
    Q_OBJECT

public:
    server_pipe(class MainWindow * parent, double time_break, QStringList * list, bool * statusT, bool * pausedT, QMutex * statusMutexT, QMutex * pausedMutexT);
    ~server_pipe();

private:
    const int waitTimeConstant = 15000;

    bool * status;
    bool * paused;

    double timeInterval;
    QTimer * timer;

    QMutex * statusMutex;
    QMutex * pausedMutex;

    int iterator;

    HANDLE pipeConnection;

    QStringList * fileList;
    QList <QTextStream * > * openedFileList;
    class MainWindow * parentWindow;

    HANDLE createPipeConnection(int waitTime);
    void closePipeConnection(HANDLE connection);
    bool sendToClient(HANDLE connection, char * data);

public slots:
signals:
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
