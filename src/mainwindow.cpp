#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DiskInspector/fat32.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_diskInputButton_clicked()
{
    FATbootSector disk;

    QString diskLetter = ui->diskInput->text();
    std::wstring drivePath = L"\\\\.\\" + diskLetter.toStdWString() + L":";

    FATbootSector boot;
    if (boot.getInfo(drivePath.c_str()) != 0) {
        ui->resultDisplay->setText("Can't read disk!");
        return;
    }

    QString result;
    result += "File System Type: " + QString::fromStdString(boot.getFileSysType()) + "\n";
    result += "-------------------------------------------\n";
    result += "Sector size (byte): " + QString::number(boot.getBytesPerSec()) + "\n";
    result += "Sectors per cluster: " + QString::number(boot.getSecPerClus()) + "\n";
    result += "Boot sector size (sector): " + QString::number(boot.getBootSecSize()) + "\n";
    result += "Number of FATs: " + QString::number(boot.getNumFatTable()) + "\n";
    result += "Volume size (sector): " + QString::number(boot.getTotalSector32()) + "\n";
    result += "FAT size (sector/FAT): " + QString::number(boot.getFatTableSize()) + "\n";
    result += "RDET start cluster: " + QString::number(boot.getFirstRootClus()) + "\n";
    result += "RDET start sector: " + QString::number(boot.getFirstRDETSector()) + "\n";
    result += "Data area start sector: " + QString::number(boot.getFirstDataSector()) + "\n";

    ui->resultDisplay->setText(result);
}
