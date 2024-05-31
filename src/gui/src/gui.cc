#include "gui.hh"

#include <QApplication>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      textBox(new QLineEdit(this)),
      listView(new QListView(this)),
      model(new QStringListModel(this)) {
  // Set up the layout
  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);
  layout->addWidget(textBox);
  layout->addWidget(listView);
  setCentralWidget(centralWidget);

  // Set the model for the list view
  listView->setModel(model);

  // Connect the text changed signal to the slot
  connect(textBox, &QLineEdit::textChanged, this, &MainWindow::onTextChanged);
}

MainWindow::~MainWindow() {}

void MainWindow::onTextChanged(const QString &text) {
  std::string word = text.toStdString();
  QStringList keywords = searchKeywords(word);
  model->setStringList(keywords);
}

QStringList MainWindow::searchKeywords(const std::string &word) {
  // Placeholder for the actual search logic
  QStringList keywords;
  // Example implementation (to be replaced with actual logic)
  keywords << "Keyword1"
           << "Keyword2"
           << "Keyword3";
  return keywords;
}

void runGui(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow window;
  window.show();
}
