#ifndef FAT32_H
#define FAT32_H

#include <QMainWindow>

#include<iostream>
#include<windows.h>
#include<stdio.h>
#include<string>
#include<vector>

int ReadSector(const std::wstring& drive, int readPoint, BYTE sector[]);


// convert from hex to char
std::string hexToString(BYTE arr[], int startLoc, int size);


std::string clearExcessSpace(const std::string& str);


// find first sector of kth cluster
int firstSectorofCluster(int FirstDataSector, int SecPerClus, int clusOrd);


int isNTFSorFAT32(BYTE bootSector[]);


class FATbootSector {
private:
    uint16_t BytesPerSec;
    uint8_t SecPerClus;

    uint16_t BootSecSize;
    uint8_t NumFatTable;
    uint32_t TotalSector32;
    uint32_t FatTableSize32;
    uint32_t FirstRootCluster; // the first cluster of RDET(FAT32)

    uint32_t FirstRDETSector; // first sector of RDET
    uint32_t FirstDataSector; // first sector of Data

    std::string FileSysType;
public:
    std::wstring drive;
    FATbootSector();

    void getInfo(BYTE arr[]);

    std::string getFileSysType();
    uint16_t getBytesPerSec();
    uint8_t getSecPerClus();
    uint16_t getBootSecSize();
    uint8_t getNumFatTable();
    uint32_t getTotalSector32();
    uint32_t getFatTableSize();
    uint32_t getFirstRootClus();
    uint32_t getFirstRDETSector();
    uint32_t getFirstDataSector();


    int getInfo(const std::wstring& diskLoc);
};


// To traverse the FAT table and get the list of cluster of an entry
std::vector<uint32_t> getListClusters(int firstCluster, FATbootSector disk);


class File {
public:
    std::string fileName;
    std::string fileExtension;
    uint8_t attribute;
    uint16_t firstCluster;
    uint32_t fileSize;

    File();
};


int getFiles(int firstCluster, FATbootSector disk, std::vector<File>& list);


std::string convertAttrNumToAttrString(uint8_t attrNum);


int interactFile(File theFile, FATbootSector disk);


int Directory(FATbootSector disk, int cluster);

#endif // FAT32_H
