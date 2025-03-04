#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QPropertyAnimation>
#include <QDebug>  // Added for debugging

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), drive(new VirtualDrive("my_drive.vd", 1024 * 1024 * 100, this)) {  // Increased to 100MB
    ui->setupUi(this);

    // Set window title
    setWindowTitle("Virtual Drive Manager");

    // Initialize model for QListView
    fileModel = new QStandardItemModel(this);
    ui->listFiles->setModel(fileModel);  // Attach model to QListView

    // Apply a modern UI style
    setStyleSheet(R"(
        QWidget {
            background-color: #2E3440;  /* Dark theme */
            color: white;
            font-size: 14px;
        }
        QPushButton {
            background-color: #4C566A;
            color: white;
            border-radius: 5px;
            padding: 5px;
        }
        QPushButton:hover {
            background-color: #88C0D0; /* Light blue on hover */
            color: black;
        }
        QListView {
            background-color: #3B4252;
            color: white;
            border: 1px solid #4C566A;
        }
        QListView::item:selected {
            background-color: #81A1C1; /* Highlight selected item */
            color: black;
        }
    )");

    // Connect buttons to slots
    connect(ui->btnAddFile, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->btnDeleteFile, &QPushButton::clicked, this, &MainWindow::onDeleteFileClicked);
    connect(drive, &VirtualDrive::fileListUpdated, this, &MainWindow::updateFileList);

    // Load and display existing files on startup
    updateFileList();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onAddFileClicked() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select a file to add");
    if (filePath.isEmpty()) {
        qDebug() << "File selection canceled.";
        return;  // User canceled selection
    }

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QString fileName = QFileInfo(filePath).fileName(); // Extract only the file name

        drive->addFile(fileName, data);  // Add file to virtual drive
        qDebug() << "File added: " << fileName;

        updateFileList();  // Force immediate UI refresh after adding

        QMessageBox::information(this, "Upload Successful", "File '" + fileName + "' has been uploaded successfully.");
    } else {
        QMessageBox::warning(this, "Upload Failed", "Could not open the selected file.");
    }
}

void MainWindow::onDeleteFileClicked() {
    QModelIndex selectedIndex = ui->listFiles->currentIndex();
    if (!selectedIndex.isValid()) {
        qDebug() << "No file selected for deletion.";
        return;  // No file selected
    }

    QString fileName = selectedIndex.data().toString();
    qDebug() << "Deleting file: " << fileName;

    drive->deleteFile(fileName);
    updateFileList();  // Refresh UI after deleting

    QMessageBox::information(this, "Delete Successful", "File '" + fileName + "' has been deleted successfully.");
}

void MainWindow::updateFileList() {
    fileModel->clear();  // Clear previous entries

    qDebug() << "Updating file list...";
    const auto fileList = drive->listFiles();
    qDebug() << "Total files found: " << fileList.size();

    if (fileList.isEmpty()) {
        qDebug() << "No files available.";
    }

    for (const auto& file : fileList) {
        qDebug() << "Adding file to UI: " << file.name;

        QStandardItem *item = new QStandardItem(file.name);
        fileModel->appendRow(item);
    }

    // Force UI refresh
    ui->listFiles->setModel(nullptr);
    ui->listFiles->setModel(fileModel);
    ui->listFiles->update();  // Force an update

    // Apply fade-in animation
    fadeInListView();

    qDebug() << "File list UI updated.";
}

void MainWindow::fadeInListView() {
    QPropertyAnimation *animation = new QPropertyAnimation(ui->listFiles, "windowOpacity");
    animation->setDuration(500);  // Animation duration: 500ms
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
