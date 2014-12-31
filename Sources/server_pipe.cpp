#include "server_pipe.h"

server_pipe::server_pipe(class MainWindow * parent, double time_break, QStringList * list, bool * statusT, bool * pausedT, QMutex * statusMutexT, QMutex * pausedMutexT)
    : QThread(parent)
{
    status = statusT;
    paused = pausedT;
    fileList = list;
    timeInterval = time_break*1000.0;
    statusMutex = statusMutexT;
    pausedMutex = pausedMutexT;
    parentWindow = parent;

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

    pipeConnection = createPipeConnection(waitTimeConstant);
    if(pipeConnection==NULL) return;

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
        Q_FOREACH(QTextStream * input, *openedFileList)
        {
            if(!input->atEnd())
            {
                allFilesFinished = false;
                QString line = input->readLine();
                char * ln = new char[line.length()+1];
                strcpy(ln, line.toStdString().c_str());
                ln[line.length()] = '\0';

                success = sendToClient(pipeConnection, ln);
                if(!success)
                {
                    emit this->clientDisconnected();
                    return;
                }
            }
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
