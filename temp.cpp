QTreeWidgetItem* MainWindow::createTreeItem(const File& file) {
    QTreeWidgetItem *item = new QTreeWidgetItem();
    QString name = QString::fromStdString(file.fileName);
    if (!file.fileExtension.empty()) {
        name += "." + QString::fromStdString(file.fileExtension);
    }
    item->setText(0, name);

    // Store File struct inside item (using QVariant → void* pointer trick)
    item->setData(0, Qt::UserRole, QVariant::fromValue((qulonglong)new File(file)));

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