#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "VirtualDrive.h"
#include <QStandardItemModel>  // Needed for QListView

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddFileClicked();
    void onDeleteFileClicked();
    void updateFileList();
    void fadeInListView();

private:
    bool authenticateUser(); // New: Authentication method

    Ui::MainWindow *ui;
    VirtualDrive *drive;
    QStandardItemModel *fileModel; // Added for QListView model
};

#endif // MAINWINDOW_H
