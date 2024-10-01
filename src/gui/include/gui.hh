#pragma once

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QSplitter>
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

  void resizeEvent(QResizeEvent *event) override;
  void addUnionOfSelectedRows();
  void removeSelectedRows();
  void onMaxRowsSliderValueChanged(int value);
  void onPapersTableClicked(const QModelIndex &index);
  void decreaseSize();
  void increaseSize();

 private:
  void setSliderLimits(size_t max);

  QLineEdit *keySearch_textBox = nullptr;
  QTableView *keywords_tableView = nullptr;
  QTimer *textChanged_timer = nullptr;
  QTabWidget *tabWidget = nullptr;
  QStandardItemModel *keywordsTab_model = nullptr;
  QCheckBox *indexTerm_checkbox = nullptr;
  QCheckBox *area_checkbox = nullptr;
  QCheckBox *authorKeyword_checkbox = nullptr;
  QSplitter *splitter = nullptr;
  QLabel *min_label = nullptr;
  QLabel *max_label = nullptr;
  QSlider *maxRows_slider = nullptr;

  int maxTabRows = 1000;  // Default maximum number of rows
};

void runGui(int argc, char *argv[]);
