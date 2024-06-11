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
#include <vector>

#include "db.hh"
#include "dbUtils.hh"
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

  // Create a horizontal layout to hold the text box and slider
  QHBoxLayout *textBoxLayout = new QHBoxLayout();
  textBoxLayout->addWidget(textBox);

  // Initialize the slider
  maxRowsSlider = new QSlider(Qt::Horizontal, this);
  maxRowsSlider->setRange(1, 10000);    // Adjust range as needed
  maxRowsSlider->setValue(maxTabRows);  // Set default value

  // Initialize the labels for min and max values
  minLabel = new QLabel("1", this);
  maxLabel = new QLabel("10000", this);

  // Add the text box, min label, slider, and max label to the layout
  textBoxLayout->addWidget(minLabel);
  textBoxLayout->addWidget(maxRowsSlider);
  textBoxLayout->addWidget(maxLabel);

  leftLayout->addLayout(textBoxLayout);
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

  // Connect the slider value change signal to the slot
  connect(maxRowsSlider, &QSlider::valueChanged, this,
          &MainWindow::onMaxRowsSliderValueChanged);
}
void MainWindow::setSliderLimits(size_t max) {
  maxRowsSlider->setRange(1, max);  // Adjust range as needed
  maxLabel->setText(QString::number(max));
}

void MainWindow::onMaxRowsSliderValueChanged(int value) {
  maxTabRows = value;
  onTimerTimeout();  // Refresh the table view with the new max rows value
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
    for (const auto &[year, papers] : result._yearToPapers) {
      unionResult._yearToPapers[year].insert(papers.begin(), papers.end());
    }
    totalCitations += result._totalCitations;
    unionResult._papers.insert(result._papers.begin(), result._papers.end());
  }
  // remove trailing comma and space
  unionResult._word = unionResult._word.substr(0, unionResult._word.size() - 2);
  size_t maxChars = 50;
  if (unionResult._word.size() > maxChars) {
    unionResult._word = unionResult._word.substr(0, maxChars) + "...";
  }
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
  model->insertRow(0, rowItems);

  addKQR(unionResult);
  setSliderLimits(model->rowCount());
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
  } else if (event->modifiers() == Qt::ControlModifier &&
             event->key() == Qt::Key_A) {
    // Select all rows in the table
    tableView->selectAll();
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
  textChangedTimer->start(1000);
}

void MainWindow::onTimerTimeout() {
  std::vector<KeywordQueryResult> kqr_vec =
      searchKeywords(this->textBox->text().toStdString());

  // Clear previous data from the model
  model->removeRows(0, model->rowCount());
  size_t maxKeywords = maxTabRows;

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
  setSliderLimits(kqr_vec.size());
}
void MainWindow::onTableClicked(const QModelIndex &index) {
  if (index.isValid() &&
      index.column() == 2) {  // Check if the third column is clicked
    QString keyword = model->data(index.siblingAtColumn(0)).toString();
    openDB();
    KeywordQueryResult kqr = getKQR(keyword.toStdString());

    // Fix missing data with zeros
    auto min_max = std::minmax_element(
        kqr._yearToCitations.begin(), kqr._yearToCitations.end(),
        [](const auto &a, const auto &b) { return a.first < b.first; });

    for (size_t year = min_max.first->first; year <= min_max.second->first;
         ++year) {
      if (kqr._yearToCitations.count(year) == 0) {
        kqr._yearToCitations.emplace(year, 0);
      }
    }

    // Create series for citation data, rate of change, and impact factor
    QLineSeries *citationSeries = new QLineSeries();
    QLineSeries *rateOfChangeSeries = new QLineSeries();
    QLineSeries *impactFactorSeries = new QLineSeries();

    // Set point labels format and make them visible for citationSeries
    citationSeries->setPointLabelsFormat("@yPoint");
    citationSeries->setPointLabelsVisible(true);
    citationSeries->setPointLabelsColor(Qt::black);
    citationSeries->setPointLabelsClipping(false);
    citationSeries->setPointLabelsFont(QFont("Arial", 15));

    // Set point labels format and make them visible for rateOfChangeSeries
    rateOfChangeSeries->setPointLabelsFormat("@yPoint");
    rateOfChangeSeries->setPointLabelsVisible(true);
    rateOfChangeSeries->setPointLabelsColor(Qt::black);
    rateOfChangeSeries->setPointLabelsClipping(false);
    rateOfChangeSeries->setPointLabelsFont(QFont("Arial", 15));

    // Set point labels format and make them visible for impactFactorSeries
    impactFactorSeries->setPointLabelsFormat("@yPoint");
    impactFactorSeries->setPointLabelsVisible(true);
    impactFactorSeries->setPointLabelsColor(Qt::black);
    impactFactorSeries->setPointLabelsClipping(false);
    impactFactorSeries->setPointLabelsFont(QFont("Arial", 15));

    size_t cumulativeCitations = 0;

    for (const auto &[year, citations] : kqr._yearToCitations) {
      if (year == getCurrentYear()) {
        continue;  // Skip the current year
      }
      cumulativeCitations += citations;
      citationSeries->append(year, cumulativeCitations);
      rateOfChangeSeries->append(year, citations);
      // Calculate impact factor
      size_t numPapers =
          kqr._yearToPapers.count(year) ? kqr._yearToPapers.at(year).size() : 1;
      double impactFactor = static_cast<double>(citations) / numPapers;
      impactFactorSeries->append(year, impactFactor);
    }

    // Create a chart and set the series
    QChart *chart = new QChart();
    chart->addSeries(citationSeries);
    chart->addSeries(rateOfChangeSeries);
    chart->addSeries(impactFactorSeries);  // Add the impact factor series
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(
        Qt::AlignBottom);  // Position legend at the bottom

    QValueAxis *axisX = new QValueAxis;
    axisX->setLabelFormat("%d");
    axisX->setTitleText("Year");
    axisX->setRange(min_max.first->first, min_max.second->first - 1);
    axisX->setTickCount(min_max.second->first - min_max.first->first + 1);
    chart->addAxis(axisX, Qt::AlignBottom);
    citationSeries->attachAxis(axisX);
    rateOfChangeSeries->attachAxis(axisX);
    impactFactorSeries->attachAxis(
        axisX);  // Attach the impact factor series to the axis

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%d");
    axisY->setTitleText("Citations");
    axisY->setRange(0, cumulativeCitations + 1);  // Set the range of the axis
    chart->addAxis(axisY, Qt::AlignLeft);
    citationSeries->attachAxis(axisY);
    rateOfChangeSeries->attachAxis(axisY);

    QValueAxis *axisY2 = new QValueAxis;
    axisY2->setLabelFormat("%.2f");
    axisY2->setTitleText("Impact Factor");
    chart->addAxis(axisY2, Qt::AlignRight);
    impactFactorSeries->attachAxis(
        axisY2);  // Attach the impact factor series to the new axis

    citationSeries->setName("Cumulative Citations");
    rateOfChangeSeries->setName("Rate of Change");
    impactFactorSeries->setName("Impact Factor");

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

