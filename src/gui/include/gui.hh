#pragma once

#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QStringListModel>

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

 private slots:
  void onTextChanged(const QString &text);

 private:
  QLineEdit *textBox;
  QListView *listView;
  QStringListModel *model;

  QStringList searchKeywords(const std::string &word);
};

void runGui(int argc, char *argv[]);
