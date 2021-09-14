#include "httprequestworker.h"
#include <QBuffer>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

HttpRequestInput::HttpRequestInput(QString v_url_str) { url_str = v_url_str; }

HttpRequestWorker::HttpRequestWorker(QObject *parent)
    : QObject(parent), manager(NULL) {
  qsrand(QDateTime::currentDateTime().toTime_t());

  manager = new QNetworkAccessManager(this);
  connect(manager, SIGNAL(finished(QNetworkReply *)), this,
          SLOT(on_manager_finished(QNetworkReply *)));
}

void HttpRequestWorker::execute(HttpRequestInput *input) {

  // reset variables
  QByteArray request_content = "";
  response = "";
  error_type = QNetworkReply::NoError;
  error_str = "";

  // prepare connection

  QNetworkRequest request = QNetworkRequest(QUrl(input->url_str));
  request.setRawHeader("User-Agent", "Agent name goes here");
  manager->get(request);
}

void HttpRequestWorker::on_manager_finished(QNetworkReply *reply) {
  error_type = reply->error();
  if (error_type == QNetworkReply::NoError) {
    response = reply->readAll();
    jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    json_array = jsonResponse.array();

  } else {
    error_str = reply->errorString();
  }

  reply->deleteLater();

  emit on_execution_finished(this);
}
