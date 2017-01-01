#define _CRT_SECURE_NO_WARNINGS

#include <IQt5Wrapper.h>
#include <Windows.h>

#include "../../src/Injector.h"
#include "../../src/uccuConfig.h"

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	) {

	if (fdwReason == DLL_PROCESS_ATTACH) {
		return TRUE;
	}

	if (fdwReason == DLL_PROCESS_DETACH) {
		if (uccuConfig::instance().enableConsoleWindow())
			FreeConsole();
		return Injector::instance().OnExit();
	}

	return TRUE;
}

bool __stdcall InitUCCU(IQt5Wrapper * wrap) {
	if (uccuConfig::instance().enableConsoleWindow()) {
		AllocConsole();
		freopen("CONOUT$", "w+t", stdout);
	}
	return Injector::instance().Init(wrap);
}

int GetUCCUVersion() {
	return 1;
}