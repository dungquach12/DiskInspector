#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "DiskInspector/fat32.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class FileTreeWidgetItem : public QTreeWidgetItem {
public:
    FileTreeWidgetItem(const File& file) : QTreeWidgetItem() {
        setData(0, Qt::UserRole, QVariant::fromValue((qulonglong)new File(file)));
    }
    ~FileTreeWidgetItem() override {
        File* f = reinterpret_cast<File*>(data(0, Qt::UserRole).toULongLong());
        delete f;
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_diskInputButton_clicked();
    void on_fileTree_itemExpanded(QTreeWidgetItem *item);
    void on_fileTree_itemPressed(QTreeWidgetItem *item, int column);

private:
    Ui::MainWindow *ui;
    FATbootSector disk;

    QTreeWidgetItem* createTreeItem(const File& file);
    void loadDirectory(QTreeWidgetItem *parent, int cluster);
};
#endif // MAINWINDOW_H
