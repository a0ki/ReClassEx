#pragma once
#include "stdafx.h"

#define LAST_STATUS_OFS (0x598 + 0x197 * sizeof(void*))
#define STATUS_SUCCESS			((NTSTATUS)0x00000000L)
#define STATUS_UNLOADED			((NTSTATUS)0x11100001L)
#define STATUS_NOT_FOUND		((NTSTATUS)0xC0000225L)
#define STATUS_UNSUCCESSFUL		((NTSTATUS)0xC0000001L)

#define DRIVER_SVC_NAME				 L"mstoresvc"
#define DRIVER_DEVICE_FILE           L"\\\\.\\" DRIVER_SVC_NAME

#define FILE_DEVICE_DRIVER 0x131120
#define IOCTL_CORE_BASE				(ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x810, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CORE_WRITE			(ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x811, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CORE_READ				(ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x812, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CORE_SETUP			(ULONG)CTL_CODE(FILE_DEVICE_DRIVER, 0x813, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

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

typedef struct _REQUEST_PROTECT {
	DWORD ProcessId;
	PVOID Address;
	DWORD Size;
	PDWORD InOutProtect;
} REQUEST_PROTECT, * PREQUEST_PROTECT;

typedef struct _REQUEST_ALLOC {
	DWORD ProcessId;
	PVOID OutAddress;
	DWORD Size;
	DWORD Protect;
} REQUEST_ALLOC, * PREQUEST_ALLOC;

typedef struct _REQUEST_FREE {
	DWORD ProcessId;
	PVOID Address;
} REQUEST_FREE, * PREQUEST_FREE;

typedef struct _REQUEST_ZERO {
	DWORD ProcessId;
	PVOID Address;
	DWORD Size;
} REQUEST_ZERO, * PREQUEST_ZERO;

typedef struct _REQUEST_MODULE {
	DWORD ProcessId;
	WCHAR Module[0xFF];
	PBYTE* OutAddress;
	PDWORD OutSize;
} REQUEST_MODULE, * PREQUEST_MODULE;

typedef struct _REQUEST_ALIVE {
	PDWORD Param1;
} REQUEST_ALIVE, * PREQUEST_ALIVE;

typedef struct _REQUEST_ENABLE {
	DWORD Param1;
} REQUEST_ENABLE, * PREQUEST_ENABLE;

typedef struct _REQUEST_SHUTDOWN {
	PVOID Param1;
} REQUEST_SHUTDOWN, * PREQUEST_SHUTDOWN;

typedef struct _THREAD_CALLBACK_INFO
{
	ULONG  cPid;
	PVOID  Address;
	ULONG  hThreadId;
	ULONG  bCreate;
} THREAD_CALLBACK_INFO, * PTHREAD_CALLBACK_INFO;

typedef struct _REQUEST_SETUP
{
	ULONG ProcessId;
	PVOID FunctionAddress;
} REQUEST_SETUP, * PREQUEST_SETUP;

namespace service
{
	class cDriver
	{
	public:
		std::wstring    DriverPath;
		static NTSTATUS _loadStatus;
		static HANDLE   _hDriver;
		static HANDLE _NotifyEvent;
		static uint32_t OSArch;
		static uint32_t ProcessId;

		cDriver();
		~cDriver();

		static cDriver& Instance();

		void Install(std::wstring Path);

		/// <summary>
		/// Fill minimum driver registry entry
		/// </summary>
		/// <param name="svcName">Driver service name</param>
		/// <param name="path">Driver path</param>
		/// <returns>Status code</returns>
		LSTATUS PrepareDriverRegEntry(const std::wstring& svcName, const std::wstring& path);

		NTSTATUS LastNtStatus();

		/// <summary>
		/// Grant Privilege to the current process
		/// </summary>
		/// <param name="name">Privilege Name</param>
		/// <returns>Status code</returns>
		NTSTATUS GrantPriviledge(const std::wstring& name);

		/// <summary>
		/// Load arbitrary driver
		/// </summary>
		/// <param name="svcName">Driver service name</param>
		/// <param name="path">Driver file path</param>
		/// <returns>Status</returns>
		NTSTATUS LoadDriver(const std::wstring& svcName, const std::wstring& path);

		/// <summary>
		/// Try to load driver if it isn't loaded
		/// </summary>
		/// <param name="path">Path to the driver file</param>
		/// <returns>Status code</returns>
		NTSTATUS EnsureLoaded(std::wstring path = L"");

		/// <summary>
		/// Unload driver
		/// </summary>
		/// <returns>Status code</returns>
		NTSTATUS Unload();

		std::wstring GetCurrentDir();
		std::string GetCurrentDirA();

		/// <summary>
		/// Gen Temp Path to Extract
		/// </summary>
		/// <param name="ret">Path to the driver file</param>
		/// <param name="filename">File name</param>
		void GenPathToExtract(wchar_t* ret, const wchar_t* filename);

		/// <summary>
		/// Gen Temp Path to Extract
		/// </summary>
		/// <param name="ret">Path to the driver file</param>
		/// <param name="filename">File name</param>
		void GenPathToExtract(char* ret, char* filename);

		/// <summary>
		/// Get System Architeture
		/// </summary>
		int GetSystemArch();

		/// <summary>
		/// Unload and Exit
		/// </summary>
		/// <returns>Status code</returns>
		NTSTATUS UnloadAndExit();

		/// <summary>
		/// Unload arbitrary driver
		/// </summary>
		/// <param name="svcName">Driver service name</param>
		/// <returns>Status</returns>
		NTSTATUS UnloadDriver(const std::wstring& svcName);

		void Setup(uint32_t pId);

		NTSTATUS Read(PVOID dest, PVOID src, DWORD size);

		NTSTATUS Write(PVOID dest, PVOID src, DWORD size, uint32_t pId = 0);

		PVOID Base();

		ULONG checkForThreadCreation(OUT PVOID* address_read);
	};

	// Syntax sugar
	inline cDriver& Driver() { return cDriver::Instance(); }
}

class Mem
{
public:
	template <typename T>
	static T read(const PVOID address)
	{
		T buffer{ };
		service::Driver().Read(&buffer, (PVOID)address, sizeof(T));

		return buffer;
	}

	static bool readwtf(PVOID Address, void* Buffer, SIZE_T Size)
	{
		if (NT_SUCCESS(service::Driver().Read(Buffer, Address, Size)))
			return true;
		return false;
	}

	template<typename T>
	static bool write(PVOID address, T buffer)
	{
		return service::Driver().Write(address, &buffer, sizeof(T));
	}
};