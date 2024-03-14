#pragma once
#include <string>

#define FILE_DEVICE_DRIVER 0x131120
#define IOCTL_CORE_BASE                (ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x810, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CORE_WRITE               (ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x811, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CORE_READ                (ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x812, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CORE_SETUP               (ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x813, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CORE_MODULE              (ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x814, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define DRIVER_SVC_NAME L"mstoresvc"
#define DRIVER_DEVICE_FILE L"\\\\.\\" DRIVER_SVC_NAME

typedef struct _REQUEST_BASE {
    DWORD ProcessId;
    PVOID Base;
} REQUEST_BASE, * PREQUEST_BASE;
typedef struct _REQUEST_WRITE {
    DWORD ProcessId;
    PVOID Dest;
    PVOID Src;
    DWORD Size;
} REQUEST_WRITE, * PREQUEST_WRITE;
typedef struct _REQUEST_READ {
    DWORD ProcessId;
    PVOID Dest;
    PVOID Src;
    DWORD Size;
} REQUEST_READ, * PREQUEST_READ;
typedef struct _REQUEST_MODULE {
    DWORD ProcessId;
    WCHAR Module[0xFF];
    PBYTE* OutAddress;
    PDWORD OutSize;
} REQUEST_MODULE, * PREQUEST_MODULE;

namespace driver_io {
    extern DWORD ProcessId;

    void setup();
    bool attach_game(std::string window_class);
    PVOID base_address();
    bool read_memory(void* address, void* buffer, uint64_t size);
    DWORD Module(const std::wstring& module_name, PBYTE* OutBaseAddress, PDWORD OutSize);

    template<typename T>
    T read(auto dwAddress)
    {
        T value{};
        if (!read_memory(reinterpret_cast<void*>(dwAddress), &value, sizeof(T)))
        {
            return T();
        }
        return value;
    }
}