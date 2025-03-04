#ifndef VIRTUALDRIVE_H
#define VIRTUALDRIVE_H

#include <QObject>
#include <QFile>
#include <QVector>

struct FileNode {
    QString name;
    int size;
    int offset;
};

class VirtualDrive : public QObject {
    Q_OBJECT

public:
    explicit VirtualDrive(const QString& filename, int size, QObject* parent = nullptr);
    ~VirtualDrive();

    void addFile(const QString& filename, const QByteArray& data);
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

#endif // VIRTUAL
