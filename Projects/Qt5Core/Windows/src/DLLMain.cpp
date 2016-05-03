#include <Windows.h>
#include <string>
#include <IQt5Wrapper.h>

#include "../../src/Qt5Core.h"

HINSTANCE hinstDLL = NULL;
HMODULE hQt5Core = NULL;
HMODULE hUCCU = NULL;

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	) {

	if (fdwReason == DLL_PROCESS_ATTACH) {
		::hinstDLL = hinstDLL;
		hQt5Core = LoadLibrary(L"Qt5CoreOLD.dll");
	//	hUCCU = LoadLibrary(L"uccu\\mvUCCU.dll");
		if (hQt5Core /*&& hUCCU*/) {
			auto o_QTranslator_load = (p_QTranslator_load)GetProcAddress(hQt5Core, "?load@QTranslator@@QAE_NABVQString@@000@Z");
			auto o_qRegisterResourceData = (p_qRegisterResourceData)GetProcAddress(hQt5Core, "?qRegisterResourceData@@YA_NHPBE00@Z");
			auto o_qUnregisterResourceData = (p_qRegisterResourceData)GetProcAddress(hQt5Core, "?qUnregisterResourceData@@YA_NHPBE00@Z");

			wrapper = new CQt5WrpaaerV1(o_QTranslator_load, o_qRegisterResourceData, o_qUnregisterResourceData);
			/*(pInitUCCU initfunc = (pInitUCCU)GetProcAddress(hUCCU, "InitUCCU");
			if (initfunc(wrapper)) {
				return TRUE;
			}
			else return FALSE; */
			return true;
		} else return FALSE;
	}

	if (fdwReason == DLL_PROCESS_DETACH) {
		if (hQt5Core) {
			FreeLibrary(hQt5Core);
			hQt5Core = NULL;
		}
		if (hUCCU) {
			FreeLibrary(hUCCU);
			hUCCU = NULL;
		}
		if (wrapper) {
			delete wrapper;
		}
	}

	return TRUE;
}

int GetWrapperVersion() {
	return 1;
}