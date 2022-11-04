#include "service.h"
#include "FunctionTypes.h"
#include "dynimport.h"
#include "utils.h"

namespace service

{
	NTSTATUS cDriver::_loadStatus = STATUS_NOT_FOUND;
	HANDLE cDriver::_hDriver = INVALID_HANDLE_VALUE;
	HANDLE cDriver::_NotifyEvent = INVALID_HANDLE_VALUE;
	uint32_t cDriver::OSArch = 0;
	uint32_t cDriver::ProcessId = 0;

	cDriver::cDriver()
	{
		HMODULE ntdll = GetModuleHandleA("ntdll.dll");

		DynImport::load("NtLoadDriver", ntdll);
		DynImport::load("NtUnloadDriver", ntdll);
		DynImport::load("RtlDosPathNameToNtPathName_U", ntdll);
		DynImport::load("RtlInitUnicodeString", ntdll);
		DynImport::load("RtlFreeUnicodeString", ntdll);
	}

	cDriver::~cDriver()
	{
		//Unload();
	}

	cDriver& cDriver::Instance()
	{
		static cDriver instance;
		return instance;
	}

	void cDriver::Install(std::wstring Path)
	{
		UnloadDriver(DRIVER_SVC_NAME);
		OSArch = GetSystemArch();

		if (Path.empty())
			DriverPath = GetCurrentDir() + L"driver.sys";
		else
			DriverPath = Path;

		_loadStatus = EnsureLoaded();
		if (_loadStatus != STATUS_SUCCESS)
		{
			Sleep(2000);
			UnloadAndExit();
		}
	}

	std::wstring cDriver::GetCurrentDir()
	{
		auto path = utils::get_currentpath();

		return std::wstring(path.begin(), path.end());
	}

	std::string cDriver::GetCurrentDirA()
	{
		char buffer[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, buffer);
		return std::string(buffer);
	}

	void cDriver::GenPathToExtract(wchar_t* ret, const wchar_t* filename)
	{
		GetTempPathW(MAX_PATH, ret);
		wcscat(ret, filename);
	}

	void cDriver::GenPathToExtract(char* ret, char* filename)
	{
		GetTempPathA(MAX_PATH, ret);
		strcat(ret, filename);
	}

	int cDriver::GetSystemArch()
	{
		SYSTEM_INFO si; //Pointer to the System Structure
		GetNativeSystemInfo(&si); //Get a pointer to the system structure

		if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		{
			return 64;
		}
		else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		{
			return 32;
		}

		return 0; //Failed to Get OS Architecture
	}

	NTSTATUS cDriver::LastNtStatus()
	{
		return *(NTSTATUS*)((unsigned char*)NtCurrentTeb() + LAST_STATUS_OFS);
	}

	LSTATUS cDriver::PrepareDriverRegEntry(const std::wstring& svcName, const std::wstring& path)
	{
		HKEY key1, key2;
		DWORD dwType = 1;
		LSTATUS status = 0;
		WCHAR wszLocalPath[MAX_PATH] = { 0 };
		WCHAR dspName[MAX_PATH] = { 0 };

		swprintf_s(wszLocalPath, ARRAYSIZE(wszLocalPath), L"\\??\\%s", path.c_str());
		swprintf_s(dspName, ARRAYSIZE(dspName), L"%s", svcName.c_str());

		status = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"system\\CurrentControlSet\\Services", &key1);
		if (status)
			return status;

		status = RegCreateKeyW(key1, svcName.c_str(), &key2);
		if (status)
		{
			RegCloseKey(key1);
			return status;
		}

		status = RegSetValueExW(
			key2, L"ImagePath", 0, REG_SZ,
			reinterpret_cast<const BYTE*>(wszLocalPath),
			static_cast<DWORD>(sizeof(WCHAR) * (wcslen(wszLocalPath) + 1))
		);

		if (status)
		{
			RegCloseKey(key2);
			RegCloseKey(key1);
			return status;
		}

		status = RegSetValueExW(key2, L"Type", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwType), sizeof(dwType));
		if (status)
		{
			RegCloseKey(key2);
			RegCloseKey(key1);
			return status;
		}

		status = RegSetValueExW(key2, L"DisplayName", 0, REG_SZ, reinterpret_cast<const BYTE*>(dspName), static_cast<DWORD>(sizeof(WCHAR) * (wcslen(dspName) + 1)));
		if (status)
		{
			RegCloseKey(key2);
			RegCloseKey(key1);
			return status;
		}

		RegCloseKey(key2);
		RegCloseKey(key1);

		return status;
	}

	NTSTATUS cDriver::EnsureLoaded(std::wstring path)
	{
		if (path.empty())
			path = DriverPath;

		UnloadDriver(DRIVER_SVC_NAME);

		_loadStatus = LoadDriver(DRIVER_SVC_NAME, path);

		if (!NT_SUCCESS(_loadStatus))
		{
			printf("Load kernel module error 0x%X\n", _loadStatus);
			return LastNtStatus();
		}

		_hDriver = CreateFileW(
			DRIVER_DEVICE_FILE,
			GENERIC_ALL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL
		);

		if (_hDriver == INVALID_HANDLE_VALUE || !_hDriver)
		{
			printf("_hDriver Returned an error %d - %ls\n", GetLastError(), DriverPath.c_str());
		}

		return LastNtStatus();
	}

	NTSTATUS cDriver::LoadDriver(const std::wstring& svcName, const std::wstring& path)
	{
		NTSTATUS status;
		UNICODE_STRING Ustr;

		// If no file provided, try to start existing service
		if (!path.empty() && PrepareDriverRegEntry(svcName, path) != 0)
			return GetLastError();

		std::wstring regPath = L"\\registry\\machine\\SYSTEM\\CurrentControlSet\\Services\\" + svcName;
		SAFE_CALL(RtlInitUnicodeString, &Ustr, regPath.c_str());

		return SAFE_NATIVE_CALL(NtLoadDriver, &Ustr);
	}

	NTSTATUS cDriver::UnloadDriver(const std::wstring& svcName)
	{
		UNICODE_STRING Ustr = { 0 };

		std::wstring regPath = L"\\registry\\machine\\SYSTEM\\CurrentControlSet\\Services\\" + svcName;
		SAFE_CALL(RtlInitUnicodeString, &Ustr, regPath.c_str());

		// Remove previously loaded instance, if any
		_loadStatus = SAFE_NATIVE_CALL(NtUnloadDriver, &Ustr);
		SHDeleteKeyW(HKEY_LOCAL_MACHINE, (L"SYSTEM\\CurrentControlSet\\Services\\" + svcName).c_str());

		return _loadStatus;
	}

	NTSTATUS cDriver::UnloadAndExit()
	{
		Unload();
		ExitProcess(0);
		TerminateProcess(GetCurrentProcess(), 0);
		return 0;
	}

	NTSTATUS cDriver::Unload()
	{
		if (_hDriver != INVALID_HANDLE_VALUE)
		{
			CloseHandle(_hDriver);
			_hDriver = INVALID_HANDLE_VALUE;
		}
		return UnloadDriver(DRIVER_SVC_NAME);
	}

	NTSTATUS cDriver::GrantPriviledge(const std::wstring& name)
	{
		TOKEN_PRIVILEGES Priv, PrivOld;
		DWORD cbPriv = sizeof(PrivOld);
		HANDLE hToken;

		if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, FALSE, &hToken))
		{
			if (GetLastError() != ERROR_NO_TOKEN)
				return LastNtStatus();

			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
				return LastNtStatus();
		}

		Priv.PrivilegeCount = 1;
		Priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		LookupPrivilegeValueW(NULL, name.c_str(), &Priv.Privileges[0].Luid);

		if (!AdjustTokenPrivileges(hToken, FALSE, &Priv, sizeof(Priv), &PrivOld, &cbPriv))
		{
			CloseHandle(hToken);
			return LastNtStatus();
		}

		if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
		{
			CloseHandle(hToken);
			return LastNtStatus();
		}

		CloseHandle(hToken);
		return STATUS_SUCCESS;
	}

	void cDriver::Setup(uint32_t pId)
	{
		ProcessId = pId;

		/*PVOID addresstowrite;
		service::Driver().checkForThreadCreation(&addresstowrite);

		REQUEST_SETUP req = { 0 };
		req.FunctionAddress = addresstowrite;
		req.ProcessId = GetCurrentProcessId();

		DWORD bytes;
		DeviceIoControl(_hDriver, IOCTL_CORE_SETUP, &req, sizeof(req), nullptr, 0, &bytes, NULL);*/
	}

	NTSTATUS cDriver::Read(PVOID dest, PVOID src, DWORD size) {
		REQUEST_READ req = { 0 };
		req.ProcessId = ProcessId;
		req.Dest = dest;
		req.Src = src;
		req.Size = size;

		DWORD bytes;
		if (!DeviceIoControl(_hDriver, IOCTL_CORE_READ, &req, sizeof(req), nullptr, 0, &bytes, NULL))
			return GetLastError();

		return STATUS_SUCCESS;
	}

	NTSTATUS cDriver::Write(PVOID dest, PVOID src, DWORD size, uint32_t pId) {
		REQUEST_WRITE req = { 0 };
		if(!pId)
			req.ProcessId = this->ProcessId;
		else
			req.ProcessId = pId;
		req.Dest = dest;
		req.Src = src;
		req.Size = size;

		DWORD bytes;
		if (!DeviceIoControl(_hDriver, IOCTL_CORE_WRITE, &req, sizeof(req), nullptr, 0, &bytes, NULL))
			return GetLastError();

		return STATUS_SUCCESS;
	}

	PVOID cDriver::Base() {
		REQUEST_BASE req = { 0, 0 };
		req.ProcessId = this->ProcessId;
		req.Base = 0;

		DWORD bytes;
		if (DeviceIoControl(_hDriver, IOCTL_CORE_BASE, &req, sizeof(req), &req, sizeof(req), &bytes, NULL))
			return req.Base;

		return (PVOID)404;
	}

	ULONG cDriver::checkForThreadCreation(OUT PVOID *address_read)
	{
		static PVOID address = 0;
		if (!address)
		{
			address = VirtualAlloc(0, sizeof(ULONG), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			*(ULONG*)address = 0;
		}

		if(address_read != 0)
			*address_read = address;

		return *(ULONG*)address;
	}
}