#include "DiskInspector/fat32.h"

// Constants for FAT directory entries
constexpr uint8_t ATTR_LONG_NAME = 0x0F;
constexpr uint8_t ENTRY_DELETED = 0xE5;
constexpr uint8_t ENTRY_EMPTY = 0x00;
constexpr uint8_t ENTRY_DOT = 0x2E;
constexpr int ENTRY_SIZE = 32;
constexpr int SHORT_NAME_LEN = 8;
constexpr int SHORT_EXT_LEN = 3;

// Offsets within directory entries
constexpr int OFFSET_ATTRIBUTE = 0x0B;
constexpr int OFFSET_FIRST_CLUSTER = 0x1A;
constexpr int OFFSET_FILE_SIZE = 0x1C;
constexpr int OFFSET_SHORT_EXT = 0x08;

// Long filename offsets
constexpr int LFN_NAME1_OFFSET = 0x01;
constexpr int LFN_NAME1_LEN = 10;
constexpr int LFN_NAME2_OFFSET = 0x0E;
constexpr int LFN_NAME2_LEN = 12;
constexpr int LFN_NAME3_OFFSET = 0x1C;
constexpr int LFN_NAME3_LEN = 4;

enum class ParseResult {
    SUCCESS,
    ERROR_READ_SECTOR,
    ERROR_INVALID_CLUSTER,
    ERROR_BOUNDS_CHECK
};

// START BOOTSECTOR SECTION

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
    std::string trimmed;
    for (char c : str) {
        // Keep only printable ASCII characters (space and above, but not DEL or higher)
        if (c >= 32 && c <= 126) {
            trimmed += c;
        }
    }
    // Remove leading/trailing spaces
    size_t start = trimmed.find_first_not_of(' ');
    size_t end = trimmed.find_last_not_of(' ');
    if (start == std::string::npos) return "";
    return trimmed.substr(start, end - start + 1);
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


// END BOOTSECTOR SECTION


// START THE FILES SECTION


File::File() {
    this->fileName = "";
    this->fileExtension = "";
    this->attribute = 0;
    this->firstCluster = 0;
    this->fileSize = 0;
}


std::string parseLongName(BYTE sector[], int offset, int& subEntryCount) {
    std::string longName;
    subEntryCount = 1;
    uint8_t attribute = sector[offset + OFFSET_ATTRIBUTE];
    while (attribute == sector[offset + OFFSET_ATTRIBUTE + 32 * subEntryCount]) {
        if (sector[offset + OFFSET_ATTRIBUTE + 32 * subEntryCount] != ATTR_LONG_NAME) break;
        subEntryCount++;
        if ((offset + OFFSET_SHORT_EXT + 32 * subEntryCount) >= 512) {
            break;
        }
    }
    for (int i = subEntryCount - 1; i >= 0; i--) {
        longName += hexToString(sector, offset + LFN_NAME1_OFFSET + 32 * i, LFN_NAME1_LEN)
                 + hexToString(sector, offset + LFN_NAME2_OFFSET + 32 * i, LFN_NAME2_LEN)
                 + hexToString(sector, offset + LFN_NAME3_OFFSET + 32 * i, LFN_NAME3_LEN);
    }
    return clearExcessSpace(longName);
}


int getFiles(int firstCluster, FATbootSector disk, std::vector<File>& list) {
    std::vector<uint32_t> listcluster = getListClusters(firstCluster, disk);
    std::vector<File> fileList;

    for (uint32_t i : listcluster) {
        int sectorNum = firstSectorofCluster(disk.getFirstDataSector(), disk.getSecPerClus(), i);
        for (int j = sectorNum; j < sectorNum + disk.getSecPerClus(); ++j) {
            BYTE sector[disk.getBytesPerSec()];
            ReadSector(disk.drive, j, sector);

            for (int k = 0; k < disk.getBytesPerSec(); k += 32) {
                if (j == sectorNum && k == 0) k += 64;
                uint8_t entryStatus = sector[k];
                if (entryStatus == ENTRY_DOT || entryStatus == ENTRY_EMPTY || entryStatus == ENTRY_DELETED) continue;

                uint8_t attribute = sector[k + OFFSET_ATTRIBUTE];
                File tmp;

                if (attribute == ATTR_LONG_NAME) {
                    int subEntryCount = 0;
                    std::string longName = parseLongName(sector, k, subEntryCount);
                    // Move k to the short entry
                    k += 32 * subEntryCount;
                    // Now extract info from the short entry
                    tmp.fileName = clearExcessSpace(longName);
                    tmp.attribute = sector[k + OFFSET_ATTRIBUTE];
                    memcpy(&tmp.firstCluster, sector + k + OFFSET_FIRST_CLUSTER, 2);
                    memcpy(&tmp.fileSize, sector + k + OFFSET_FILE_SIZE, 4);

                } else {
                    tmp.fileName = clearExcessSpace(hexToString(sector, k, 8));
                    tmp.fileExtension = clearExcessSpace(hexToString(sector, k + OFFSET_SHORT_EXT, 3));
                    tmp.attribute = attribute;
                    memcpy(&tmp.firstCluster, sector + k + OFFSET_FIRST_CLUSTER, 2);
                    memcpy(&tmp.fileSize, sector + k + OFFSET_FILE_SIZE, 4);
                }
                fileList.push_back(tmp);
            }
        }
    }
    list = fileList;
    return 0;
}
