#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include "VirtualDrive.h"  // Include your VirtualDrive class here

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_uploadButton_clicked();
    void on_deleteButton_clicked();

private:
    Ui::MainWindow *ui;
    VirtualDrive *drive;
    void refreshFileList();
};
#endif // MAINWINDOW_H
