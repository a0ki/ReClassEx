#include "utils.h"

namespace utils {

	string TextFormat(const char* format, ...)
	{
		auto temp = std::vector<char>{};
		auto length = std::size_t{ 63 };
		std::va_list args;
		while (temp.size() <= length)
		{
			temp.resize(length + 1);
			va_start(args, format);
			const auto status = vsnprintf(temp.data(), temp.size(), format, args);
			va_end(args);
			if (status < 0)
				return std::string("");
			length = (std::size_t)(status);
		}
		return std::string{ temp.data(), length };
	}

	wstring TextFormatW(const char* format, ...)
	{
		auto temp = std::vector<char>{};
		auto length = std::size_t{ 63 };
		std::va_list args;
		while (temp.size() <= length)
		{
			temp.resize(length + 1);
			va_start(args, format);
			const auto status = vsnprintf(temp.data(), temp.size(), format, args);
			va_end(args);
			if (status < 0)
				return std::wstring(L"");
			length = (std::size_t)(status);
		}
		string str = std::string{ temp.data(), length };
		return wstring{ str.begin(), str.end() };
	}

	std::string tm_to_readable_time(tm ctx) {
		char buffer[80];

		strftime(buffer, sizeof(buffer), "%a %d/%m/%y %H:%M:%S", &ctx);

		return std::string(buffer);
	}

	static std::time_t string_to_timet(std::string timestamp) {
		auto cv = strtol(timestamp.c_str(), NULL, 10); // long

		return (time_t)cv;
	}

	static std::tm timet_to_tm(time_t timestamp) {
		std::tm context;

		localtime_s(&context, &timestamp);

		return context;
	}

	DWORD FindThreadID(HWND Window)
	{
		THREADENTRY32 te32 = { 0 };
		DWORD ret = 0;
		HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		te32.dwSize = sizeof(THREADENTRY32);

		DWORD dwPID;
		GetWindowThreadProcessId(Window, &dwPID);

		do
		{
			if (!Thread32First(hThreadSnap, &te32))
				break;

			do
			{
				if (te32.th32OwnerProcessID == dwPID)
				{
					ret = te32.th32ThreadID;
					break;
				}
			} while (Thread32Next(hThreadSnap, &te32));
		} while (0);

		CloseHandle(hThreadSnap);

		return ret;
	}

	std::string random_string(size_t length)
	{
		std::string tmp_s;
		static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

		srand((unsigned)time(NULL) * _getpid());

		tmp_s.reserve(length);

		for (int i = 0; i < length; ++i)
			tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

		return tmp_s;
	}

	std::string get_currentpath()
	{
		char imgPath[MAX_PATH];
		DWORD value = MAX_PATH;
		QueryFullProcessImageNameA(GetCurrentProcess(), 0, imgPath, &value);

		std::string path = imgPath;
		path = path.substr(0, path.find_last_of('\\') + 1);

		return std::string(path);
	}

	uint32_t getpIdByname(std::wstring a1)
	{
		uint32_t ret = 0;
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(entry);
		if (!Process32First(snap, &entry))
		{
			return ret;
		}
		do {
			if (entry.szExeFile == a1)
			{
				ret = entry.th32ProcessID;
				break;
			}
		} while (Process32Next(snap, &entry));
		CloseHandle(snap);
		return ret;
	}

	int random_int(int max, int min) {
		srand((unsigned)time(NULL) * _getpid());

		return rand() % (max - min) + min;
	}

	float random_float(float max, float min) {
		srand((unsigned)time(NULL) * _getpid());

		return (min + 1) + (((float)rand()) / (float)RAND_MAX) * (max - (min + 1));
	}
}