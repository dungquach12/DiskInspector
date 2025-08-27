# DiskInspector

A simple disk inspection application built with **Qt** and **CMake**.  
Supports basic disk information retrieval and a clean button-based UI.

## Feature:
- Read info of a FAT32 disk
---

## Requirements
- CMake (>= 3.16 recommended)
- Qt 6 (tested with Qt 6.9.1)
- A C++17-compatible compiler (e.g. GCC, Clang, MSVC, MinGW)

---

## Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/dungquach12/DiskInspector
   cd DiskInspector
   ```

2. Configure the project with CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

3. Build the project
   ```bash
   cmake --build build
   ```

## Run
### On Windows:

    ```bash
    .\build\DiskInspector.exe
    ```
