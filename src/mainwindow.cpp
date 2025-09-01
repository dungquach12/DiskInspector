#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DiskInspector/fat32.h"
#include "DiskInspector/helper.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->fileTree->setColumnWidth(0,520);
}

MainWindow::~MainWindow()
{
    delete ui;
}


QTreeWidgetItem* MainWindow::createTreeItem(const File& file) {
    FileTreeWidgetItem *item = new FileTreeWidgetItem(file);
    QString name = QString::fromStdString(file.fileName);
    item->setText(0, name);
    if (!file.fileExtension.empty()) {
        item->setText(1, QString::fromStdString(file.fileExtension));
    }

    if (file.fileSize) { item->setText(2, QString::fromStdString(convertSize(file.fileSize)));}

    // Set icon depending on type
    if ((file.attribute & 0x10) != 0) // Directory bit
        item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    else
        item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));

    return item;
}

void MainWindow::loadDirectory(QTreeWidgetItem *parent, int cluster) {
    std::vector<File> list;
    getFiles(cluster, disk, list);

    for (auto &f : list) {
        QTreeWidgetItem *child = createTreeItem(f);
        if (parent)
            parent->addChild(child);
        else
            ui->fileTree->addTopLevelItem(child);

        // if it's a folder, add a dummy child so Qt shows expand arrow
        if (f.attribute & 0x10) {
            child->addChild(new QTreeWidgetItem(QStringList() << "..."));
        }
    }
}

void MainWindow::on_fileTree_itemExpanded(QTreeWidgetItem *item)
{
    // if first child is dummy "..."
    if (item->childCount() == 1 && item->child(0)->text(0) == "...") {
        item->takeChildren(); // remove dummy

        File *f = reinterpret_cast<File*>(
            item->data(0, Qt::UserRole).toULongLong()
            );
        if (f && (f->attribute & 0x10)) { // folder
            loadDirectory(item, f->firstCluster);
        }
    }
}


void MainWindow::on_fileTree_itemPressed(QTreeWidgetItem *item, int column)
{
    ui->txtDisplay->clear();
    File *f = reinterpret_cast<File*>(item->data(0, Qt::UserRole).toULongLong());
    if (!f) return;

    if (!(f->attribute & 0x10)) { // not a folder
        if (f->fileExtension == "TXT" || f->fileName.find("txt") != std::string::npos) {
            QString content;
            std::vector<uint32_t> clusters = getListClusters(f->firstCluster, disk);
            int size = f->fileSize;

            for (uint32_t i : clusters) {
                int sectorNum = firstSectorofCluster(disk.getFirstDataSector(), disk.getSecPerClus(), i);
                for (int j = sectorNum; j < sectorNum + disk.getSecPerClus(); j++) {
                    BYTE sector[512];
                    ReadSector(disk.drive, j, sector);
                    if (size > 512) {
                        content += QString::fromStdString(hexToString(sector, 0, 511));
                        size -= 512;
                    } else {
                        content += QString::fromStdString(hexToString(sector, 0, size));
                        break;
                    }
                }
            }

            ui->txtDisplay->setPlainText(content);
        }
    }
}


void MainWindow::on_diskInputButton_clicked()
{
    ui->resultDisplay1->clear();

    QString diskLetter = ui->diskInput->text();
    std::wstring drivePath = L"\\\\.\\" + diskLetter.toStdWString() + L":";

    if (disk.getInfo(drivePath.c_str()) != 0) {
        ui->resultDisplay1->setText("Can't read disk!");
        ui->fileTree->clear();
        return;
    }

    QString fsType = QString::fromStdString(disk.getFileSysType()).trimmed();
    if (fsType != "FAT32") {
        ui->resultDisplay1->setText("Unsupported filesystem: " + fsType);
        ui->fileTree->clear();
        return;
    }

    QString result;
    result += "<b>File System Type:</b> " + QString::fromStdString(disk.getFileSysType());
    result += "<hr>";
    result += "<table border='0' cellspacing='2' cellpadding='2'>";
    result += "<tr><td><b>Sector size (byte):</b></td><td>" + QString::fromStdString(convertSize(disk.getBytesPerSec())) + "</td></tr>";
    result += "<tr><td><b>Sectors per cluster:</b></td><td>" + QString::number(disk.getSecPerClus()) + "</td></tr>";
    result += "<tr><td><b>Boot sector size (sector):</b></td><td>" + QString::number(disk.getBootSecSize()) + "</td></tr>";
    result += "<tr><td><b>Number of FATs:</b></td><td>" + QString::number(disk.getNumFatTable()) + "</td></tr>";
    result += "<tr><td><b>Volume size (sector):</b></td><td>" + QString::number(disk.getTotalSector32()) + "</td></tr>";
    result += "<tr><td><b>FAT size (sector/FAT):</b></td><td>" + QString::number(disk.getFatTableSize()) + "</td></tr>";
    result += "<tr><td><b>RDET start cluster:</b></td><td>" + QString::number(disk.getFirstRootClus()) + "</td></tr>";
    result += "<tr><td><b>RDET start sector:</b></td><td>" + QString::number(disk.getFirstRDETSector()) + "</td></tr>";
    result += "<tr><td><b>Data area start sector:</b></td><td>" + QString::number(disk.getFirstDataSector()) + "</td></tr>";
    result += "</table>";

    ui->resultDisplay1->setHtml(result);
    ui->fileTree->clear();
    loadDirectory(nullptr, disk.getFirstRootClus());
}
