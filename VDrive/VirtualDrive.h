#ifndef VIRTUALDRIVE_H
#define VIRTUALDRIVE_H

#include <QObject>
#include <QFile>
#include <QVector>
#include <QByteArray>  // Added for handling QByteArray

struct FileNode {
    QString name;
    int size;
    int offset;
    QByteArray hash; // Added: stores the file's hash for integrity verification
};

class VirtualDrive : public QObject {
    Q_OBJECT

public:
    explicit VirtualDrive(const QString& filename, int size, QObject* parent = nullptr);
    ~VirtualDrive();

    // Updated addFile to accept a hash parameter along with file data.
    void addFile(const QString& filename, const QByteArray& data, const QByteArray& hash);
    void deleteFile(const QString& filename);
    QVector<FileNode> listFiles() const;

signals:
    void fileListUpdated();

private:
    QString driveFilename;
    int driveSize;
    int nextFreeOffset;
    QVector<FileNode> fileDirectory;

    QFile driveFile;

    void saveMetadata();
    void loadMetadata();
};

#endif // VIRTUALDRIVE_H
