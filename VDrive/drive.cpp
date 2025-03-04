#include "VirtualDrive.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>

VirtualDrive::VirtualDrive(const QString& filename, int size, QObject* parent)
    : QObject(parent), driveSize(size), nextFreeOffset(0) {

    driveFile.setFileName(filename);
    if (!driveFile.open(QIODevice::ReadWrite)) {
        qDebug() << "Error: Could not create or open virtual drive file!";
        return;
    }

    if (driveFile.size() == 0) {
        qDebug() << "Initializing new virtual drive.";
        driveFile.resize(driveSize);
    }

    loadMetadata();
}

VirtualDrive::~VirtualDrive() {
    saveMetadata();
}

void VirtualDrive::addFile(const QString& filename, const QByteArray& data) {
    if (nextFreeOffset + data.size() > driveSize) {
        qDebug() << "Error: Not enough space!";
        return;
    }

    FileNode newFile = {filename, static_cast<int>(data.size()), nextFreeOffset};
    fileDirectory.append(newFile);

    qDebug() << "File added successfully: " << filename;
    qDebug() << "Total files in memory after adding: " << fileDirectory.size();

    if (driveFile.open(QIODevice::ReadWrite)) {
        driveFile.seek(nextFreeOffset);
        driveFile.write(data);
        driveFile.close();
    }

    nextFreeOffset += data.size();
    saveMetadata();  // Save updated metadata
    emit fileListUpdated();
}

void VirtualDrive::deleteFile(const QString& filename) {
    auto it = std::find_if(fileDirectory.begin(), fileDirectory.end(), [&](const FileNode& file) {
        return file.name == filename;
    });

    if (it != fileDirectory.end()) {
        fileDirectory.erase(it);
        saveMetadata();
        emit fileListUpdated();
        qDebug() << "File deleted successfully: " << filename;
    } else {
        qDebug() << "Error: File not found!";
    }
}

QVector<FileNode> VirtualDrive::listFiles() const {
    qDebug() << "Fetching files from virtual drive. Total files stored:" << fileDirectory.size();

    for (const auto& file : fileDirectory) {
        qDebug() << "Stored file:" << file.name;
    }

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

    for (const auto& file : fileDirectory) {
        out << file.name << file.size << file.offset;
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
        nextFreeOffset = 0;  // Start fresh
        return;
    }

    QDataStream in(&metaFile);
    int count;
    in >> count;
    fileDirectory.clear();
    nextFreeOffset = 0;  // Ensure we start at 0

    for (int i = 0; i < count; ++i) {
        FileNode file;
        in >> file.name >> file.size >> file.offset;
        fileDirectory.append(file);

        nextFreeOffset = qMax(nextFreeOffset, file.offset + file.size);
        qDebug() << "Loaded file from metadata: " << file.name;
    }

    metaFile.close();
    qDebug() << "Metadata loaded successfully. Total files: " << fileDirectory.size();
}
