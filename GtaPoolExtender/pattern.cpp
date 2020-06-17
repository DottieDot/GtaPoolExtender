#include "pattern.hpp"

#include <string>
#include <Windows.h>
#include <psapi.h>
#include <tlhelp32.h>

DWORD GetImageSize()
{
	_MODULEINFO minfo;
	return GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &minfo,
		sizeof(minfo))
		? minfo.SizeOfImage
		: 0;
}

void* ScanPattern(const char* pattern, const char* mask)
{
	char* base = reinterpret_cast<char*>(GetModuleHandle(nullptr));
	char* end = base + GetImageSize();
	size_t patternLen = strlen(mask);

	for (auto i = base; i < (end - patternLen); ++i)
	{
		for (size_t j = 0; j < patternLen; ++j)
		{
			if (i[j] != pattern[j] && mask[j] != '?')
			{
				break;
			}
			else if (j == (patternLen - 1))
			{
				return i;
			}
		}
	}

	return nullptr;
}
