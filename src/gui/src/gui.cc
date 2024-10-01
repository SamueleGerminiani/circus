#include "gui.hh"

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "message.hh"

static void printTimeSeries(QLineSeries *series);

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupLayout();
  setupConnections();
}

void MainWindow::setupLayout() {
  keySearch_textBox = new QLineEdit(this);
  keywords_tableView = new QTableView(this);
  // Set up the layout
  QWidget *centralWidget = new QWidget(this);

  //  Create a splitter to divide the left and right sections
  splitter = new QSplitter(Qt::Horizontal, centralWidget);
  // The initial sizes for the splitter (adjust as needed)
  splitter->setStretchFactor(0, 1);  // Make left side stretchable
  splitter->setStretchFactor(1, 2);  // Make right side stretchable
  // Set the splitter as the central widget
  setCentralWidget(splitter);

  // Left part ------------------------------------------------------

  // Set up the table view
  keywordsTab_model = new QStandardItemModel(0, 4, this);
  // Set headers for the table
  keywordsTab_model->setHeaderData(0, Qt::Horizontal, "Keyword");
  keywordsTab_model->setHeaderData(1, Qt::Horizontal, "Type");
  keywordsTab_model->setHeaderData(2, Qt::Horizontal, "Total Citations");
  keywordsTab_model->setHeaderData(3, Qt::Horizontal, "z-score");
  // Set the keywordsTab_model for the table view
  keywords_tableView->setModel(keywordsTab_model);
  // Set the table to be non-editable
  keywords_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  // Enable sorting on the table view
  keywords_tableView->setSortingEnabled(true);

  // create a timer to update the table
  textChanged_timer = new QTimer(this);

  // Left layout: Text box, slider, labels, checkboxes, and table view
  QVBoxLayout *leftLayout = new QVBoxLayout();

  // Create a horizontal layout to hold the text box and slider
  QHBoxLayout *textSlider_hlayout = new QHBoxLayout();
  textSlider_hlayout->addWidget(keySearch_textBox);

  // Initialize the slider
  maxRows_slider = new QSlider(Qt::Horizontal, this);
  maxRows_slider->setRange(1, 10000);    // Adjust range as needed
  maxRows_slider->setValue(maxTabRows);  // Set default value

  // Initialize the labels for min and max values
  min_label = new QLabel("1", this);
  max_label = new QLabel("10000", this);

  // Initialize checkboxes
  indexTerm_checkbox = new QCheckBox("Index term", this);
  authorKeyword_checkbox = new QCheckBox("Author keyword", this);
  area_checkbox = new QCheckBox("Area", this);
  indexTerm_checkbox->setChecked(true);
  area_checkbox->setChecked(true);
  authorKeyword_checkbox->setChecked(true);

  // Add the text box, min label, slider, and max label to the layout
  textSlider_hlayout->addWidget(min_label);
  textSlider_hlayout->addWidget(maxRows_slider);
  textSlider_hlayout->addWidget(max_label);
  textSlider_hlayout->addWidget(authorKeyword_checkbox);
  textSlider_hlayout->addWidget(indexTerm_checkbox);
  textSlider_hlayout->addWidget(area_checkbox);

  // Add textSlider_hlayout and keywords_tableView to the left layout
  leftLayout->addLayout(textSlider_hlayout);
  leftLayout->addWidget(keywords_tableView);

  // Create a container widget for the left layout
  QWidget *leftWidget = new QWidget();
  leftWidget->setLayout(leftLayout);

  // Add the left widget to the splitter
  splitter->addWidget(leftWidget);

  // Right part ------------------------------------------------------
  tabWidget = new QTabWidget(this);
  // Make the tabs closable and movable
  tabWidget->setTabsClosable(true);
  tabWidget->setMovable(true);
  // Add the right widget to the splitter
  splitter->addWidget(tabWidget);
}

void MainWindow::setupConnections() {
  connect(tabWidget, &QTabWidget::tabCloseRequested, tabWidget,
          &QTabWidget::removeTab);

  // Create a timer with a single-shot mode
  textChanged_timer->setSingleShot(true);

  // Connect the text changed signal to the slot
  connect(keySearch_textBox, &QLineEdit::textChanged, this,
          &MainWindow::startTextChangedTimer);

  // Connect the timer's timeout signal to the slot
  connect(textChanged_timer, &QTimer::timeout, this, &MainWindow::updateTable);

  // Connect the table view click signal to the slot
  connect(keywords_tableView, &QTableView::clicked, this,
          &MainWindow::onTableClicked);

  // Connect the slider value change signal to the slot
  connect(maxRows_slider, &QSlider::valueChanged, this,
          &MainWindow::onMaxRowsSliderValueChanged);

  // connect the checkboxes to upate the table
  connect(authorKeyword_checkbox, &QCheckBox::stateChanged, this,
          &MainWindow::updateTable);
  connect(indexTerm_checkbox, &QCheckBox::stateChanged, this,
          &MainWindow::updateTable);
  connect(area_checkbox, &QCheckBox::stateChanged, this,
          &MainWindow::updateTable);
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
             event->key() == Qt::Key_Minus) {
    // Close the current tab when Ctrl + - is pressed
    decreaseSize();
  } else if (event->modifiers() == Qt::ControlModifier &&
             event->key() == Qt::Key_Equal) {
    // Close the current tab when Ctrl + = is pressed
    increaseSize();
  } else if (event->modifiers() == Qt::ControlModifier &&
             event->key() == Qt::Key_Backspace) {
    // Remove selected rows when Ctrl + Backspace is pressed
    removeSelectedRows();
  } else if (event->modifiers() == Qt::ControlModifier &&
             event->key() == Qt::Key_A) {
    // Select all rows in the table
    keywords_tableView->selectAll();
  } else if (event->modifiers() == Qt::ControlModifier &&
             event->key() == Qt::Key_L) {
    if (splitter->sizes()[0] != 0) {
      // Set the splitter sizes: left minimized, right maximized
      QList<int> sizes;
      sizes
          << 0
          << 1;  // 0 width for the left widget, full width for the right widget
      splitter->setSizes(sizes);
      splitter->update();
    } else {
      // Set the splitter sizes: left minimized, right maximized
      QList<int> sizes;
      sizes
          << 1
          << 0;  // 0 width for the left widget, full width for the right widget
      splitter->setSizes(sizes);
      splitter->update();
    }
  } else if (event->modifiers() == Qt::ControlModifier &&
             event->key() == Qt::Key_K) {
    // Set the splitter sizes: left minimized, right maximized
    QList<int> sizes;
    sizes << 1
          << 1;  // 0 width for the left widget, full width for the right widget
    splitter->setSizes(sizes);
    splitter->update();
  } else {
    // Handle other key events
    QMainWindow::keyPressEvent(event);
  }
}

MainWindow::~MainWindow() {}
void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
}

void MainWindow::decreaseSize() {
  int decrement = 2;

  int reference_labelSize = 0;
  for (QWidget *widget : QApplication::allWidgets()) {
    if (QLabel *label = qobject_cast<QLabel *>(widget)) {
      // get size of the font
      int labelSize = label->font().pointSize();
      reference_labelSize = std::max(reference_labelSize, labelSize);
    }
  }
  int reference_buttonSize = 0;
  for (QWidget *widget : QApplication::allWidgets()) {
    if (QPushButton *button = qobject_cast<QPushButton *>(widget)) {
      // get size of the font
      int labelSize = button->font().pointSize();
      reference_buttonSize = std::max(reference_buttonSize, labelSize);
    }
  }
  int reference_lineEditSize = 0;
  for (QWidget *widget : QApplication::allWidgets()) {
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
      // get size of the font
      int labelSize = lineEdit->font().pointSize();
      reference_lineEditSize = std::max(reference_lineEditSize, labelSize);
    }
  }
  int reference_tableViewSize = 0;
  for (QWidget *widget : QApplication::allWidgets()) {
    if (QTableView *tableView = qobject_cast<QTableView *>(widget)) {
      // get size of the font
      int labelSize = tableView->font().pointSize();
      reference_tableViewSize = std::max(reference_tableViewSize, labelSize);
    }
  }
  reference_labelSize -= decrement;
  reference_buttonSize -= decrement;
  reference_lineEditSize -= decrement;
  reference_tableViewSize -= decrement;

  // Iterate through all widgets in the application
  for (QWidget *widget : QApplication::allWidgets()) {
    // Check if the widget contains text
    if (QLabel *label = qobject_cast<QLabel *>(widget)) {
      // get size of the font
      label->setFont(
          QFont("Arial", reference_labelSize));  // Set font size for QLabel
    } else if (QPushButton *button = qobject_cast<QPushButton *>(widget)) {
      button->setFont(QFont(
          "Arial", reference_buttonSize));  // Set font size for QPushButton
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
      lineEdit->setFont(QFont(
          "Arial", reference_lineEditSize));  // Set font size for QLineEdit
    } else if (QTableView *tableView = qobject_cast<QTableView *>(widget)) {
      tableView->setFont(QFont(
          "Arial", reference_tableViewSize));  // Set font size for QTableView
      tableView->resizeColumnsToContents();
    }
  }
}
void MainWindow::increaseSize() {
  int increment = 2;

  int reference_labelSize = 0;
  for (QWidget *widget : QApplication::allWidgets()) {
    if (QLabel *label = qobject_cast<QLabel *>(widget)) {
      // get size of the font
      int labelSize = label->font().pointSize();
      reference_labelSize = std::max(reference_labelSize, labelSize);
    }
  }
  int reference_buttonSize = 0;
  for (QWidget *widget : QApplication::allWidgets()) {
    if (QPushButton *button = qobject_cast<QPushButton *>(widget)) {
      // get size of the font
      int labelSize = button->font().pointSize();
      reference_buttonSize = std::max(reference_buttonSize, labelSize);
    }
  }
  int reference_lineEditSize = 0;
  for (QWidget *widget : QApplication::allWidgets()) {
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
      // get size of the font
      int labelSize = lineEdit->font().pointSize();
      reference_lineEditSize = std::max(reference_lineEditSize, labelSize);
    }
  }
  int reference_tableViewSize = 0;
  for (QWidget *widget : QApplication::allWidgets()) {
    if (QTableView *tableView = qobject_cast<QTableView *>(widget)) {
      // get size of the font
      int labelSize = tableView->font().pointSize();
      reference_tableViewSize = std::max(reference_tableViewSize, labelSize);
    }
  }
  reference_labelSize += increment;
  reference_buttonSize += increment;
  reference_lineEditSize += increment;
  reference_tableViewSize += increment;

  // Iterate through all widgets in the application
  for (QWidget *widget : QApplication::allWidgets()) {
    // Check if the widget contains text
    if (QLabel *label = qobject_cast<QLabel *>(widget)) {
      // get size of the font
      label->setFont(
          QFont("Arial", reference_labelSize));  // Set font size for QLabel
    } else if (QPushButton *button = qobject_cast<QPushButton *>(widget)) {
      button->setFont(QFont(
          "Arial", reference_buttonSize));  // Set font size for QPushButton
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
      lineEdit->setFont(QFont(
          "Arial", reference_lineEditSize));  // Set font size for QLineEdit
    } else if (QTableView *tableView = qobject_cast<QTableView *>(widget)) {
      tableView->setFont(QFont(
          "Arial", reference_tableViewSize));  // Set font size for QTableView
      tableView->resizeColumnsToContents();
    }
  }
}

void runGui(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow window;
  window.showMaximized();
  app.exec();
  messageInfo("GUI closed");
}

