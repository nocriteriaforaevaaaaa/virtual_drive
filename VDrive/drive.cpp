#include "VirtualDrive.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QInputDialog>
#include <algorithm>

VirtualDrive::VirtualDrive(const QString &filename, int size, QObject *parent)
    : QObject(parent),
    driveFilename(filename),
    driveSize(size),
    nextFreeOffset(0)
{
    driveFile.setFileName(driveFilename);

    // Attempt to open or create the drive file
    if (!driveFile.open(QIODevice::ReadWrite)) {
        qDebug() << "Error: Could not create or open virtual drive file!";
        return;
    }

    // If the file is empty, resize it to the specified size
    if (driveFile.size() == 0) {
        qDebug() << "Initializing new virtual drive.";
        driveFile.resize(driveSize);
    }
    driveFile.close();

    loadMetadata();
}

VirtualDrive::~VirtualDrive() {
    // Save metadata before shutting down
    saveMetadata();
}

// Add a file with encrypted data and its hash
void VirtualDrive::addFile(const QString &filename, const QByteArray &data, const QByteArray &hash) {
    QString newFilename = filename;

    // Check if file with this name already exists
    bool nameExists = std::any_of(fileDirectory.begin(), fileDirectory.end(),
                                  [&](const FileNode &file){ return file.name == newFilename; });

    // If a file with the same name exists, prompt for a new name
    if (nameExists) {
        bool ok;
        newFilename = QInputDialog::getText(nullptr, "Rename File",
                                            "File with this name already exists. Enter a new name:",
                                            QLineEdit::Normal, filename, &ok);
        if (!ok || newFilename.isEmpty()) {
            qDebug() << "File addition cancelled.";
            return;
        }

        // Check again if the new name is also taken
        while (std::any_of(fileDirectory.begin(), fileDirectory.end(),
                           [&](const FileNode &file){ return file.name == newFilename; })) {
            newFilename = QInputDialog::getText(nullptr, "Rename File",
                                                "That name is also taken. Enter another name:",
                                                QLineEdit::Normal, newFilename, &ok);
            if (!ok || newFilename.isEmpty()) {
                qDebug() << "File addition cancelled.";
                return;
            }
        }
    }

    // Check if there's enough space in the drive
    if (nextFreeOffset + data.size() > driveSize) {
        qDebug() << "Error: Not enough space in the virtual drive!";
        return;
    }

    // Create a FileNode entry
    FileNode newFile;
    newFile.name = newFilename;
    newFile.size = data.size();
    newFile.offset = nextFreeOffset;
    newFile.hash = hash;
    fileDirectory.append(newFile);

    // Write the encrypted data to the drive file
    if (driveFile.open(QIODevice::ReadWrite)) {
        driveFile.seek(nextFreeOffset);
        driveFile.write(data);
        driveFile.close();
    } else {
        qDebug() << "Error: Could not open drive file for writing.";
        return;
    }

    // Update the offset
    nextFreeOffset += data.size();

    // Save metadata and notify listeners
    saveMetadata();
    emit fileListUpdated();

    qDebug() << "File added successfully:" << newFilename;
}

void VirtualDrive::deleteFile(const QString &filename) {
    auto it = std::find_if(fileDirectory.begin(), fileDirectory.end(),
                           [&](const FileNode &file){ return file.name == filename; });
    if (it != fileDirectory.end()) {
        fileDirectory.erase(it);
        saveMetadata();
        emit fileListUpdated();
        qDebug() << "File deleted successfully:" << filename;
    } else {
        qDebug() << "Error: File not found!";
    }
}

QByteArray VirtualDrive::readFile(const QString &filename) {
    // Find the file in the directory
    auto it = std::find_if(fileDirectory.begin(), fileDirectory.end(),
                           [&](const FileNode &file){ return file.name == filename; });
    if (it == fileDirectory.end()) {
        qDebug() << "File not found in virtual drive:" << filename;
        return QByteArray();
    }

    // Open the drive file
    if (!driveFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Unable to open drive file for reading";
        return QByteArray();
    }

    qDebug() << "Reading file:" << it->name
             << "Offset:" << it->offset
             << "Size:" << it->size;

    // Seek and read the encrypted data
    driveFile.seek(it->offset);
    QByteArray data = driveFile.read(it->size);
    driveFile.close();

    qDebug() << "Read data length:" << data.size();
    return data; // This data is still encrypted; decryption happens in MainWindow
}

QVector<FileNode> VirtualDrive::listFiles() const {
    return fileDirectory;
}

void VirtualDrive::saveMetadata() {
    QFile metaFile("metadata.dat");
    if (!metaFile.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open metadata.dat for writing!";
        return;
    }

    QDataStream out(&metaFile);
    out << fileDirectory.size();

    for (const auto &file : fileDirectory) {
        out << file.name << file.size << file.offset << file.hash;
        qDebug() << "Saving file to metadata:" << file.name;
    }

    metaFile.close();
    qDebug() << "Metadata saved successfully.";
}

void VirtualDrive::loadMetadata() {
    QFile metaFile("metadata.dat");
    if (!metaFile.open(QIODevice::ReadOnly)) {
        qDebug() << "No metadata file found. Initializing empty drive.";
        fileDirectory.clear();
        nextFreeOffset = 0;
        return;
    }

    QDataStream in(&metaFile);
    int count;
    in >> count;
    fileDirectory.clear();
    nextFreeOffset = 0;

    for (int i = 0; i < count; ++i) {
        FileNode file;
        in >> file.name >> file.size >> file.offset >> file.hash;
        fileDirectory.append(file);

        // Ensure nextFreeOffset is at least beyond this file
        nextFreeOffset = qMax(nextFreeOffset, file.offset + file.size);
        qDebug() << "Loaded file from metadata:" << file.name;
    }

    metaFile.close();
    qDebug() << "Metadata loaded successfully. Total files:" << fileDirectory.size();
}
