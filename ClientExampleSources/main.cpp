#include <QCoreApplication>
#include <QDebug>
#include <windows.h>

void closeConnection(HANDLE connection);
HANDLE connectToPipe(void);
bool readFromPipe(HANDLE connection, char * buffer);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    char buffer[512];

    HANDLE connection = connectToPipe();
    if(connection!=NULL)
    {
        while(readFromPipe(connection, buffer)) qDebug() << buffer;
        closeConnection(connection);
    }
    else qDebug() << "A problem occured when trying to contact server";


    return 0;

    return a.exec();
}

HANDLE connectToPipe(void)
{
    HANDLE connection = CreateFileA("\\\\.\\pipe\\uwb_pipe",
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

    if(connection!=INVALID_HANDLE_VALUE) return connection;
    else return NULL;
}

bool readFromPipe(HANDLE connection, char * buffer)
{
    DWORD bytesRecieved = 0;
    bool pipe_control;

    pipe_control = ReadFile(
                connection, buffer,
                511*sizeof(char),
                &bytesRecieved, NULL);

    if(!pipe_control) closeConnection(connection);

    return pipe_control;
}

void closeConnection(HANDLE connection)
{
    CloseHandle(connection);
}
