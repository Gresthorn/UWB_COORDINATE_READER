#include "server_pipe.h"

server_pipe::server_pipe(class MainWindow * parent, double time_break, QStringList * list, bool * statusT, bool * pausedT, QMutex * statusMutexT, QMutex * pausedMutexT, bool comport_enabled, int port_number)
    : QThread(parent)
{
    status = statusT;
    paused = pausedT;
    fileList = list;
    timeInterval = time_break*1000.0;
    statusMutex = statusMutexT;
    pausedMutex = pausedMutexT;
    RS232Mutex = new QMutex;
    parentWindow = parent;

    comport = comport_enabled;
    port = port_number;

    timer = new QTimer(this->parent());
    iterator = 0;

    //open files
    openedFileList = new QList <QTextStream * >;
    Q_FOREACH(QString str, *fileList)
    {
        QFile * f = new QFile(str);
        if(f->open(QFile::ReadOnly | QFile::Text))
        {
            QTextStream * input = new QTextStream(f);
            openedFileList->append(input);
        }
        else
        {
            QMessageBox::critical(parent, tr("Error opening file"), tr("The following file could not be opened: %1").arg(f->fileName()), QMessageBox::Ok);
            emit this->fileNotOpened(f->fileName());
            break;
        }
    }

    packetHandlerList = new QList<uwbPacketTx * >;

    if(comport)
    {
        // if sending data via comport, each radar unit (represented by single file) must have
        // own packet handler

        for(int i=0; i<openedFileList->count(); i++)
        {
            QString line = openedFileList->at(i)->readLine();
            QStringList firstLineSplitted = line.split(QChar('#'));
            // file check was done already before
            int radarId = firstLineSplitted.at(0).toInt();

            packetHandlerList->append(new uwbPacketTx(radarId, port));

            // return file pointer back to zero
            openedFileList->at(i)->seek(0);
        }
    }
}

int server_pipe::closeRS232port()
{
    RS232Mutex->lock();

        RS232_CloseComport(port);

    RS232Mutex->unlock();
}

server_pipe::~server_pipe()
{
    Q_FOREACH(QTextStream * f, *openedFileList)
    {
        f->device()->close();
    }
    openedFileList->clear();
    delete openedFileList;

    delete timer;

    closePipeConnection(pipeConnection);
}

void server_pipe::run()
{
    bool allFilesFinished;

    pipeConnection = NULL;


    if(!comport)
    {
        pipeConnection = createPipeConnection(waitTimeConstant);
        if(pipeConnection==NULL) return;
    }
    else
    {
        RS232Mutex->lock();

        if(RS232_OpenComport(port, 9600, "8N1") > 0)
        {
             RS232Mutex->unlock();
             emit this->comportCouldNotBeOpened();
             while(1)
             {
                 // wait until upper functions will terminate this thread
                 Sleep(5000);
                 continue;
             }
        }
        RS232Mutex->unlock();
    }

    while(1)
    {
        pausedMutex->lock();
                if(*paused)
                {
                    Sleep(500);
                    pausedMutex->unlock();
                    continue;
                }
        pausedMutex->unlock();

        statusMutex->lock();
                if(!(*status)) {
                    return;
                }
        statusMutex->unlock();

        allFilesFinished = true;
        bool success = false;
        int index = 0;

        Q_FOREACH(QTextStream * input, *openedFileList)
        {
            if(!input->atEnd())
            {
                allFilesFinished = false;
                QString line = input->readLine();

                if(!comport)
                {
                    // windows pipe communication
                    char * ln = new char[line.length()+1];
                    strcpy(ln, line.toStdString().c_str());
                    ln[line.length()] = '\0';

                    success = sendToClient(pipeConnection, ln);
                }
                else
                {
                    // send data via comport
                    success = true;
                    QStringList list = line.split(QChar('#'));
                    int dataCount = (list.count()-3)/2;

                    float * temp_array = new float[dataCount];
                    for(int i=1; i<=(dataCount/2); i++)
                    {
                        temp_array[i-1] = list.at(i*4-1).toFloat();
                        temp_array[i] = list.at(i*4).toFloat();
                    }

                    packetHandlerList->at(index)->generatePacket(temp_array, dataCount);
                    // since packet handlers uses com ports, we need to lock mutex
                    RS232Mutex->lock();
                    success = !packetHandlerList->at(index)->sendPacket();
                    RS232Mutex->unlock();

                    delete temp_array;
                }

                if(!success)
                {
                    emit this->clientDisconnected();
                    return;
                }
            }

            index++;
        }

        emit this->updateProgressBarSignal(iterator++);
        if(allFilesFinished)
        {
            emit this->allFilesFinished();
            break;
        }

        Sleep(timeInterval);
    }

}

// PIPE FUNCTIONS ----------------------------------------------------------------------------------
HANDLE server_pipe::createPipeConnection(int waitTime)
{
    // CREATING PIPE
    HANDLE connection = CreateNamedPipeA("\\\\.\\pipe\\uwb_pipe",
                                         PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
                                         PIPE_TYPE_BYTE,
                                         1, 0, 0, 0, NULL);

    if(connection == NULL || connection == INVALID_HANDLE_VALUE)
    {
        emit this->pipeCreationFailure();
        return NULL;
    }

    // PIPE IS CREATED, WAITING FOR CLIENT

    emit this->creatingEvents();
    OVERLAPPED ov;
    ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    ConnectNamedPipe(connection, &ov);

    emit this->waitingForClient();
    if(WaitForSingleObject(ov.hEvent, waitTime)==WAIT_OBJECT_0)
    {
        emit this->clientConnected();
        return connection;
    } else {
        // CLIENT NOT RESPONDING OR SOMETHING WENT WRONG
        emit this->clientNotResponding();
        CloseHandle(connection);
        return NULL;
    }

    return NULL;
}

void server_pipe::closePipeConnection(HANDLE connection)
{
    CloseHandle(connection);
}

bool server_pipe::sendToClient(HANDLE connection, char * data)
{
    bool result;
    DWORD bytesSent = 0;
    result = WriteFile(connection,
                       data,
                       (strlen(data)+1)*sizeof(char),
                       &bytesSent, NULL);
    return result;
}
