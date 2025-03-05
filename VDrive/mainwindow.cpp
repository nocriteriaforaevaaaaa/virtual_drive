#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QPropertyAnimation>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), drive(new VirtualDrive("my_drive.vd", 1024 * 1024 * 100, this))
{
    ui->setupUi(this);
    setWindowTitle("Virtual Drive Manager");
    fileModel = new QStandardItemModel(this);
    ui->listFiles->setModel(fileModel);
    setStyleSheet(R"(
        QWidget {
            background-color: #2E3440;
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
            background-color: #88C0D0;
            color: black;
        }
        QListView {
            background-color: #3B4252;
            color: white;
            border: 1px solid #4C566A;
        }
        QListView::item:selected {
            background-color: #81A1C1;
            color: black;
        }
    )");
    connect(ui->btnAddFile, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->btnDeleteFile, &QPushButton::clicked, this, &MainWindow::onDeleteFileClicked);
    connect(drive, &VirtualDrive::fileListUpdated, this, &MainWindow::updateFileList);
    updateFileList();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onAddFileClicked() {

    QString filePath = QFileDialog::getOpenFileName(this, "Select a file to add");
    if (filePath.isEmpty())
        return;
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QString fileName = QFileInfo(filePath).fileName();
        drive->addFile(fileName, data);
        QTimer::singleShot(100, this, &MainWindow::updateFileList);
        QMessageBox::information(this, "Upload Successful", "File '" + fileName + "' has been uploaded successfully.");
    } else {
        QMessageBox::warning(this, "Upload Failed", "Could not open the selected file.");
    }
}

void MainWindow::onDeleteFileClicked() {
    QModelIndex selectedIndex = ui->listFiles->currentIndex();
    if (!selectedIndex.isValid())
        return;
    QString fileName = selectedIndex.data().toString();
    drive->deleteFile(fileName);
    updateFileList();
    QMessageBox::information(this, "Delete Successful", "File '" + fileName + "' has been deleted successfully.");
}

void MainWindow::updateFileList() {
    fileModel->clear();


    QVector<QString> driveFiles;
    const auto fileList = drive->listFiles();

    for (const auto& file : fileList) {
        fileModel->appendRow(new QStandardItem(file.name));
    }
    ui->listFiles->setModel(fileModel);
    fadeInListView();
}

void MainWindow::fadeInListView() {
    QPropertyAnimation *animation = new QPropertyAnimation(ui->listFiles, "windowOpacity");
    animation->setDuration(500);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
