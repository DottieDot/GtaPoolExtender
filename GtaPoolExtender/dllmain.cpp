// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

#include "main.hpp"

#include <thread>

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		std::thread thrd(patch, hModule);
		thrd.detach();
	}
	break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		cleanup();
		break;
	}
	return TRUE;
}

