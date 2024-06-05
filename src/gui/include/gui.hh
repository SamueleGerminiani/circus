#pragma once

#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QStandardItemModel>  // Include QStandardItemModel for the table
#include <QStringListModel>
#include <QTableView>
#include <QTimer>

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr);
  void setupLayout();
  void setupConnections();
  ~MainWindow();

 private slots:
  void startTextChangedTimer();
  void onTimerTimeout();
  void onTableClicked(const QModelIndex &index);
  void keyPressEvent(QKeyEvent *event) override;
  void adjustFontSizes();

  void resizeEvent(QResizeEvent *event) override;
  void addUnionOfSelectedRows();
  void removeSelectedRows();

 private:
  QLineEdit *textBox;
  QTableView *tableView;
  QStandardItemModel *model;
  QTimer *textChangedTimer;
  QTabWidget *tabWidget;
};

void runGui(int argc, char *argv[]);
