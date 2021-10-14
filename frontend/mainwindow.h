// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "httprequestworker.h"
#include "jsontablemodel.h"
#include <QCheckBox>
#include <QMainWindow>
#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void fillInstruments();
  void initialiseElements();
  void goToCurrentFoundIndex(QModelIndex index);
private slots:
  void on_filterBox_textChanged(const QString &arg1);
  void on_searchBox_textChanged(const QString &arg1);
  void handle_result_instruments(HttpRequestWorker *worker);
  void handle_result_cycles(HttpRequestWorker *worker);
  void on_instrumentsBox_currentTextChanged(const QString &arg1);
  void on_cyclesBox_currentTextChanged(const QString &arg1);
  void on_groupButton_clicked(bool checked);
  void columnHider(int state);
  void on_clearSearchButton_clicked();
  void on_findUp_clicked();
  void on_findDown_clicked();
  void on_searchAll_clicked();
  void recentCycle();

protected:
  // Window close event
  void closeEvent(QCloseEvent *event);

private:
  Ui::MainWindow *ui;
  JsonTableModel *model;
  QSortFilterProxyModel *proxyModel;
  QMenu *viewMenu;
  JsonTableModel::Header header;
  QModelIndexList foundIndices;
  int currentFoundIndex;
  bool init;
};
#endif // MAINWINDOW_H
