#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if( w.isValid() ) w.show();
    else return -1;

    return a.exec();
}
