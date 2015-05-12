#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QInputDialog>
#include <QThread>
#include <QMutex>
#include <QRegExp>
#include <QtSerialPort/QSerialPortInfo>

#include "server_pipe.h"
#include "uwbpacketclass.h"
#include "rs232.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // writing into logger function
    void log_message(QString message);

private:
    Ui::MainWindow *ui;

    int port_number;

    int number_of_last_records;
    double time_step_interval;
    QStringList * fileList;

    QThread * threadHandle;

    QMutex * transmittingStatusMutex;
    bool * transmittingStatus;
    QMutex * pausedMutex;
    bool * paused;
    class server_pipe * transmitter;

    // file analyse
    bool file_analyzer(QString file_path);
    void timeStepIntervalManualEnter(void);

signals:
    // signal to start user input
    void startUserInputSignal(void);
public slots:
    void startUserInputSlot(void);
    void timeStepIntervalSlot(void);
    void transmitterSlot(void);
    void updateProgressBar(int);

    void allFilesFinishedSlot(void);
    void fileNotOpened(QString);

    void pipeCreationFailureSlot(void);
    void creatingEventsSlot(void);
    void waitingForClientSlot(void);
    void clientConnectedSlot(void);
    void clientNotRespondingSlot(void);
    void clientDisconnectedSlot(void);

    void comportCouldNotBeOpened(void);

};


#endif // MAINWINDOW_H
