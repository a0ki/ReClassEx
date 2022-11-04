#pragma once
#include "stdafx.h"

namespace utils {
	extern string TextFormat(const char* format, ...);
	extern wstring TextFormatW(const char* format, ...);
	extern std::string tm_to_readable_time(tm ctx);
	extern std::time_t string_to_timet(std::string timestamp);
	extern std::tm timet_to_tm(time_t timestamp);
	extern DWORD FindThreadID(HWND Window);
	extern std::string random_string(size_t length);
	extern std::string get_currentpath();
	extern uint32_t getpIdByname(std::wstring a1);
	extern int random_int(int max, int min);
	extern float random_float(float max, float min);
}