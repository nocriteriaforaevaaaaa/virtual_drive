#ifndef VIRTUALDRIVE_H
#define VIRTUALDRIVE_H

#include <QObject>
#include <QFile>
#include <QVector>
#include <QByteArray>

// File metadata structure
struct FileNode {
    QString name;
    int size;
    int offset;
    QByteArray hash;  // For storing file hash if needed
};

class VirtualDrive : public QObject {
    Q_OBJECT

public:
    explicit VirtualDrive(const QString &filename, int size, QObject *parent = nullptr);
    ~VirtualDrive();

    // Adds a file with encrypted data and its hash
    void addFile(const QString &filename, const QByteArray &data, const QByteArray &hash);

    // Deletes a file from the virtual drive
    void deleteFile(const QString &filename);

    // Reads (encrypted) data from the virtual drive
    QByteArray readFile(const QString &filename);

    // Returns the list of files in the drive
    QVector<FileNode> listFiles() const;
    void addFile(const QString& filename, const QByteArray& data);



    // New search method
    QVector<FileNode> searchFiles(const QString& query, bool caseSensitive = false) const;

signals:
    void fileListUpdated();

private:
    QString driveFilename;
    int driveSize;
    int nextFreeOffset;
    QVector<FileNode> fileDirectory;
    QFile driveFile; // The container file for storing data

    void saveMetadata();
    void loadMetadata();
};

#endif // VIRTUALDRIVE_H
