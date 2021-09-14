#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "jsontablemodel.h"
#include <QNetworkReply>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItemModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    fillInstruments();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fillInstruments()
{
    QList<QString> instruments = {"default","merlin","nimrod","sandals","iris"};
    ui -> testBox2->clear();
    foreach (const QString instrument, instruments)
    {
        ui -> testBox2->addItem(instrument);
    }
}

void MainWindow::handle_result(HttpRequestWorker *worker) {
    QString msg;

    if (worker->error_type == QNetworkReply::NoError) {
        auto response = worker->response;
        ui -> testBox->clear();
        ui -> testBox->addItem("default");
        foreach (const QJsonValue &value, worker->json_array)
        {
            if (value.toString()!="journal.xml")
                ui -> testBox->addItem(value.toString());
        }
    }
    else {
        // an error occurred
        msg = "Error1: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

void MainWindow::handle_result2(HttpRequestWorker *worker) {
    QString msg;

    if (worker->error_type == QNetworkReply::NoError) {
        auto jsonArray = worker->json_array;
        auto jsonObject = jsonArray.at(0).toObject();
        JsonTableModel::Header header;
        foreach(const QString& key, jsonObject.keys())
        {
            header.push_back(JsonTableModel::Heading( { {"title",key},    {"index",key} }) );
        }
        model = new JsonTableModel(header, this);
        model->setJson(jsonArray);
        ui->testTable->setModel(model);
        ui->testTable->show();

    }
    else {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}


void MainWindow::on_testBox2_currentIndexChanged(const QString &arg1)
{
    if (arg1 == "default")
    {
        return;
    }
    QString url_str = "http://127.0.0.1:5000/getCycles/"+arg1;

    HttpRequestInput input(url_str, "GET");

    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker*)), this, SLOT(handle_result(HttpRequestWorker*)));
    worker->execute(&input);
}


void MainWindow::on_testBox_currentIndexChanged(const QString &arg1)
{
    if (arg1 == "default")
    {
        return;
    }
    QString url_str = "http://127.0.0.1:5000/getJournal/"+ui->testBox2->currentText()+"/"+arg1;

    HttpRequestInput input(url_str, "GET");

    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker*)), this, SLOT(handle_result2(HttpRequestWorker*)));
    worker->execute(&input);
}

