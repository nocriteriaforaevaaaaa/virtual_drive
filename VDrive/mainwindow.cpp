#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "VirtualDrive.h"

#include <QDesktopServices>
#include <QStandardPaths>
#include <QUrl>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QPropertyAnimation>
#include <QInputDialog>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QtCrypto> // Ensure QCA is installed and properly linked

// Hardcoded 32-byte key (for AES-256) and 16-byte IV (for CBC mode).
static const QByteArray encryptionKeyBytes("12345678901234567890123456789012", 32);
static const QByteArray encryptionIVBytes("1234567890123456", 16);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , drive(new VirtualDrive("my_drive.vd", 1024 * 1024 * 100, this)) // 100MB virtual drive
{
    ui->setupUi(this);
    setWindowTitle("Virtual Drive Manager");


    fileModel = new QStandardItemModel(this);
    ui->listFiles->setModel(fileModel);


    setStyleSheet(R"(
        QMainWindow {
            background-color: #ffefd5;  /* PapayaWhip */
        }
        QWidget {
            font-family: "Segoe UI", sans-serif;
            font-size: 14px;
        }
        QPushButton {
            background-color: #ff6347;  /* Tomato */
            border: none;
            border-radius: 8px;
            padding: 10px;
            color: white;
        }
        QPushButton:hover {
            background-color: #ff4500;  /* OrangeRed */
        }
        QPushButton:pressed {
            background-color: #cd3700;
        }
        QListView {
            background-color: #e0ffff;  /* LightCyan */
            border: 2px solid #20b2aa;  /* LightSeaGreen */
            border-radius: 5px;
            padding: 5px;
        }
        QListView::item {
            padding: 5px;
        }
        QListView::item:selected {
            background-color: #ffeb3b;  /* Vibrant Yellow */
            color: black;
        }
        QLineEdit {
            background-color: #fffacd;  /* LemonChiffon */
            border: 2px solid #ffa500;  /* Orange */
            border-radius: 4px;
            padding: 5px;
        }
    )");


    connect(ui->btnAddFile, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->btnDeleteFile, &QPushButton::clicked, this, &MainWindow::onDeleteFileClicked);
    connect(ui->btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);


    connect(drive, &VirtualDrive::fileListUpdated, this, &MainWindow::updateFileList);


    updateFileList();
}

MainWindow::~MainWindow() {
    delete ui;
}

// Prompt the user for a password. Return true if the user enters the correct one.
bool MainWindow::authenticateUser() {
    bool ok;
    QString password = QInputDialog::getText(this, tr("Authentication"),
                                             tr("Enter password:"), QLineEdit::Password,
                                             "", &ok);
    if (!ok) { // User cancelled
        return false;
    }

    // Hardcoded password for demonstration
    if (password == "aeva1234") {
        return true;
    } else {
        QMessageBox::warning(this, tr("Authentication Failed"),
                             tr("Incorrect password. Access denied."));
        return false;
    }
}

// Upload a file: read it, hash it, encrypt it, store in the virtual drive
void MainWindow::onAddFileClicked() {
    // Ask for password before upload
    if (!authenticateUser()) {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "Select a file to add");
    if (filePath.isEmpty()) {
        qDebug() << "File selection canceled.";
        return;
    }

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray plainData = file.readAll();
        QString originalFileName = QFileInfo(filePath).fileName();

        // Hash the file data
        QByteArray fileHash = QCryptographicHash::hash(plainData, QCryptographicHash::Sha256);

        // Encrypt the data
        QCA::Initializer init;
        QCA::SymmetricKey key(encryptionKeyBytes);
        QCA::InitializationVector iv(encryptionIVBytes);
        QCA::Cipher encryptCipher("aes256", QCA::Cipher::CBC,
                                  QCA::Cipher::DefaultPadding, QCA::Encode, key, iv);
        QByteArray encryptedData = encryptCipher.process(plainData).toByteArray();

        // Store the encrypted data in the virtual drive
        drive->addFile(originalFileName, encryptedData, fileHash);

        updateFileList();

        // Notify user
        QVector<FileNode> files = drive->listFiles();
        if (!files.isEmpty()) {
            QString finalFileName = files.last().name;
            QMessageBox::information(this, "Upload Successful",
                                     "File '" + finalFileName + "' has been uploaded successfully.");
        }
    } else {
        QMessageBox::warning(this, "Upload Failed", "Could not open the selected file.");
    }
}

// Delete the selected file
void MainWindow::onDeleteFileClicked() {
    // Ask for password before deleting
    if (!authenticateUser()) {
        return;
    }

    QModelIndex selectedIndex = ui->listFiles->currentIndex();
    if (!selectedIndex.isValid()) {
        qDebug() << "No file selected for deletion.";
        return;
    }

    QString fileName = selectedIndex.data().toString();
    qDebug() << "Deleting file:" << fileName;

    drive->deleteFile(fileName);
    updateFileList();

    QMessageBox::information(this, "Delete Successful",
                             "File '" + fileName + "' has been deleted successfully.");
}

// Open the selected file: read encrypted data, decrypt, write to temp, open with default app
void MainWindow::onOpenFileClicked() {
    QModelIndex selectedIndex = ui->listFiles->currentIndex();
    if (!selectedIndex.isValid()) {
        QMessageBox::warning(this, "Open File", "No file selected.");
        return;
    }

    QString fileName = selectedIndex.data().toString();
    qDebug() << "Opening file:" << fileName;

    // Read the encrypted data from the drive
    QByteArray encryptedData = drive->readFile(fileName);
    if (encryptedData.isEmpty()) {
        QMessageBox::warning(this, "Open File", "Failed to read file data.");
        return;
    }

    // Decrypt the data
    QCA::Initializer init;
    QCA::SymmetricKey key(encryptionKeyBytes);
    QCA::InitializationVector iv(encryptionIVBytes);
    QCA::Cipher decryptCipher("aes256", QCA::Cipher::CBC,
                              QCA::Cipher::DefaultPadding, QCA::Decode, key, iv);
    QByteArray decryptedData = decryptCipher.process(encryptedData).toByteArray();

    if (decryptedData.isEmpty()) {
        QMessageBox::warning(this, "Open File", "Failed to decrypt file data.");
        return;
    }

    // Write the decrypted data to a temporary file
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFilePath = tempDir + "/" + fileName;
    QFile tempFile(tempFilePath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Open File", "Failed to create temporary file.");
        return;
    }
    tempFile.write(decryptedData);
    tempFile.close();

    // Open the file with the system's default application
    QDesktopServices::openUrl(QUrl::fromLocalFile(tempFilePath));
}

// Refresh the list of files in the QListView
void MainWindow::updateFileList() {
    fileModel->clear();
    qDebug() << "Updating file list...";
    const auto fileList = drive->listFiles();
    qDebug() << "Total files found:" << fileList.size();

    for (const auto &file : fileList) {
        qDebug() << "Adding file to UI:" << file.name;
        QStandardItem *item = new QStandardItem(file.name);
        fileModel->appendRow(item);
    }

    // Force UI update and apply a fade-in animation
    ui->listFiles->setModel(nullptr);
    ui->listFiles->setModel(fileModel);
    ui->listFiles->update();
    fadeInListView();

    qDebug() << "File list UI updated.";
}

// Simple fade-in animation for the QListView
void MainWindow::fadeInListView() {
    QPropertyAnimation *animation = new QPropertyAnimation(ui->listFiles, "windowOpacity");
    animation->setDuration(500);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
