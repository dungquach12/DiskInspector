#include "DiskInspector/fat32.h"

enum class FS_Type  { Unknown, NTFS, FAT32 };

int ReadSector(const std::wstring& drive, int readPoint, BYTE sector[])
{
    int retCode = 0;
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFile(drive.c_str(),    // Drive to open
        GENERIC_READ,           // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,        // Share Mode
        NULL,                   // Security Descriptor
        OPEN_EXISTING,          // How to create
        0,                      // File attributes
        NULL);                  // Handle to template

    if (device == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }

    SetFilePointer(device, readPoint * 512, NULL, FILE_BEGIN);

    if (!ReadFile(device, sector, 512, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
    }

    return 0;
}


std::string hexToString(BYTE arr[], int startLoc, int size) {
    return std::string(reinterpret_cast<char*>(arr + startLoc), size);
}


std::string clearExcessSpace(const std::string& str) {
    std::string trimmed = str;

    // Remove null characters
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\0'), trimmed.end());

    // Remove multiple spaces
    auto new_end = std::unique(trimmed.begin(), trimmed.end(),
        [](char a, char b) { return a == ' ' && b == ' '; });
    trimmed.erase(new_end, trimmed.end());

    // Trim leading/trailing spaces
    if (!trimmed.empty() && trimmed.front() == ' ') trimmed.erase(trimmed.begin());
    if (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();

    return trimmed;
}


int firstSectorofCluster(int FirstDataSector, int SecPerClus, int clusOrd) { return FirstDataSector + (clusOrd - 2) * SecPerClus; }


FS_Type detectFileSystem(BYTE bootSector[]) {
    std::string name = clearExcessSpace(hexToString(bootSector, 0x03, 8));
    uint16_t FAT32sign;
    memcpy(&FAT32sign, bootSector + 510, 2);

    if (name.find("NTFS") != std::string::npos) return FS_Type::NTFS;
    if (FAT32sign == 0xAA55) return FS_Type::FAT32;
    return FS_Type::Unknown;
}


std::vector<uint32_t> getListClusters(int firstCluster, FATbootSector disk)
{
    std::vector<uint32_t> listClusters;
    BYTE sectorFat[512];
    uint32_t nextCluster = firstCluster;

    do {
        int thisFATSecNum = disk.getBootSecSize() + nextCluster / ((disk.getBytesPerSec() / 4) - 1);

        int thisFATEntOffset = (nextCluster * 4) % 512;
        listClusters.push_back(nextCluster);
        ReadSector(disk.drive, thisFATSecNum, sectorFat);

        memcpy(&nextCluster, sectorFat + thisFATEntOffset, 4);
    } while (nextCluster != 0x0FFFFFFF && nextCluster != 0x0FFFFFF8);

    return listClusters;
}


FATbootSector::FATbootSector() {
    this->BytesPerSec = 0;
    this->SecPerClus = 0;
    this->BootSecSize = 0;
    this->NumFatTable = 0;
    this->TotalSector32 = 0;
    this->FatTableSize32 = 0;
    this->FirstRootCluster = 0;
    this->FirstRDETSector = 0;
    this->FirstDataSector = 0;

    this->FileSysType = ' ';
}


void FATbootSector::getInfo(BYTE arr[]) {
    memcpy(&this->BytesPerSec, arr + 0xB, 2);
    this->SecPerClus = arr[0xD];
    memcpy(&this->BootSecSize, arr + 0xE, 2);
    this->NumFatTable = arr[0x10];
    memcpy(&this->TotalSector32, arr + 0x20, 4);

    // Get File System Type
    this->FileSysType = hexToString(arr, 0x52, 8);

    memcpy(&this->FatTableSize32, arr + 0x24, 4);
    memcpy(&this->FirstRootCluster, arr + 0x2C, 4);
    this->FirstDataSector = this->BootSecSize + this->NumFatTable * this->FatTableSize32; // Get first sector of Data region
    this->FirstRDETSector = firstSectorofCluster(this->FirstDataSector, this->SecPerClus, this->FirstRootCluster);
}


std::string FATbootSector::getFileSysType() { return this->FileSysType; }


uint16_t FATbootSector::getBytesPerSec() { return this->BytesPerSec; }

uint8_t FATbootSector::getSecPerClus() { return this->SecPerClus; }


uint16_t FATbootSector::getBootSecSize() { return this->BootSecSize; }


uint8_t FATbootSector::getNumFatTable() { return this->NumFatTable; }


uint32_t FATbootSector::getTotalSector32() { return this->TotalSector32; }


uint32_t FATbootSector::getFatTableSize() { return this->FatTableSize32; }


uint32_t FATbootSector::getFirstRootClus() { return this->FirstRootCluster; }


uint32_t FATbootSector::getFirstRDETSector() { return this->FirstRDETSector; }


uint32_t FATbootSector::getFirstDataSector() { return this->FirstDataSector; }


int FATbootSector::getInfo(const std::wstring& diskLoc) {
    BYTE sector[512];
    if (!ReadSector(diskLoc, 0, sector)) {
        this->getInfo(sector);
        this->drive = diskLoc;
        return 0;
    }
    return 1;
}