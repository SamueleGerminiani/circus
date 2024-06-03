#include "gui.hh"

#include <QApplication>
#include <QMessageBox>
#include <QPainter>
#include <QStandardItemModel>  // Include QStandardItemModel for the table
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <vector>

#include "db.hh"
#include "message.hh"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      textBox(new QLineEdit(this)),
      tableView(new QTableView(this)),
      model(new QStandardItemModel(0, 2, this)),
      textChangedTimer(new QTimer(this)),
      tabWidget(new QTabWidget(this)) {
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
  model->setHeaderData(1, Qt::Horizontal, "Total Citations");

  // Set the table to be non-editable
  tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // Make the tabs closable
  tabWidget->setTabsClosable(true);
  // Enable movable tabs
  tabWidget->setMovable(true);

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

  // Connect tab close requested signal to removeTab slot
  connect(tabWidget, &QTabWidget::tabCloseRequested, tabWidget,
          &QTabWidget::removeTab);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_W) {
    // Close the current tab when Ctrl + W is pressed
    tabWidget->removeTab(tabWidget->currentIndex());
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

void MainWindow::startTextChangedTimer() {
  // If the timer is running, stop it and start again
  if (textChangedTimer->isActive()) textChangedTimer->stop();

  // Start the timer with the desired delay
  textChangedTimer->start(500);
}

void MainWindow::onTimerTimeout() {
  std::vector<KeywordQueryResult> kqr_vec =
      searchKeywords(this->textBox->text().toStdString());

  // Clear previous data from the model
  model->removeRows(0, model->rowCount());

  // Add new data to the model
  for (const auto &kqr : kqr_vec) {
    QList<QStandardItem *> rowItems;
    rowItems << new QStandardItem(QString::fromStdString(kqr._word));
    rowItems << new QStandardItem(QString::number(kqr._totalCitations));
    model->appendRow(rowItems);
  }

  // Resize columns to fit contents
  tableView->resizeColumnsToContents();

  // Hide the chart tab widget if no charts are being shown
  if (tabWidget->count() == 0) {
    tabWidget->hide();
  }
}
void MainWindow::onTableClicked(const QModelIndex &index) {
  if (index.isValid() &&
      index.column() == 1) {  // Check if the second column is clicked
    QString keyword = model->data(index.siblingAtColumn(0)).toString();
    openDB();
    KeywordQueryResult kqr = getCitations(keyword.toStdString());
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
}

void runGui(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow window;
  window.showMaximized();
  app.exec();
  messageInfo("GUI closed");
}

