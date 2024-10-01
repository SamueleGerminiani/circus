#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <iostream>

#include "db.hh"
#include "dbUtils.hh"
#include "gui.hh"
#include "message.hh"

void MainWindow::setSliderLimits(size_t max) {
  maxRows_slider->setRange(1, max);  // Adjust range as needed
  max_label->setText(QString::number(max));
}

void MainWindow::onMaxRowsSliderValueChanged(int value) {
  maxTabRows = value;
  updateTable();  // Refresh the table view with the new max rows value
}

void MainWindow::addUnionOfSelectedRows() {
  QModelIndexList selectedIndexes =
      keywords_tableView->selectionModel()->selectedRows();

  if (selectedIndexes.size() < 2) {
    // No selected rows, do nothing
    return;
  }

  std::vector<KeywordQueryResult> searchResults =
      searchKeywords(keySearch_textBox->text().toStdString());

  std::vector<KeywordQueryResult> selected_keywords;
  for (const QModelIndex &index : selectedIndexes) {
    int row = index.row();
    QString word =
        keywordsTab_model->data(keywordsTab_model->index(row, 0)).toString();

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
    for (const auto &[year, cit] :
         result._yearToCitationInYearOfPapersPublishedThePreviousTwoYears) {
      unionResult
          ._yearToCitationInYearOfPapersPublishedThePreviousTwoYears[year] +=
          cit;
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
  keywordsTab_model->insertRow(0, rowItems);

  addKQR(unionResult);
  setSliderLimits(keywordsTab_model->rowCount());
}

void MainWindow::onTableClicked(const QModelIndex &index) {
  if (index.isValid()) {
    if (index.column() == 0) {  // Check if the first column is clicked
      openListOfPapers(keywordsTab_model->data(index.siblingAtColumn(0))
                           .toString()
                           .toStdString());
    } else if (index.column() == 2) {  // Check if the third column is clicked
      openChartWindow(
          keywordsTab_model->data(index.siblingAtColumn(0)).toString());
    }
  }
}

void printTimeSeries(QLineSeries *series) {
  for (int i = 0; i < series->count(); ++i) {
    QPointF point = series->at(i);
    std::cout << point.x() << " " << point.y() << std::endl;
  }
}

void MainWindow::removeSelectedRows() {
  QModelIndexList selectedIndexes =
      keywords_tableView->selectionModel()->selectedRows();

  // Remove selected rows from the keywordsTab_model
  for (int i = selectedIndexes.size() - 1; i >= 0; --i) {
    auto row = selectedIndexes.at(i).row();
    QString word =
        keywordsTab_model->data(keywordsTab_model->index(row, 0)).toString();
    removeKQR(getKQR(word.toStdString()));
    keywordsTab_model->removeRow(row);
  }
}

void MainWindow::startTextChangedTimer() {
  // If the timer is running, stop it and start again
  if (textChanged_timer->isActive()) textChanged_timer->stop();

  // Start the timer with the desired delay
  textChanged_timer->start(1000);
}

void MainWindow::updateTable() {
  std::vector<KeywordQueryResult> kqr_vec =
      searchKeywords(this->keySearch_textBox->text().toStdString());

  // Clear previous data from the keywordsTab_model
  keywordsTab_model->removeRows(0, keywordsTab_model->rowCount());
  size_t maxKeywords = maxTabRows;

  // Add new data to the keywordsTab_model
  for (const auto &kqr : kqr_vec) {
    if (maxKeywords-- == 0) break;

    QList<QStandardItem *> rowItems;
    rowItems << new QStandardItem(QString::fromStdString(kqr._word));

    if (!((indexTerm_checkbox->isChecked() &&
           kqr._type.count(KeywordType::IndexTerm)) ||
          (authorKeyword_checkbox->isChecked() &&
           kqr._type.count(KeywordType::AuthorKeyword)) ||
          (area_checkbox->isChecked() &&
           kqr._type.count(KeywordType::SubjectArea)))) {
      continue;
    }

    // build the string for the type
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
    keywordsTab_model->appendRow(rowItems);
  }

  // Resize columns to fit contents
  keywords_tableView->resizeColumnsToContents();

  // Sort the table by the third column in descending order of citations
  keywords_tableView->sortByColumn(2, Qt::DescendingOrder);
  setSliderLimits(kqr_vec.size());
}
void MainWindow::openChartWindow(const QString &keyword) {
  openDB();
  KeywordQueryResult kqr = getKQR(keyword.toStdString());

  auto min_max = std::minmax_element(
      kqr._yearToCitations.begin(), kqr._yearToCitations.end(),
      [](const auto &a, const auto &b) { return a.first < b.first; });

  // Fix missing data with zeros
  for (size_t year = min_max.first->first; year <= min_max.second->first;
       ++year) {
    if (kqr._yearToCitations.count(year) == 0) {
      kqr._yearToCitations.emplace(year, 0);
    }
  }

  // Create series for citation data, rate of change, and impact factor
  QLineSeries *citationSeries = new QLineSeries();
  QLineSeries *impactFactorSeries = new QLineSeries();
  QLineSeries *numberOfPapersSeries = new QLineSeries();

  // Set point labels format and make them visible for citationSeries
  citationSeries->setPointLabelsFormat("@yPoint");
  citationSeries->setPointLabelsVisible(true);
  citationSeries->setPointLabelsColor(Qt::black);
  citationSeries->setPointLabelsClipping(false);
  citationSeries->setPointLabelsFont(QFont("Arial", 15));

  // Set point labels format and make them visible for impactFactorSeries
  impactFactorSeries->setPointLabelsFormat("@yPoint");
  impactFactorSeries->setPointLabelsVisible(true);
  impactFactorSeries->setPointLabelsColor(Qt::black);
  impactFactorSeries->setPointLabelsClipping(false);
  impactFactorSeries->setPointLabelsFont(QFont("Arial", 15));

  // Set point labels format and make them visible for numberOfPapersSeries
  numberOfPapersSeries->setPointLabelsFormat("@yPoint");
  numberOfPapersSeries->setPointLabelsVisible(true);
  numberOfPapersSeries->setPointLabelsColor(Qt::black);
  numberOfPapersSeries->setPointLabelsClipping(false);
  numberOfPapersSeries->setPointLabelsFont(QFont("Arial", 15));

  size_t maxCitations = 0;
  size_t maxPapers = 0;

  for (const auto &[year, citations] : kqr._yearToCitations) {
    if (year == getCurrentYear()) {
      continue;  // Skip the current year
    }

    citationSeries->append(year, citations);
    maxCitations = std::max(maxCitations, citations);

    size_t newPapersThisYear =
        kqr._yearToPapers.count(year) ? kqr._yearToPapers.at(year).size() : 0;
    numberOfPapersSeries->append(year, newPapersThisYear);
    maxPapers = std::max(maxPapers, newPapersThisYear);

    double impactFactor = 0;
    if (year - min_max.first->first >= 2) {
      size_t nPapersOneYearBefore = kqr._yearToPapers.count(year - 1)
                                        ? kqr._yearToPapers.at(year - 1).size()
                                        : 0;
      size_t nPapersTwoYearsBefore = kqr._yearToPapers.count(year - 2)
                                         ? kqr._yearToPapers.at(year - 2).size()
                                         : 0;
      messageErrorIf(
          kqr._yearToCitationInYearOfPapersPublishedThePreviousTwoYears.empty(),
          "Missing data for impact factor calculation");
      if (nPapersOneYearBefore + nPapersTwoYearsBefore > 0 &&
          kqr._yearToCitationInYearOfPapersPublishedThePreviousTwoYears.count(
              year)) {
        impactFactor =
            static_cast<double>(
                kqr._yearToCitationInYearOfPapersPublishedThePreviousTwoYears
                    .at(year)) /
            (nPapersOneYearBefore + nPapersTwoYearsBefore);
      }
    }
    impactFactorSeries->append(year, impactFactor);
  }

  // Create a chart and set the series
  QChart *chart = new QChart();
  chart->addSeries(citationSeries);
  chart->addSeries(numberOfPapersSeries);
  chart->addSeries(impactFactorSeries);  // Add the impact factor series
  chart->legend()->setVisible(true);
  chart->legend()->setAlignment(
      Qt::AlignBottom);  // Position legend at the bottom

  QValueAxis *axisX = new QValueAxis;
  axisX->setLabelFormat("%d");
  axisX->setTitleText("Year");
  axisX->setRange(min_max.first->first, min_max.second->first - 1);
  axisX->setTickCount(getCurrentYear() - 1 - min_max.first->first + 1);
  chart->addAxis(axisX, Qt::AlignBottom);
  citationSeries->attachAxis(axisX);
  impactFactorSeries->attachAxis(
      axisX);  // Attach the impact factor series to the axis
  numberOfPapersSeries->attachAxis(axisX);

  QValueAxis *axisY = new QValueAxis;
  axisY->setLabelFormat("%d");
  axisY->setTitleText("Citations");
  axisY->setRange(
      0, std::max(maxCitations, maxPapers) + 1);  // Set the range of the
  // axis
  chart->addAxis(axisY, Qt::AlignLeft);
  citationSeries->attachAxis(axisY);

  QValueAxis *axisY2 = new QValueAxis;
  axisY2->setLabelFormat("%.2f");
  axisY2->setTitleText("Impact Factor");
  chart->addAxis(axisY2, Qt::AlignRight);
  impactFactorSeries->attachAxis(
      axisY2);  // Attach the impact factor series to the new axis

  citationSeries->setName("Citations");
  numberOfPapersSeries->setName("New papers");
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

  // printTimeSeries(numberOfPapersSeries);
  // printTimeSeries(citationSeries);
  // printTimeSeries(impactFactorSeries);
}
