#include "f4se_common/f4se_version.h"
#include "f4se_common/Utilities.h"
#include "f4se_common/Relocation.h"
#include "f4se_common/BranchTrampoline.h"
#include "f4se_common/SafeWrite.h"
#include <shlobj.h>
#include "common/IFileStream.h"
//#include "Hooks_ObScript.h"
//#include "Hooks_Papyrus.h"
#include "Hooks_Scaleform.h"
/*#include "Hooks_Gameplay.h"
#include "Hooks_Memory.h"
#include "Hooks_GameData.h"
#include "Hooks_SaveLoad.h"
#include "Hooks_Input.h"
#include "Hooks_Debug.h"
#include "Hooks_Threads.h"
#include "Hooks_Camera.h"*/
#include "PluginManager.h"
#include "InternalSerialization.h"

#include "windows.h"
#include "f4se_loader_common/IdentifyEXE.h"

IDebugLog gLog;
void * g_moduleHandle = nullptr;

void WaitForDebugger(void)
{
	/*while(!IsDebuggerPresent())
	{
		Sleep(10);
	}

	Sleep(1000 * 2);*/
}

static bool isInit = false;

bool checkFalloutVersion(void)
{
	_MESSAGE("Checking FO76 version.");
	std::string procName = "Fallout76.exe";
	const std::string & runtimeDir = GetRuntimeDirectory();
	std::string procPath = runtimeDir + "\\" + procName;
	std::string		dllSuffix;
	ProcHookInfo	procHookInfo;

	// check exe version
	if (!IdentifyEXE(procPath.c_str(), false, &dllSuffix, &procHookInfo))
	{
		_ERROR("EXE version mismatch.");
		return false;
	}
	else
		return true;
}

void F4SE_Initialize(void)
{
	if(isInit) return;
	isInit = true;

	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout 76\\SFE\\sfe.log");

	if (!checkFalloutVersion())
		return;

	

#ifndef _DEBUG
	__try {
#endif

		FILETIME	now;
		GetSystemTimeAsFileTime(&now);

		_MESSAGE("SFE runtime: initialize (version = %d.%d.%d %08X %08X%08X, os = %s)",
			F4SE_VERSION_INTEGER, F4SE_VERSION_INTEGER_MINOR, F4SE_VERSION_INTEGER_BETA, RUNTIME_VERSION,
			now.dwHighDateTime, now.dwLowDateTime, GetOSInfoStr().c_str());

		_MESSAGE("imagebase = %016I64X", GetModuleHandle(NULL));
		_MESSAGE("reloc mgr imagebase = %016I64X", RelocationManager::s_baseAddr);


#ifdef _DEBUG
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

		WaitForDebugger();
#endif

		if(!g_branchTrampoline.Create(1024 * 64))
		{
			_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
			return;
		}

		if(!g_localTrampoline.Create(1024 * 64, g_moduleHandle))
		{
			_ERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
			return;
		}

		//Hooks_Debug_Init();
		//Hooks_ObScript_Init();
		//Hooks_Papyrus_Init();
		Hooks_Scaleform_Init();
		//Hooks_Gameplay_Init();
		//Hooks_GameData_Init();
		//Hooks_SaveLoad_Init();
		//Hooks_Input_Init();
		//Hooks_Threads_Init();
		//Hooks_Camera_Init();

		//g_pluginManager.Init();

		//Hooks_Debug_Commit();
		//Hooks_ObScript_Commit();
		//Hooks_Papyrus_Commit();
		Hooks_Scaleform_Commit();
		//Hooks_Gameplay_Commit();
		//Hooks_GameData_Commit();
		//Hooks_SaveLoad_Commit();
		//Hooks_Input_Commit();
		//Hooks_Threads_Commit();
		//Hooks_Camera_Commit();

		//Init_CoreSerialization_Callbacks();

		FlushInstructionCache(GetCurrentProcess(), NULL, 0);

#ifndef _DEBUG
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		_ERROR("exception thrown during startup");
	}
#endif

	_MESSAGE("init complete");
}

/*extern "C" {

	void StartF4SE(void)
	{
		F4SE_Initialize();
	}

	BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
	{
		switch(dwReason)
		{
		case DLL_PROCESS_ATTACH:
			g_moduleHandle = (void *)hDllHandle;
			F4SE_Initialize();
			break;

		case DLL_PROCESS_DETACH:
			break;
		};

		return TRUE;
	}

};*/




/*
Minimal dxgi.dll shim by reg2k (https://github.com/reg2k).
*/

using _CreateDXGIFactory = HRESULT(*)(REFIID riid, void** ppFactory);
using _CreateDXGIFactory2 = HRESULT(*)(UINT flags, REFIID riid, void** ppFactory);
using _DXGID3D10CreateDevice = HRESULT(*)(HMODULE hModule, void* pFactory, void* pAdapter, UINT Flags, void* pUnknown, void** ppDevice);
using _DXGID3D10CreateLayeredDevice = HRESULT(*)(void* pUnknown1, void* pUnknown2, void* pUnknown3, void* pUnknown4, void* pUnknown5);
using _DXGID3D10GetLayeredDeviceSize = SIZE_T(*)(const void *pLayers, UINT NumLayers);
using _DXGID3D10RegisterLayers = HRESULT(*)(const void *pLayers, UINT NumLayers);
_CreateDXGIFactory  CreateDXGIFactory_Original;
_CreateDXGIFactory  CreateDXGIFactory1_Original;
_CreateDXGIFactory2 CreateDXGIFactory2_Original;
_DXGID3D10CreateDevice DXGID3D10CreateDevice_Original;
_DXGID3D10CreateLayeredDevice DXGID3D10CreateLayeredDevice_Original;
_DXGID3D10GetLayeredDeviceSize DXGID3D10GetLayeredDeviceSize_Original;
_DXGID3D10RegisterLayers DXGID3D10RegisterLayers_Original;

HMODULE originalModule = NULL;
HMODULE dllHandle = NULL;
extern "C" {
	__declspec(dllexport) HRESULT __stdcall CreateDXGIFactory(REFIID riid, void** ppFactory) {
		static bool initDone = false;
		if (!initDone) {
			initDone = true;
			g_moduleHandle = (void *)dllHandle;
			F4SE_Initialize();
		}
		return CreateDXGIFactory_Original(riid, ppFactory);
	}

	__declspec(dllexport) HRESULT __stdcall CreateDXGIFactory1(REFIID riid, void** ppFactory) {
		return CreateDXGIFactory1_Original(riid, ppFactory);
	}

	__declspec(dllexport) HRESULT __stdcall CreateDXGIFactory2(UINT flags, REFIID riid, void** ppFactory) {
		return CreateDXGIFactory2_Original(flags, riid, ppFactory);
	}

	__declspec(dllexport) HRESULT __stdcall DXGID3D10CreateDevice(HMODULE hModule, void* pFactory, void* pAdapter, UINT Flags, void* pUnknown, void** ppDevice) {
		return DXGID3D10CreateDevice_Original(hModule, pFactory, pAdapter, Flags, pUnknown, ppDevice);
	}

	__declspec(dllexport) HRESULT __stdcall DXGID3D10CreateLayeredDevice(void* pUnknown1, void* pUnknown2, void* pUnknown3, void* pUnknown4, void* pUnknown5) {
		return DXGID3D10CreateLayeredDevice_Original(pUnknown1, pUnknown2, pUnknown3, pUnknown4, pUnknown5);
	}

	__declspec(dllexport) SIZE_T __stdcall DXGID3D10GetLayeredDeviceSize(const void *pLayers, UINT NumLayers) {
		return DXGID3D10GetLayeredDeviceSize_Original(pLayers, NumLayers);
	}

	__declspec(dllexport) HRESULT __stdcall DXGID3D10RegisterLayers(const void *pLayers, UINT NumLayers) {
		return DXGID3D10RegisterLayers_Original(pLayers, NumLayers);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		char path[MAX_PATH];
		GetSystemDirectoryA(path, MAX_PATH);
		strcat_s(path, "\\dxgi.dll");

		dllHandle = hModule;
		originalModule = LoadLibraryA(path);
		CreateDXGIFactory_Original = reinterpret_cast<_CreateDXGIFactory> (GetProcAddress(originalModule, "CreateDXGIFactory"));
		CreateDXGIFactory1_Original = reinterpret_cast<_CreateDXGIFactory> (GetProcAddress(originalModule, "CreateDXGIFactory1"));
		CreateDXGIFactory2_Original = reinterpret_cast<_CreateDXGIFactory2>(GetProcAddress(originalModule, "CreateDXGIFactory2"));
		DXGID3D10CreateDevice_Original = reinterpret_cast<_DXGID3D10CreateDevice>(GetProcAddress(originalModule, "DXGID3D10CreateDevice"));
		DXGID3D10CreateLayeredDevice_Original = reinterpret_cast<_DXGID3D10CreateLayeredDevice>(GetProcAddress(originalModule, "DXGID3D10CreateLayeredDevice"));
		DXGID3D10GetLayeredDeviceSize_Original = reinterpret_cast<_DXGID3D10GetLayeredDeviceSize>(GetProcAddress(originalModule, "DXGID3D10GetLayeredDeviceSize"));
		DXGID3D10RegisterLayers_Original = reinterpret_cast<_DXGID3D10RegisterLayers>(GetProcAddress(originalModule, "DXGID3D10RegisterLayers"));
		break;

	case DLL_PROCESS_DETACH:
		FreeLibrary(originalModule);
		break;
	}
	return TRUE;
}