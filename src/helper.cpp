#include "DiskInspector/helper.h"


std::string convertSize(uint32_t bytes) {
    const std::string unit[]= {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    while (bytes >= 1024 && i < 4) {
        bytes /= 1024;
        i++;
    }
    return std::to_string(bytes) + " " + unit[i];
}
