#include <IQt5Wrapper.h>
#include <Windows.h>
#include "../../src/Injector.h"

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	) {

	if (fdwReason == DLL_PROCESS_ATTACH) {
		return TRUE;
	}

	if (fdwReason == DLL_PROCESS_DETACH) {
		return Injector::instance().OnExit();
	}

	return TRUE;
}

bool __stdcall InitUCCU(IQt5Wrpaaer * wrap) {
	return Injector::instance().Init(wrap);
}

int GetUCCUVersion() {
	return 1;
}