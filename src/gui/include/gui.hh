#pragma once

#include <QCheckBox>
#include <QLabel>
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
  void updateTable();
  void onTableClicked(const QModelIndex &index);
  void openChartWindow(const QString &keyword);
  void openListOfPapers(const std::string &keyword);
  void keyPressEvent(QKeyEvent *event) override;
  void adjustFontSizes();

  void resizeEvent(QResizeEvent *event) override;
  void addUnionOfSelectedRows();
  void removeSelectedRows();
  void onMaxRowsSliderValueChanged(int value);
  void onPapersTableClicked(const QModelIndex &index);

 private:
  void setSliderLimits(size_t max);

  QLineEdit *textBox;
  QTableView *tableView;
  QStandardItemModel *model;
  QTimer *textChangedTimer;
  QTabWidget *tabWidget;
  QCheckBox *indexTerm;
  QCheckBox *area;
  QCheckBox *authorKeyword;

  QLabel *minLabel;
  QLabel *maxLabel;

  QSlider *maxRowsSlider;
  int maxTabRows = 1000;  // Default maximum number of rows
};

void runGui(int argc, char *argv[]);
