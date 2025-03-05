#include "VirtualDrive.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <algorithm>

VirtualDrive::VirtualDrive(const QString& filename, int size, QObject* parent)
    : QObject(parent), driveSize(size), nextFreeOffset(0), driveFile(filename)
{
    if (!driveFile.open(QIODevice::ReadWrite)) {
        qDebug() << "Error: Could not create or open virtual drive file!";
        return;
    }
    if (driveFile.size() == 0) {
        driveFile.resize(driveSize);
    }
    loadMetadata();
}

VirtualDrive::~VirtualDrive() {
    saveMetadata();
}

void VirtualDrive::addFile(const QString& filename, const QByteArray& data) {
    qDebug() << "Attempting to add file:" << filename << "Size:" << data.size();

    if (nextFreeOffset + data.size() > driveSize) {
        qDebug() << "Error: Not enough space!";
        return;
    }

    FileNode newFile = { filename, static_cast<int>(data.size()), nextFreeOffset };
    fileDirectory.append(newFile);

    if (driveFile.open(QIODevice::ReadWrite)) {
        driveFile.seek(nextFreeOffset);
        driveFile.write(data);
        driveFile.close();
    }

    nextFreeOffset += data.size();
    saveMetadata();

    emit fileListUpdated();

    qDebug() << "File added successfully. Total files now:" << fileDirectory.size();
}

void VirtualDrive::deleteFile(const QString& filename) {
    auto it = std::find_if(fileDirectory.begin(), fileDirectory.end(), [&](const FileNode& f) {
        return f.name == filename;
    });
    if (it != fileDirectory.end()) {
        fileDirectory.erase(it);
        saveMetadata();
        emit fileListUpdated();
        qDebug() << "File deleted:" << filename;
    } else {
        qDebug() << "File not found:" << filename;
    }
}

QVector<FileNode> VirtualDrive::listFiles() const {
    QVector<FileNode> filteredFiles;
    for (const auto& file : fileDirectory) {
        // Filter out system files: metadata.dat and the virtual drive file itself.
        if (file.name != "metadata.dat" && file.name != driveFile.fileName())
            filteredFiles.append(file);
    }
    qDebug() << "Fetching files. Total user files:" << filteredFiles.size();
    return filteredFiles;
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
    qDebug() << "Metadata saved.";
}

void VirtualDrive::loadMetadata() {
    QFile metaFile("metadata.dat");
    if (!metaFile.open(QIODevice::ReadOnly)) {
        qDebug() << "No metadata found. Starting fresh.";
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
        in >> file.name >> file.size >> file.offset;
        // Skip system files so that only user-uploaded files are loaded
        if (file.name == "metadata.dat" || file.name == driveFile.fileName())
            continue;
        fileDirectory.append(file);
        nextFreeOffset = qMax(nextFreeOffset, file.offset + file.size);
        qDebug() << "Loaded file from metadata:" << file.name;
    }
    metaFile.close();
    qDebug() << "Metadata loaded. Total user files:" << fileDirectory.size();
}
