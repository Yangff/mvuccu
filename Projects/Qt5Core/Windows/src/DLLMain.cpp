#include <Windows.h>
#include <string>
#include <IQt5Wrapper.h>

#include "../../src/Qt5Core.h"

HINSTANCE hinstDLL = NULL;
HMODULE hQt5Core = NULL;
HMODULE hUCCU = NULL;

const wchar_t* ignoreName = L"QtWebEngineProcess.exe";
wchar_t buff[2048];

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	) {

	if (fdwReason == DLL_PROCESS_ATTACH) {
		DWORD nSize = 2048;
		QueryFullProcessImageName(GetCurrentProcess(), NULL, buff, &nSize);
		size_t pathSize = wcslen(buff);
		bool match = true;

		if (pathSize >= 22) {
			for (int i = 0; i < 22; i++) {
				if (buff[pathSize - 22 + i] != ignoreName[i])
					match = false;
			}
		}
		else match = false;

		::hinstDLL = hinstDLL;
		hQt5Core = LoadLibrary(L"Qt5CoreOLD.dll");
		if (hQt5Core) {
			auto o_QTranslator_load = (p_QTranslator_load)GetProcAddress(hQt5Core, "?load@QTranslator@@QAE_NABVQString@@000@Z");
			auto o_qRegisterResourceData = (p_qRegisterResourceData)GetProcAddress(hQt5Core, "?qRegisterResourceData@@YA_NHPBE00@Z");
			auto o_qUnregisterResourceData = (p_qRegisterResourceData)GetProcAddress(hQt5Core, "?qUnregisterResourceData@@YA_NHPBE00@Z");
			auto o_QCoreApplication_setOrganizationDomain = (p_QCoreApplication_setOrganizationDomain)GetProcAddress(hQt5Core, "?setOrganizationDomain@QCoreApplication@@SAXABVQString@@@Z");

			wrapper = new CQt5WrapperV1(o_QTranslator_load, o_qRegisterResourceData, o_qUnregisterResourceData, o_QCoreApplication_setOrganizationDomain);

			if (!match) {
				size_t len = GetModuleFileName(hinstDLL, buff, 2048);
				std::wstring::size_type pos = std::wstring(buff, len).find_last_of(L"\\/");
				std::wstring root = std::wstring(buff).substr(0, pos);
				std::wstring path = root + L"\\/mvuccu";
				SetDllDirectory(path.c_str());
				SetCurrentDirectory(root.c_str());
				hUCCU = LoadLibrary(L"mvUCCU.dll");

				if (hUCCU) {
					pInitUCCU initfunc = (pInitUCCU)GetProcAddress(hUCCU, "InitUCCU");
					if (initfunc(wrapper))
						return TRUE;
					return FALSE;
				}
				return FALSE;
			}
			return TRUE;
			
		}
		return FALSE;
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