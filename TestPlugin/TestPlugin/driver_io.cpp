#include "stdafx.h"
#include "driver_io.h"
#include "driver_data.h"

namespace driver_io {

    HANDLE _hDriver;
    DWORD ProcessId;

    std::string get_system_dir_letter() {
        char systemDir[MAX_PATH];
        DWORD result = GetSystemDirectoryA(systemDir, MAX_PATH);
        return std::string(1, systemDir[0]);
    }

    bool create_file_from_memory(const std::string& file_name, const unsigned char* file_data, size_t size) {
        std::ofstream file(file_name, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open file for writing." << std::endl;
            return false;
        }

        file.write(reinterpret_cast<const char*>(file_data), size);
        file.close();
        return true;
    }

    HANDLE open_handle() {
        return CreateFileW(
            DRIVER_DEVICE_FILE,             // Path to the driver
            GENERIC_READ | GENERIC_WRITE,   // Desired access
            0,                              // Share mode
            NULL,                           // Security attributes
            OPEN_EXISTING,                  // Creation disposition
            FILE_ATTRIBUTE_NORMAL,          // Flags and attributes
            NULL                            // Template file
        );
    }

    void setup() {
        _hDriver = open_handle();
        if (_hDriver == INVALID_HANDLE_VALUE) {
            auto driver_path = get_system_dir_letter() + ":\\Windows\\System32\\drivers\\Qsec.sys";

            system("sc stop mstoresvc");
            system("sc delete mstoresvc");

            std::string create_drv_cmd = "sc create mstoresvc type=kernel binPath=\"" + driver_path + "\"";
            system(create_drv_cmd.c_str());

            create_file_from_memory(driver_path, driver_data, sizeof(driver_data));

            system("sc start mstoresvc");
            system("cls");

            _hDriver = CreateFileW(
                DRIVER_DEVICE_FILE,             // Path to the driver
                GENERIC_READ | GENERIC_WRITE,   // Desired access
                0,                              // Share mode
                NULL,                           // Security attributes
                OPEN_EXISTING,                  // Creation disposition
                FILE_ATTRIBUTE_NORMAL,          // Flags and attributes
                NULL                            // Template file
            );

            if (_hDriver == INVALID_HANDLE_VALUE)
            {
                MessageBoxA(0, "[-] Failed to load kernel driver", "Error", MB_ICONERROR);
                exit(0);
            }
        }
    }

    bool attach_game(std::string window_class) {
        // Wait untill find window for 60 seconds
        auto start = std::chrono::high_resolution_clock::now();
        while (!FindWindowA(window_class.c_str(), nullptr)) {
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            if (elapsed.count() > 60) {
                return false;
            }
        }
        GetWindowThreadProcessId(FindWindowA(window_class.c_str(), nullptr), &ProcessId);
        return (ProcessId != 0);
    }

    bool read_memory(void* address, void* buffer, uint64_t size) {
        if (buffer == nullptr || size == 0) {
            return false;
        }

        REQUEST_READ req = { 0 };
        req.ProcessId = ProcessId;
        req.Dest = buffer;
        req.Src = address;
        req.Size = size;

        DWORD bytes;
        if (!DeviceIoControl(_hDriver, IOCTL_CORE_READ, &req, sizeof(req), nullptr, 0, &bytes, NULL))
        {
            //CloseHandle(hDriver);
            return false;
        }
        return true;
    }

    PVOID base_address()
    {
        REQUEST_BASE req = { 0, 0 };
        req.ProcessId = ProcessId;
        req.Base = 0;

        DWORD bytes;
        if (DeviceIoControl(_hDriver, IOCTL_CORE_BASE, &req, sizeof(req), &req, sizeof(req), &bytes, NULL))
            return req.Base;

        return (PVOID)404;
    }

    DWORD Module(const std::wstring& module_name, PBYTE* OutBaseAddress, PDWORD OutSize)
    {
        // Check for valid pointers
        if (OutBaseAddress == nullptr || OutSize == nullptr) {
            return STATUS_INVALID_PARAMETER;
        }

        REQUEST_MODULE req = { 0 };
        req.ProcessId = ProcessId;
        wsprintfW(req.Module, L"%ls", module_name.c_str());
        // Initialize OutAddress and OutSize in the request
        req.OutAddress = OutBaseAddress;
        req.OutSize = OutSize;

        DWORD bytes;
        // Call DeviceIoControl and check for success
        if (!DeviceIoControl(_hDriver, IOCTL_CORE_MODULE, &req, sizeof(req), nullptr, 0, &bytes, NULL)) {
            return GetLastError();
        }

        return 0;
    }

}