#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include "VirtualDrive.h"

// Forward declarations for classes we use as pointers
class QToolBar;
class QComboBox;
class QLineEdit;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
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
    void onSortOrderChanged(int index);
    void onSearchTextChanged(const QString &searchText);
    void clearSearch();

private:
    bool authenticateUser();

    Ui::MainWindow *ui;
    VirtualDrive *drive;
    QStandardItemModel *fileModel;

    // Added UI elements for the toolbar, sorting, and searching
    QToolBar *toolBar;
    QComboBox *sortComboBox;
    QLineEdit *searchLineEdit;
};

#endif // MAINWINDOW_H
