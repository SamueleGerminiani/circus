#include "gui.hh"

#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>  // Include QStandardItemModel for the table
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <algorithm>
#include <iostream>
#include <vector>

#include "db.hh"
#include "message.hh"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      textBox(new QLineEdit(this)),
      tableView(new QTableView(this)),
      model(new QStandardItemModel(0, 4, this)),
      textChangedTimer(new QTimer(this)),
      tabWidget(new QTabWidget(this)) {
  setupLayout();
  setupConnections();
}

void MainWindow::setupLayout() {
  // Set up the layout
  QWidget *centralWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

  QVBoxLayout *leftLayout = new QVBoxLayout();
  leftLayout->addWidget(textBox);
  leftLayout->addWidget(tableView);

  mainLayout->addLayout(leftLayout);
  mainLayout->addWidget(tabWidget);

  setCentralWidget(centralWidget);

  // Set the model for the table view
  tableView->setModel(model);

  // Set headers for the table
  model->setHeaderData(0, Qt::Horizontal, "Keyword");
  model->setHeaderData(1, Qt::Horizontal, "Type");
  model->setHeaderData(2, Qt::Horizontal, "Total Citations");
  model->setHeaderData(3, Qt::Horizontal, "z-score");

  // Set the table to be non-editable
  tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Make the tabs closable and movable
  tabWidget->setTabsClosable(true);
  tabWidget->setMovable(true);

  // Enable sorting on the table view
  tableView->setSortingEnabled(true);
}

void MainWindow::setupConnections() {
  connect(tabWidget, &QTabWidget::tabCloseRequested, tabWidget,
          &QTabWidget::removeTab);

  // Create a timer with a single-shot mode
  textChangedTimer->setSingleShot(true);

  // Connect the text changed signal to the slot
  connect(textBox, &QLineEdit::textChanged, this,
          &MainWindow::startTextChangedTimer);

  // Connect the timer's timeout signal to the slot
  connect(textChangedTimer, &QTimer::timeout, this,
          &MainWindow::onTimerTimeout);

  // Connect the table view click signal to the slot
  connect(tableView, &QTableView::clicked, this, &MainWindow::onTableClicked);
}

void MainWindow::addUnionOfSelectedRows() {
  QModelIndexList selectedIndexes = tableView->selectionModel()->selectedRows();

  if (selectedIndexes.size() < 2) {
    // No selected rows, do nothing
    return;
  }

  std::vector<KeywordQueryResult> searchResults =
      searchKeywords(textBox->text().toStdString());

  std::vector<KeywordQueryResult> selected_keywords;
  for (const QModelIndex &index : selectedIndexes) {
    int row = index.row();
    QString word = model->data(model->index(row, 0)).toString();

    auto found = std::find_if(searchResults.begin(), searchResults.end(),
                              [&word](const KeywordQueryResult &kqr) {
                                return kqr._word == word.toStdString();
                              });
    messageErrorIf(found == searchResults.end(),
                   "Selected row not found in search results");
    selected_keywords.push_back(*found);
  }

  // Initialize a new KeywordQueryResult to store the union
  KeywordQueryResult unionResult;
  unionResult._word = "";
  unionResult._type.clear();
  unionResult._yearToCitations.clear();
  unionResult._papers.clear();

  // Collect the union of types and other fields from selected rows
  size_t totalCitations = 0;
  for (const auto &result : selected_keywords) {
    unionResult._word += result._word + ", ";
    for (auto type : result._type) {
      unionResult._type.insert(type);
    }
    for (const auto &[year, citations] : result._yearToCitations) {
      unionResult._yearToCitations[year] += citations;
    }
    totalCitations += result._totalCitations;
    unionResult._papers.insert(result._papers.begin(), result._papers.end());
  }
  // remove trailing comma and space
  unionResult._word = unionResult._word.substr(0, unionResult._word.size() - 2);
  unionResult._totalCitations = totalCitations;

  // Recalculate z-score
  addZScore(unionResult);

  // Add the union result to the table
  QList<QStandardItem *> rowItems;
  rowItems << new QStandardItem(QString::fromStdString(unionResult._word));
  std::string typeStr;
  for (auto type : unionResult._type) {
    typeStr += toString(type) + ", ";
  }
  // Remove the trailing comma and space
  typeStr = typeStr.substr(0, typeStr.size() - 2);

  rowItems << new QStandardItem(QString::fromStdString(typeStr));
  rowItems << new QStandardItem(QString::number(unionResult._totalCitations));
  QStandardItem *zScoreItem =
      new QStandardItem(QString::number(unionResult._zScore));
  if (unionResult._zScore >= 1.f) {
    zScoreItem->setData(QColor(Qt::darkGreen), Qt::BackgroundRole);
  } else if (unionResult._zScore < -1.f) {
    zScoreItem->setData(QColor(Qt::darkRed), Qt::BackgroundRole);
  }
  rowItems << zScoreItem;
  model->appendRow(rowItems);

  addKQR(unionResult);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_M) {
    // Add new table entry as the union of selected rows
    addUnionOfSelectedRows();
  } else if (event->modifiers() == Qt::ControlModifier &&
             event->key() == Qt::Key_W) {
    // Close the current tab when Ctrl + W is pressed
    tabWidget->removeTab(tabWidget->currentIndex());
  } else if (event->modifiers() == Qt::ControlModifier &&
             event->key() == Qt::Key_Backspace) {
    // Remove selected rows when Ctrl + Backspace is pressed
    removeSelectedRows();
  } else {
    // Handle other key events
    QMainWindow::keyPressEvent(event);
  }
}

MainWindow::~MainWindow() {
  delete textBox;
  delete tableView;
  delete model;
  delete textChangedTimer;
  delete tabWidget;
}
void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  adjustFontSizes();
}

// Custom slot to adjust font size of all text-containing widgets based on
// window size
void MainWindow::adjustFontSizes() {
  int windowWidth = this->width();  // Get current window width

  // Iterate through all widgets in the application
  for (QWidget *widget : QApplication::allWidgets()) {
    // Check if the widget contains text
    if (QLabel *label = qobject_cast<QLabel *>(widget)) {
      label->setFont(
          QFont("Arial", windowWidth / 50));  // Set font size for QLabel
    } else if (QPushButton *button = qobject_cast<QPushButton *>(widget)) {
      button->setFont(
          QFont("Arial", windowWidth / 50));  // Set font size for QPushButton
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
      lineEdit->setFont(
          QFont("Arial", windowWidth / 50));  // Set font size for QLineEdit
    } else if (QTableView *tableView = qobject_cast<QTableView *>(widget)) {
      int fontSize = tableView->size().width() / 60;
      tableView->setFont(
          QFont("Arial", fontSize));  // Set font size for QTableView
      tableView->resizeColumnsToContents();
    }

    // Add more conditions for other text-containing widgets as needed
  }
}

void MainWindow::startTextChangedTimer() {
  // If the timer is running, stop it and start again
  if (textChangedTimer->isActive()) textChangedTimer->stop();

  // Start the timer with the desired delay
  textChangedTimer->start(300);
}

void MainWindow::onTimerTimeout() {
  std::vector<KeywordQueryResult> kqr_vec =
      searchKeywords(this->textBox->text().toStdString());

  // Clear previous data from the model
  model->removeRows(0, model->rowCount());
  size_t maxKeywords = INT_MAX;

  // Add new data to the model
  for (const auto &kqr : kqr_vec) {
    if (maxKeywords-- == 0) break;

    QList<QStandardItem *> rowItems;
    rowItems << new QStandardItem(QString::fromStdString(kqr._word));

    std::string typeStr;
    for (auto type : kqr._type) {
      typeStr += toString(type) + ", ";
    }
    // Remove the trailing comma and space
    typeStr = typeStr.substr(0, typeStr.size() - 2);

    rowItems << new QStandardItem(QString::fromStdString(typeStr));

    // new integer item for citations
    auto citationItem = new QStandardItem();
    citationItem->setData(static_cast<qlonglong>(kqr._totalCitations),
                          Qt::DisplayRole);  // Cast to qlonglong

    rowItems << citationItem;

    // new double item for z-score
    auto zScoreItem = new QStandardItem();
    zScoreItem->setData(kqr._zScore, Qt::DisplayRole);
    // set color to green if z-score is greater than 1
    if (kqr._zScore >= 1.f) {
      zScoreItem->setData(QColor(Qt::darkGreen), Qt::BackgroundRole);
    } else if (kqr._zScore < -1.f) {
      zScoreItem->setData(QColor(Qt::darkRed), Qt::BackgroundRole);
    }
    rowItems << zScoreItem;
    model->appendRow(rowItems);
  }

  // Resize columns to fit contents
  tableView->resizeColumnsToContents();

  // Sort the table by the third column in descending order of citations
  tableView->sortByColumn(2, Qt::DescendingOrder);
}
void MainWindow::onTableClicked(const QModelIndex &index) {
  if (index.isValid() &&
      index.column() == 2) {  // Check if the third column is clicked
    QString keyword = model->data(index.siblingAtColumn(0)).toString();
    openDB();
    KeywordQueryResult kqr = getKQR(keyword.toStdString());
    // fix missing data with zeros
    auto min_max = std::minmax_element(
        kqr._yearToCitations.begin(), kqr._yearToCitations.end(),
        [](const auto &a, const auto &b) { return a.first < b.first; });

    for (size_t year = min_max.first->first; year <= min_max.second->first;
         ++year) {
      if (kqr._yearToCitations.count(year) == 0) {
        kqr._yearToCitations.emplace(year, 0);
      }
    }

    std::vector<std::pair<size_t, size_t>> rateOfChangeData;

    // Create the first series for citation data
    QLineSeries *series1 = new QLineSeries();
    QLineSeries *series2 = new QLineSeries();
    size_t comulativeCitations = 0;

    for (const auto &[x, y] : kqr._yearToCitations) {
      comulativeCitations += y;
      series1->append(x, comulativeCitations);
      series2->append(x, y);
    }

    // Create a chart and set the series
    QChart *chart = new QChart();
    chart->addSeries(series1);
    chart->addSeries(series2);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(
        Qt::AlignBottom);  // Position legend at the bottom

    QValueAxis *axisX = new QValueAxis;
    axisX->setLabelFormat("%d");
    axisX->setTitleText("Citations");
    axisX->setRange(min_max.first->first, min_max.second->first);
    // Add a label for every year
    axisX->setTickCount(min_max.second->first - min_max.first->first + 1);
    chart->addAxis(axisX, Qt::AlignBottom);
    series1->attachAxis(axisX);
    series2->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%d");
    axisY->setTitleText("Year");
    axisY->setRange(0, comulativeCitations + 1);  // Set the range of the axis
    chart->addAxis(axisY, Qt::AlignLeft);
    series1->attachAxis(axisY);
    series2->attachAxis(axisY);

    series1->setName("Citations");
    series2->setName("Rate of Change");

    // Remove the title of the chart
    chart->setTitle(keyword);

    // Create a chart view and set the chart
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Add the chart view as a new tab
    int tabIndex = tabWidget->addTab(chartView, keyword);
    tabWidget->setCurrentIndex(tabIndex);
    tabWidget->show();  // Ensure the tab widget is shown when charts are added
  }

  adjustFontSizes();
}
void MainWindow::removeSelectedRows() {
  QModelIndexList selectedIndexes = tableView->selectionModel()->selectedRows();

  // Remove selected rows from the model
  for (int i = selectedIndexes.size() - 1; i >= 0; --i) {
    auto row = selectedIndexes.at(i).row();
    QString word = model->data(model->index(row, 0)).toString();
    removeKQR(getKQR(word.toStdString()));
    model->removeRow(row);
  }
}

void runGui(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow window;
  window.showMaximized();
  app.exec();
  messageInfo("GUI closed");
}

