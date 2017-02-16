#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->logger->setReadOnly(true);

    number_of_last_records = -1;
    ui->progress->setValue(0);
    fileList = new QStringList;
    time_step_interval = 0.0769;

    transmittingStatusMutex = new QMutex;
    transmittingStatus = new bool;
    *transmittingStatus = false;
    pausedMutex = new QMutex;
    paused = new bool;
    *paused = false;
    transmitter = NULL;

    // setting icons
    ui->newFilesButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/fileIcon.png'); background-repeat: none; border: none;}"
                                              "QPushButton:hover{background-image: url(':/main/icons/fileIconHover.png');}"));
    ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/startIcon.png'); background-repeat: none; border: none;}"
                                           "QPushButton:hover{background-image: url(':/main/icons/startIconHover.png');}"));
    ui->timeStepIntervalButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/timeIcon.png'); background-repeat: none; border: none;}"
                                                      "QPushButton:hover{background-image: url(':/main/icons/timeIconHover.png');}"));

    this->show();

    connect(ui->timeStepIntervalButton, SIGNAL(clicked()), this, SLOT(timeStepIntervalSlot()));
    connect(ui->newFilesButton, SIGNAL(clicked()), this, SLOT(startUserInputSlot()));
    connect(this, SIGNAL(startUserInputSignal()), this, SLOT(startUserInputSlot()));
    connect(ui->startButton, SIGNAL(clicked()), this, SLOT(transmitterSlot()));
    connect(this, SIGNAL(quit()), this, SLOT(close()));

    QList<QSerialPortInfo> list;
    list = QSerialPortInfo::availablePorts();

    if(list.isEmpty())
        ui->serialComboBox->addItem(tr("No COM ports availible"));
    else
    {
        for(int i=0; i<list.count(); i++)
        {
            ui->serialComboBox->addItem(list.at(i).portName());
        }
    }

    QStringList baudrates = (QStringList() << "300" << "600" << "1200" << "2400" << "4800" << "9600" << "19200" << "38400"
                                           << "57600" << "115200" << "128000" << "256000" << "500000" << "1000000" );
    ui->baudRate->addItems(baudrates);
    ui->baudRate->setCurrentIndex(5);

    QStringList modes = (QStringList() << "8N1" << "8O1" << "8E1" << "8N2" << "8O2" << "8E2" << "7N1" << "7O1" << "7E1" << "7N2"
                                       << "7E2" << "6N1" << "6O1" << "6E1" << "6N2" << "6O2" << "6E2" << "5N1" << "5O1" << "5E1"
                                       << "5N2" << "5O2" << "5E2");
    ui->serialMode->addItems(modes);

    initValid = this->startUserInputSlot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::startUserInputSlot()
{
    bool ok = false;
    while(!ok)
    {
        number_of_last_records = 0;
        // getting files
        *fileList = QFileDialog::getOpenFileNames(this, tr("Select coordinate containers"), "./", tr("Text files (*.txt)"));

        // no file selected, closing application
        if(fileList->empty()) break;

        Q_FOREACH(QString file, *fileList)
        {
            log_message(QString("Analyzing file: ").append(file));
            ok = file_analyzer(file);

            // if only one file is incorrect, stop process
            if(!ok)
            {
                QMessageBox::information(this, tr("Incorrect file format"), tr("Some files are corrupted or does not meet the required format.\n"
                                                                               "Please check the logs for more detailes. If everzthing seems to\n"
                                                                               "be correct, check the compatibility/program version or contact\n"
                                                                               "authors."), QMessageBox::Ok);
                log_message("File analyze failed: File is incorrect");
                break;
            }
        }
    }

    return ok;
}

void MainWindow::timeStepIntervalSlot()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Time step interval setup"), tr("Would you like to select a setup file?"), QMessageBox::Yes | QMessageBox::No);

    if(reply==QMessageBox::Yes)
    {
        // selecting a setup.txt file
        QFile setupFile(QFileDialog::getOpenFileName(this, tr("Select setup file"), tr("./"), tr("Text files (*.txt)")));
        if(!setupFile.open(QFile::ReadOnly | QFile::Text))
        {
            log_message("Error: Setup file could not be opened");
            QMessageBox::critical(this, tr("File could not be opened"), tr("The setup file could not be opened. Eneter the time step interval manually."), QMessageBox::Ok);

            // manual enter
            timeStepIntervalManualEnter();
        }
        else
        {
            // trying to find time step interval value
            QTextStream setupFileStream(&setupFile);
            while(!setupFileStream.atEnd())
            {
                QStringList line_parse = setupFileStream.readLine().split(QChar('#'));
                if(line_parse.at(0)==QString("SAMPLING_PERIOD"))
                {
                    // try to extract value
                    bool isNumber;
                    double temp = line_parse.at(1).toDouble(&isNumber);
                    if(isNumber)
                    {
                        // everything is OK
                        time_step_interval = temp;
                        log_message(QString("Time step interval set to %1 from setup file.").arg(time_step_interval));
                        setupFile.close();
                        return;
                    }
                    else
                    {
                        setupFile.close();

                        // no value found in file
                        log_message("Error: Time step interval identifier found but could not extract the value.");
                        QMessageBox::critical(this, tr("No value found in file"), tr("The setup file does containe required identifier but the value could not be extracted. Enter the value manually."), QMessageBox::Ok);

                        // manual enter
                        timeStepIntervalManualEnter();
                        return;
                    }
                }
                else if(line_parse.at(0).length()<=2) break; // means ! sign or error file
            }
            setupFile.close();

            // no value found in file
            log_message("Error: Time step interval value not found in file");
            QMessageBox::critical(this, tr("No value found in file"), tr("The setup file does not contained required value. Enter the value manually."), QMessageBox::Ok);

            // manual enter
            timeStepIntervalManualEnter();
        }

    }
    else timeStepIntervalManualEnter();
}

void MainWindow::transmitterSlot()
{
    pausedMutex->lock();
        if(*paused)
        {
            *paused = false;

            ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/pauseIcon.png'); background-repeat: none; border: none;}"
                                                   "QPushButton:hover{background-image: url(':/main/icons/pauseIconHover.png');}"));
            log_message("Continue transmitter thread");

            pausedMutex->unlock();

            return;
        }
    pausedMutex->unlock();

    transmittingStatusMutex->lock();
        if(!(*transmittingStatus))
        {
            log_message("Starting transmitting thread");

            *transmittingStatus = true;

            ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/pauseIcon.png'); background-repeat: none; border: none;}"
                                                   "QPushButton:hover{background-image: url(':/main/icons/pauseIconHover.png');}"));
            ui->timeStepIntervalButton->setDisabled(true);
            ui->newFilesButton->setDisabled(true);

            if(transmitter != NULL) { transmitter->quit(); delete transmitter; }
            ui->progress->setValue(0);


            if(!ui->serialCheckBox->isChecked())
            {
                transmitter = new server_pipe(this, time_step_interval, fileList, transmittingStatus, paused, transmittingStatusMutex, pausedMutex);
                connect(transmitter, SIGNAL(clientConnected()), this, SLOT(clientConnectedSlot()));
                connect(transmitter, SIGNAL(clientNotResponding()), this, SLOT(clientNotRespondingSlot()));
                connect(transmitter, SIGNAL(creatingEvents()), this, SLOT(creatingEventsSlot()));
                connect(transmitter, SIGNAL(pipeCreationFailure()), this, SLOT(pipeCreationFailureSlot()));
                connect(transmitter, SIGNAL(waitingForClient()), this, SLOT(waitingForClientSlot()));
                connect(transmitter, SIGNAL(clientDisconnected()), this, SLOT(clientDisconnectedSlot()));
            }
            else
            {
                QString comport_string(ui->serialComboBox->itemText(ui->serialComboBox->currentIndex()));
                QRegExp match_comport_number(QString("(\\d+)"));

                QStringList number_list;
                int pos = 0;

                while ((pos = match_comport_number.indexIn(comport_string, pos)) != -1) {
                    number_list << match_comport_number.cap(1);
                    pos += match_comport_number.matchedLength();
                }

                // now in number_list there are all numbers that are in comport name, we will extract the first one and convert it to integer

                int port;
                if(number_list.isEmpty()) port = -1;
                else port = number_list.at(0).toInt();

                bool to_int_ok = false;
                unsigned int baud_rate = ui->baudRate->itemText(ui->baudRate->currentIndex()).toUInt(&to_int_ok);
                if(!to_int_ok) baud_rate = 9600; //default baudrate

                QString serial_mode = ui->serialMode->itemText(ui->serialMode->currentIndex());

                transmitter = new server_pipe(this, time_step_interval, fileList, transmittingStatus, paused,
                                              transmittingStatusMutex, pausedMutex, true, port-1, baud_rate, serial_mode.toStdString().c_str());

                connect(transmitter, SIGNAL(comportCouldNotBeOpened()), this, SLOT(comportCouldNotBeOpened()));

                log_message(QString("Port with index: %1 was set up. If this index is not correct, modify port name to contain correct port number.").arg(port-1));
            }

            connect(transmitter, SIGNAL(updateProgressBarSignal(int)), this, SLOT(updateProgressBar(int)));
            connect(transmitter, SIGNAL(allFilesFinished()), this, SLOT(allFilesFinishedSlot()));

            transmitter->start();
        }
        else
        {
            QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Transmitter thread"), tr("Would you like just to pause the thread (click no to stop)?"), QMessageBox::Yes | QMessageBox::No);

            if(reply==QMessageBox::Yes)
            {
                pausedMutex->lock();
                    *paused = true;
                pausedMutex->unlock();

                ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/startIcon2.png'); background-repeat: none; border: none;}"
                                                       "QPushButton:hover{background-image: url(':/main/icons/startIconHover.png');}"));
                log_message("Transmitter thread paused");

                transmittingStatusMutex->unlock();

                return;
            }

            transmitter->closeRS232port();
            transmitter->terminate();   

            log_message("Terminating transmitter thread");
            *transmittingStatus = false;
            *paused = false;

            delete transmitter;
            transmitter = NULL;

            ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/startIcon.png'); background-repeat: none; border: none;}"
                                                   "QPushButton:hover{background-image: url(':/main/icons/startIconHover.png');}"));
            ui->timeStepIntervalButton->setDisabled(false);
            ui->newFilesButton->setDisabled(false);
        }

        transmittingStatusMutex->unlock();
}

void MainWindow::updateProgressBar(int counter)
{
    int percent = ((double)(counter)/(double)(number_of_last_records))*100.0;
    ui->progress->setValue(percent);
}

void MainWindow::allFilesFinishedSlot()
{
    transmitter->closeRS232port();
    transmitter->quit();
    log_message("All data were sent. No more data in files. Closing thread.");

    ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/startIcon.png'); background-repeat: none; border: none;}"
                                           "QPushButton:hover{background-image: url(':/main/icons/startIconHover.png');}"));
    ui->timeStepIntervalButton->setDisabled(false);
    ui->newFilesButton->setDisabled(false);

    *transmittingStatus = false;
    *paused = false;

    delete transmitter;
    transmitter = NULL;
}

void MainWindow::pipeCreationFailureSlot(void)
{
    log_message("Error: Could not create pipe handler");
}

void MainWindow::creatingEventsSlot(void)
{
    log_message("Creating signalization events");
}

void MainWindow::waitingForClientSlot(void)
{
    log_message("Waiting for client");
}

void MainWindow::clientConnectedSlot(void)
{
    log_message("Client successfully connected");
}

void MainWindow::clientNotRespondingSlot(void)
{
    transmitter->quit();
    log_message("Client not responging - timeout passed");

    ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/startIcon.png'); background-repeat: none; border: none;}"
                                           "QPushButton:hover{background-image: url(':/main/icons/startIconHover.png');}"));
    ui->timeStepIntervalButton->setDisabled(false);
    ui->newFilesButton->setDisabled(false);

    delete transmitter;
    transmitter = NULL;

    *transmittingStatus = false;
    *paused = false;
}

void MainWindow::clientDisconnectedSlot()
{
    transmitter->terminate();

    qDebug() << "Closing";

    log_message("Client is disconnected. Terminating transmitter thread.");

    ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/startIcon.png'); background-repeat: none; border: none;}"
                                           "QPushButton:hover{background-image: url(':/main/icons/startIconHover.png');}"));
    ui->timeStepIntervalButton->setDisabled(false);
    ui->newFilesButton->setDisabled(false);

    delete transmitter;
    transmitter = NULL;

    *transmittingStatus = false;
    *paused = false;
}

void MainWindow::comportCouldNotBeOpened()
{

    transmitter->terminate();
    log_message("The com port could not be opened. No transmitting is running.");

    ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/startIcon.png'); background-repeat: none; border: none;}"
                                           "QPushButton:hover{background-image: url(':/main/icons/startIconHover.png');}"));
    ui->timeStepIntervalButton->setDisabled(false);
    ui->newFilesButton->setDisabled(false);

    delete transmitter;
    transmitter = NULL;

    *transmittingStatus = false;
    *paused = false;
}

void MainWindow::fileNotOpened(QString str)
{
    log_message(QString("The following file could not be opened: ").append(str));
    log_message("Terminating transmitter thread");
    transmitter->quit();

    ui->startButton->setStyleSheet(QString("QPushButton{background-image: url(':/main/icons/startIcon.png'); background-repeat: none; border: none;}"
                                           "QPushButton:hover{background-image: url(':/main/icons/startIconHover.png');}"));
    ui->timeStepIntervalButton->setDisabled(false);
    ui->newFilesButton->setDisabled(false);

    delete transmitter;
    transmitter = NULL;
}

void MainWindow::timeStepIntervalManualEnter()
{
    bool ok;
    double temp = QInputDialog::getDouble(this, tr("Time step interval setup"), tr("Enter the time interval"), time_step_interval, 0.005, 2.0, 5, &ok);
    if(ok)
    {
        time_step_interval = temp;
        log_message(QString("Time step interval set to %1 manually").arg(time_step_interval));
    }
    else log_message(QString("Time step interval remains %1").arg(time_step_interval));
}


void MainWindow::log_message(QString message)
{
    ui->logger->append(message);
}

bool MainWindow::file_analyzer(QString file_path)
{
    int iterator = 0; // represents the number of line currently analysed

    QFile file(file_path);
    if(!file.open(QFile::ReadOnly))
    {
        log_message(QString("Could not open file: ").append(file.fileName()));
        file.close();
        return false;
    }
    else
    {
        // analyze the file structure
        QTextStream input(&file);

        while(!input.atEnd())
        {
            iterator++; // represents the number of line currently analysed

            QStringList line_parse(input.readLine().split(QChar('#')));

            // minimal of items must be 3 according to structure
            short count = line_parse.count();
            if(count<3)
            {
                log_message(QString("The following file does not have minimal data: ").append(file.fileName()));
                file.close();
                return false;
            }

            // at 2nd index must be integer number representing number of visible targets
            bool isNumber = false;
            int visible_targets = line_parse.at(2).toInt(&isNumber);
            if(!isNumber)
            {
                log_message(QString("In file \'%1\' on line %2 number of visible targets was not recognized").arg(file.fileName()).arg(iterator));
                file.close();
                return false; // was not int
            }

            // checking for complete sets of x, y, TOA1, TOA2
            if(4*visible_targets+3!=count)
            {
                log_message(QString("In file \'%1\' on line %2 number of visible targets does not correspond with data initiated").arg(file.fileName()).arg(iterator));
                file.close();
                return false;
            }

            // check if there is required number of x, y, TOA1, TOA2

            Q_FOREACH(QString str, line_parse)
            {
                str.toDouble(&isNumber);
                if(!isNumber)
                {
                    log_message(QString("In file \'%1\' on line %2 some data are not relevant").arg(file.fileName()).arg(iterator));
                    file.close();
                    return false; // is not relevant number
                }
            }

        }
    }

    // if file seems to be OK, we will check if number of records is same as in other files analysed previously
    if(number_of_last_records>0)
    {
        if(iterator!=number_of_last_records)
        {
            log_message(QString("In file \'%1\', number of records is not same as in previous files: %2").arg(file.fileName()).arg(iterator));
            return false;
        }
    }
    else
    {
        number_of_last_records = iterator;
        log_message(QString("Setting number of records constant to %1").arg(iterator));
    }

    log_message("File analyze success: File is OK");
    file.close();
    return true;
}
