// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "backend.h"
#include "mainwindow.h"

#include <QApplication>
#include <QProcess>
#include <QThread>

int main(int argc, char *argv[])
{
    // Configure the main application early to create an event loop
    QApplication application(argc, argv);
    application.setWindowIcon(QIcon(":/icon"));
    // Start backend as soon as we can
    Backend backend;
    backend.start();
    // A sleep is not ideal but the mainwindow needs the backend fully started
    // before it can start
    QThread::sleep(1);

    // Start frontend and hook up kill backend on quit
    QObject::connect(&application, &QApplication::aboutToQuit, &backend, &Backend::stop);
    MainWindow window;
    window.show();
    return application.exec();
}
