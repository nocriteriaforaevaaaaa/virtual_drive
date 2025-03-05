#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "VirtualDrive.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QPropertyAnimation>
#include <QInputDialog>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDebug>
#include <QtCrypto> // Ensure QCA is installed and set up

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , drive(new VirtualDrive("my_drive.vd", 1024 * 1024 * 100, this)) // 100MB virtual drive
{
    ui->setupUi(this);
    setWindowTitle("Virtual Drive Manager");

    // Initialize the file model and attach it to the list view
    fileModel = new QStandardItemModel(this);
    ui->listFiles->setModel(fileModel);


    setStyleSheet(R"(
    /* Main window background */
    QMainWindow {
        background-color: #ffefd5;  /* PapayaWhip */
    }

    /* General widget styling */
    QWidget {
        font-family: "Segoe UI", sans-serif;
        font-size: 14px;
    }

    /* Buttons with bright colors */
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

    /* List view styling */
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

    /* Optional: Customize other controls as needed */
    QLineEdit {
        background-color: #fffacd;  /* LemonChiffon */
        border: 2px solid #ffa500;  /* Orange */
        border-radius: 4px;
        padding: 5px;
    }
)");


    // Connect buttons to slots
    connect(ui->btnAddFile, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->btnDeleteFile, &QPushButton::clicked, this, &MainWindow::onDeleteFileClicked);
    connect(drive, &VirtualDrive::fileListUpdated, this, &MainWindow::updateFileList);

    // Load any existing files
    updateFileList();
}

MainWindow::~MainWindow() {
    delete ui;
}

// Authentication method: prompts for a password before proceeding.
bool MainWindow::authenticateUser() {
    bool ok;
    QString password = QInputDialog::getText(this, tr("Authentication"),
                                             tr("Enter password:"), QLineEdit::Password,
                                             "", &ok);
    if (!ok) {  // User cancelled the dialog
        return false;
    }

    // Check the entered password against a preset value.
    if (password == "aeva1234") {  // Replace with secure verification
        return true;
    } else {
        QMessageBox::warning(this, tr("Authentication Failed"),
                             tr("Incorrect password. Access denied."));
        return false;
    }
}

void MainWindow::onAddFileClicked() {
    // Authenticate before proceeding with file upload
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

        // Compute a hash for integrity verification using SHA-256
        QByteArray fileHash = QCryptographicHash::hash(plainData, QCryptographicHash::Sha256);

        // Encrypt the data using QCA with AES-256 in CBC mode.
        QCA::Initializer init;
        // For AES-256, you need a 32-byte key. Example key (32 bytes):
        QByteArray keyBytes = QByteArray("12345678901234567890123456789012", 32);

        // For AES in CBC mode, you typically need a 16-byte IV. Example IV (16 bytes):
        QByteArray ivBytes = QByteArray("1234567890123456", 16);

        // Now pass these QByteArrays to the QCA constructors
        QCA::SymmetricKey key(keyBytes);
        QCA::InitializationVector iv(ivBytes);

        QCA::Cipher cipher(QString("aes256"), QCA::Cipher::CBC,
                           QCA::Cipher::DefaultPadding, QCA::Encode, key, iv);
        QByteArray encryptedData = cipher.process(plainData).toByteArray();

        // Add the file (encrypted data and hash) to the virtual drive.
        drive->addFile(originalFileName, encryptedData, fileHash);

        updateFileList();

        // Notify user of successful upload.
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

void MainWindow::onDeleteFileClicked() {
    // Authenticate before allowing file deletion.
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

    // Force UI update and apply a fade-in animation.
    ui->listFiles->setModel(nullptr);
    ui->listFiles->setModel(fileModel);
    ui->listFiles->update();
    fadeInListView();
    qDebug() << "File list UI updated.";
}

void MainWindow::fadeInListView() {
    QPropertyAnimation *animation = new QPropertyAnimation(ui->listFiles, "windowOpacity");
    animation->setDuration(500); // Animation duration: 500ms
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
