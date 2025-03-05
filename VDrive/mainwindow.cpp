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
#include <QtCrypto>
#include <QToolBar>
#include <QComboBox>
#include <QLineEdit>
#include <QDebug>
#include <algorithm>

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

    // Create a toolbar with sorting and search widgets.
    toolBar = new QToolBar("Options", this);
    addToolBar(toolBar);

    sortComboBox = new QComboBox(this);
    sortComboBox->addItem("Default");
    sortComboBox->addItem("Alphabetical");
    sortComboBox->addItem("File Size");

    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Search files...");
    searchLineEdit->setClearButtonEnabled(true);

    toolBar->addWidget(sortComboBox);
    toolBar->addWidget(searchLineEdit);

    // Initialize the file list model
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


    // Connect signals and slots.
    connect(ui->btnAddFile, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    connect(ui->btnDeleteFile, &QPushButton::clicked, this, &MainWindow::onDeleteFileClicked);
    connect(ui->btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    connect(sortComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSortOrderChanged);
    connect(searchLineEdit, &QLineEdit::textChanged,
            this, &MainWindow::onSearchTextChanged);
    connect(drive, &VirtualDrive::fileListUpdated, this, &MainWindow::updateFileList);

    // Initial file list update.
    updateFileList();
}

MainWindow::~MainWindow() {
    delete ui;
}

// Prompt the user for a password. Return true if the entered password is correct.
bool MainWindow::authenticateUser() {
    bool ok;
    QString password = QInputDialog::getText(this, tr("Authentication"),
                                             tr("Enter password:"), QLineEdit::Password,
                                             "", &ok);
    if (!ok) { // User cancelled.
        return false;
    }
    // Hardcoded password for demonstration.
    if (password == "aeva1234") {
        return true;
    } else {
        QMessageBox::warning(this, tr("Authentication Failed"),
                             tr("Incorrect password. Access denied."));
        return false;
    }
}

// Upload a file: read it, hash it, encrypt it, and store it in the virtual drive.
void MainWindow::onAddFileClicked() {
    // Ask for password before upload.
    if (!authenticateUser()) {
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "Select a file to add");
    if (filePath.isEmpty()) {
        qDebug() << "File selection canceled.";
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Upload Failed", "Could not open the selected file.");
        return;
    }

    QByteArray plainData = file.readAll();
    file.close();
    QString originalFileName = QFileInfo(filePath).fileName();

    // Hash the file data.
    QByteArray fileHash = QCryptographicHash::hash(plainData, QCryptographicHash::Sha256);

    // Encrypt the data.
    QCA::Initializer init;
    QCA::SymmetricKey key(encryptionKeyBytes);
    QCA::InitializationVector iv(encryptionIVBytes);
    QCA::Cipher encryptCipher("aes256", QCA::Cipher::CBC,
                              QCA::Cipher::DefaultPadding, QCA::Encode, key, iv);
    QByteArray encryptedData = encryptCipher.process(plainData).toByteArray();

    // Store the encrypted data in the virtual drive.
    drive->addFile(originalFileName, encryptedData, fileHash);
    updateFileList();

    // Notify user of success.
    QVector<FileNode> files = drive->listFiles();
    if (!files.isEmpty()) {
        QString finalFileName = files.last().name;
        QMessageBox::information(this, "Upload Successful",
                                 "File '" + finalFileName + "' has been uploaded successfully.");
    }
}

// Delete the selected file.
void MainWindow::onDeleteFileClicked() {
    // Ask for password before deletion.
    if (!authenticateUser()) {
        return;
    }

    QModelIndex selectedIndex = ui->listFiles->currentIndex();
    if (!selectedIndex.isValid()) {
        qDebug() << "No file selected for deletion.";
        return;
    }

    // The displayed file name might include extra details (like size); extract the actual name.
    QString displayName = selectedIndex.data().toString();
    QString fileName = displayName.split(" (").first();
    qDebug() << "Deleting file:" << fileName;

    drive->deleteFile(fileName);
    updateFileList();

    QMessageBox::information(this, "Delete Successful",
                             "File '" + fileName + "' has been deleted successfully.");
}

// Open the selected file: read encrypted data, decrypt, write to a temporary file, and open with the default app.
void MainWindow::onOpenFileClicked() {
    QModelIndex selectedIndex = ui->listFiles->currentIndex();
    if (!selectedIndex.isValid()) {
        QMessageBox::warning(this, "Open File", "No file selected.");
        return;
    }

    QString displayName = selectedIndex.data().toString();
    QString fileName = displayName.split(" (").first();
    qDebug() << "Opening file:" << fileName;

    // Read the encrypted data from the drive.
    QByteArray encryptedData = drive->readFile(fileName);
    if (encryptedData.isEmpty()) {
        QMessageBox::warning(this, "Open File", "Failed to read file data.");
        return;
    }

    // Decrypt the data.
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

    // Write the decrypted data to a temporary file.
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFilePath = tempDir + "/" + fileName;
    QFile tempFile(tempFilePath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Open File", "Failed to create temporary file.");
        return;
    }
    tempFile.write(decryptedData);
    tempFile.close();

    // Open the file with the system's default application.
    QDesktopServices::openUrl(QUrl::fromLocalFile(tempFilePath));
}

// Refresh the list of files in the QListView.
void MainWindow::updateFileList() {
    fileModel->clear();
    qDebug() << "Updating file list...";
    const auto fileList = drive->listFiles();
    qDebug() << "Total files found:" << fileList.size();

    // Display each file with its size (in KB).
    for (const auto &file : fileList) {
        QStandardItem *item = new QStandardItem(
            file.name + " (" + QString::number(file.size / 1024) + " KB)"
            );
        fileModel->appendRow(item);
    }

    ui->listFiles->setModel(fileModel);
    ui->listFiles->update();
    fadeInListView();

    qDebug() << "File list UI updated.";
}

// Slot to change the sort order based on the selected option.
void MainWindow::onSortOrderChanged(int index) {
    QVector<FileNode> files = drive->listFiles();

    if (index == 1) {  // Alphabetical.
        std::sort(files.begin(), files.end(),
                  [](const FileNode &a, const FileNode &b) { return a.name < b.name; });
    } else if (index == 2) {  // By Size.
        std::sort(files.begin(), files.end(),
                  [](const FileNode &a, const FileNode &b) { return a.size < b.size; });
    }

    fileModel->clear();
    for (const auto &file : files) {
        QStandardItem *item = new QStandardItem(
            file.name + " (" + QString::number(file.size / 1024) + " KB)"
            );
        fileModel->appendRow(item);
    }
}

// Simple fade-in animation for the QListView.
void MainWindow::fadeInListView() {
    QPropertyAnimation *animation = new QPropertyAnimation(ui->listFiles, "windowOpacity");
    animation->setDuration(500);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

// Filter the file list based on the search text.
void MainWindow::onSearchTextChanged(const QString& searchText) {
    if (!searchText.trimmed().isEmpty()) {
        QVector<FileNode> searchResults = drive->searchFiles(searchText, false);
        fileModel->clear();
        for (const auto &file : searchResults) {
            QStandardItem *item = new QStandardItem(
                file.name + " (" + QString::number(file.size / 1024) + " KB)"
                );
            fileModel->appendRow(item);
        }
    } else {
        updateFileList();
    }
}

void MainWindow::clearSearch() {
    searchLineEdit->clear();
    updateFileList();
}
