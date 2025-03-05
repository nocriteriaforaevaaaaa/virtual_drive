#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include "VirtualDrive.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddFileClicked();
    void onDeleteFileClicked();
    void onOpenFileClicked();
    void updateFileList();
    void fadeInListView();

private:
    bool authenticateUser();

    Ui::MainWindow *ui;
    VirtualDrive *drive;
    QStandardItemModel *fileModel;
};

#endif
