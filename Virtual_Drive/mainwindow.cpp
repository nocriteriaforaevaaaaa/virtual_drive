#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , drive(new VirtualDrive("my_drive.vd", 1024 * 1024 * 10)) // Initialize with your drive parameters
{
    ui->setupUi(this);

    // Connect buttons to their respective slots
    connect(ui->uploadButton, &QPushButton::clicked, this, &MainWindow::on_uploadButton_clicked);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::on_deleteButton_clicked);

    refreshFileList();
}

MainWindow::~MainWindow()
{
    delete drive;
    delete ui;
}

void MainWindow::on_uploadButton_clicked()
{
    // Open file dialog to select file to upload
    QString filePath = QFileDialog::getOpenFileName(this, "Select a file to upload");

    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            vector<char> data(fileData.begin(), fileData.end());

            // Extract file name from file path
            QString fileName = QFileInfo(filePath).fileName();

            // Add the file to the virtual drive
            drive->addFile(fileName.toStdString(), data);

            // Refresh the file list
            refreshFileList();

            QMessageBox::information(this, "Upload Success", "File uploaded successfully.");
        } else {
            QMessageBox::warning(this, "Upload Failed", "Failed to open the file.");
        }
    }
}

void MainWindow::on_deleteButton_clicked()
{
    // Get the selected file name
    QListWidgetItem *selectedItem = ui->fileListWidget->currentItem();
    if (selectedItem) {
        QString fileName = selectedItem->text();

        // Delete the file from the virtual drive
        drive->deleteFile(fileName.toStdString());

        // Refresh the file list
        refreshFileList();

        QMessageBox::information(this, "Delete Success", "File deleted successfully.");
    } else {
        QMessageBox::warning(this, "Delete Failed", "Please select a file to delete.");
    }
}

void MainWindow::refreshFileList()
{
    ui->fileListWidget->clear();

    // List files in the virtual drive
    drive->listFiles();
    for (const auto& file : drive->getFileDirectory()) {
        ui->fileListWidget->addItem(QString::fromStdString(file.name));
    }
}
