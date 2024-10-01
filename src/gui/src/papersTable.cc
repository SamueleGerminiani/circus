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

#include "DBPayload.hh"
#include "db.hh"
#include "gui.hh"

void MainWindow::openListOfPapers(const std::string &keyword) {
  // Fetch the papers using the keyword
  std::vector<DBPayload> papers = getPapers(keyword);

  // Create a new QTableWidget for the papers
  QTableWidget *table = new QTableWidget();
  table->setRowCount(static_cast<int>(papers.size()));
  table->setColumnCount(
      7);  // DOI, Title, Authors, Year, Total Citations, Keywords, Abstract

  // Set the column headers
  QStringList headers;
  headers << "Title"
          << "Year"
          << "Total Citations"
          << "Authors"
          << "DOI"
          << "Abstract"
          << "Keywords";
  table->setHorizontalHeaderLabels(headers);
  table->setSortingEnabled(true);

  // Populate the table with paper data
  int row = 0;
  for (const DBPayload &paper : papers) {
    // Title
    table->setItem(row, 0,
                   new QTableWidgetItem(QString::fromStdString(paper.title)));
    // Year
    QTableWidgetItem *yearItem = new QTableWidgetItem();
    yearItem->setData(Qt::EditRole, paper.year);  // Set as an integer
    yearItem->setTextAlignment(
        Qt::AlignCenter);  // Optional: Align the number in the center
    table->setItem(row, 1, yearItem);

    // Total Citations
    QTableWidgetItem *citationItem = new QTableWidgetItem();
    citationItem->setData(Qt::EditRole,
                          paper.total_citations);  // Set as an integer
    citationItem->setTextAlignment(
        Qt::AlignCenter);  // Optional: Align the number in the center
    table->setItem(row, 2, citationItem);

    // Authors
    table->setItem(
        row, 3,
        new QTableWidgetItem(QString::fromStdString(paper.authors_list)));

    // DOI
    table->setItem(row, 4,
                   new QTableWidgetItem(QString::fromStdString(paper.doi)));

    // Abstract
    table->setItem(
        row, 5, new QTableWidgetItem(QString::fromStdString(paper.abstract)));

    // Keywords (combining index terms and author keywords)
    std::string keywords;
    std::set<std::string> unique_keywords;
    for (const auto &index_term : paper.index_terms) {
      unique_keywords.insert(index_term);
    }
    for (const auto &author_keyword : paper.author_keywords) {
      unique_keywords.insert(author_keyword);
    }
    for (const auto &keyword : unique_keywords) {
      keywords += keyword + ", ";
    }
    if (!keywords.empty()) {
      keywords = keywords.substr(0, keywords.size() - 2);
    }
    table->setItem(row, 6,
                   new QTableWidgetItem(QString::fromStdString(keywords)));

    row++;
  }

  // Adjust table properties
  table->resizeColumnsToContents();
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);  // Disable editing

  // Create a new QWidget for the tab
  QWidget *tab = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(table);
  tab->setLayout(layout);

  // Add the new tab to the tabWidget
  QString tabTitle = QString::fromStdString("Papers for keyword: " + keyword);
  int tabIndex = tabWidget->addTab(tab, tabTitle);
  tabWidget->setCurrentIndex(tabIndex);

  // Connect the table view click signal to the slot
  connect(table, &QTableWidget::clicked, this,
          &MainWindow::onPapersTableClicked);

  // conform the reference size
  increaseSize();
  decreaseSize();
}

void MainWindow::onPapersTableClicked(const QModelIndex &index) {
  // Retrieve the sender widget (the QTableWidget that was clicked)
  QTableWidget *table = qobject_cast<QTableWidget *>(sender());

  // Ensure the table exists and the index is valid
  if (!table || !index.isValid()) return;

  // Retrieve the selected row and column
  int row = index.row();
  int column = index.column();

  // If the clicked column is the abstract column (assuming abstract is in
  // column 6)
  if (true || column == 6) {
    // Get the abstract from the abstract column
    QString abstract = table->item(row, 5)->text();

    // Create a new QWidget for the new tab
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();

    // Create a QLabel to display the abstract text in big font
    QLabel *abstractLabel = new QLabel(abstract);
    abstractLabel->setWordWrap(true);  // Wrap text to fit the widget
    QFont font = abstractLabel->font();
    font.setPointSize(abstractLabel->fontInfo().pointSize() *
                      2);  // Set a larger font size
    abstractLabel->setFont(font);

    // Create a QPushButton to copy the abstract to clipboard
    QPushButton *copyButton = new QPushButton("Copy Abstract to Clipboard");

    // Connect the button's clicked signal to a lambda that copies the abstract
    connect(copyButton, &QPushButton::clicked, this, [abstract]() {
      QClipboard *clipboard = QApplication::clipboard();
      clipboard->setText(abstract);  // Copy the abstract text to the clipboard
    });

    // Add the label and button to the layout
    layout->addWidget(abstractLabel);
    layout->addWidget(copyButton);
    tab->setLayout(layout);

    // Add the new tab to the tabWidget with a title based on the paper's title
    // or DOI
    //    QString doi = table->item(row, 0)->text();  // Assuming DOI is in
    //    column 0
    QString title =
        table->item(row, 0)->text();  // Assuming title is in column 1
    QString tabTitle = "Abstract - " + title;
    // Add the table as a new tab
    int tabIndex = tabWidget->addTab(tab, tabTitle);
    tabWidget->setCurrentIndex(tabIndex);
    tabWidget->show();
    // conform the reference size
    increaseSize();
    decreaseSize();
  }
}

