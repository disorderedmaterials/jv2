#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "httprequestworker.h"
#include "jsontablemodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void fillInstruments();
private slots:
    void handle_result(HttpRequestWorker *worker);
    void handle_result2(HttpRequestWorker *worker);
    void on_testBox2_currentIndexChanged(const QString &arg1);

    void on_testBox_currentIndexChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    JsonTableModel *model;
};
#endif // MAINWINDOW_H
