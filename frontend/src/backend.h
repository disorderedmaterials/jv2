// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef BACKEND_H
#define BACKEND_H

#include <QProcess>
#include <QString>

/**
 * Control the backend process that communicates with the Journal server
 */
class Backend : public QObject
{
    Q_OBJECT

    public:
    static QString bind_address() { return "127.0.0.1:5000"; };

    Backend();

    public slots:
    void start();
    void stop();

    private:
    QProcess process_;
};

#endif
