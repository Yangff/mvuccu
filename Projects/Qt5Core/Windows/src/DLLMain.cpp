#include <Windows.h>
#include <string>
#include <IQt5Wrapper.h>

HINSTANCE hinstDLL = NULL;
HMODULE hQt5Core = NULL;
HMODULE hUCCU = NULL;

typedef bool(__thiscall *p_QTranslator_load)(
	void const *thus,
	const QString & filename,
	const QString & directory,
	const QString & search_delimiters,
	const QString & suffix
	);

typedef bool(*p_qRegisterResourceData)(
	int version,
	const unsigned char *tree,
	const unsigned char *name,
	const unsigned char *data
	);

typedef bool(*p_qUnregisterResourceData)(
	int version,
	const unsigned char *tree,
	const unsigned char *name,
	const unsigned char *data
	);

class CQt5Wrpaaer : public IQt5Wrpaaer {
private:
	p_QTranslator_load AddrTranslator;
	p_qRegisterResourceData AddrRegister;
	p_qUnregisterResourceData AddrUnregister;

	std::function<bool(void const *thus, const QString & filename,
		const QString & directory,
		const QString & search_delimiters,
		const QString & suffix)> TranslatorHooked;

	std::function<bool(int version,
		const unsigned char *tree,
		const unsigned char *name,
		const unsigned char *data)> RegisterHooked;
public:
	CQt5Wrpaaer(p_QTranslator_load AddrTranslator, p_qRegisterResourceData AddrRegister, p_qUnregisterResourceData AddrUnregister) :AddrTranslator(AddrTranslator), AddrRegister(AddrRegister), AddrUnregister(AddrUnregister) {};
	
	virtual void Set_QTranslator_load(std::function<bool(void const *thus, 
		const QString & filename,
		const QString & directory,
		const QString & search_delimiters,
		const QString & suffix)> f) {
		TranslatorHooked = f;
	}

	virtual void Set_qRegisterResourceData(std::function<bool(int version,
		const unsigned char *tree,
		const unsigned char *name,
		const unsigned char *data)> f) {
		RegisterHooked = f;
	}

	virtual bool Call_QTranslator_load(void const *thus, const QString & filename,
		const QString & directory,
		const QString & search_delimiters,
		const QString & suffix) {
		return AddrTranslator(thus, filename, directory, search_delimiters, suffix);
	}

	virtual bool Call_qRegisterResourceData(int version,
		const unsigned char *tree,
		const unsigned char *name,
		const unsigned char *data) {
		return AddrRegister(version, tree, name, data);
	}

	virtual bool Call_qUnregisterResourceData(int version,
		const unsigned char *tree,
		const unsigned char *name,
		const unsigned char *data) {
		return AddrUnregister(version, tree, name, data);
	}
} *wrapped = NULL;

BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	) {

	if (fdwReason == DLL_PROCESS_ATTACH) {
		::hinstDLL = hinstDLL;
		hQt5Core = LoadLibrary(L"Qt5CoreOLD.dll");
		hUCCU = LoadLibrary(L"uccu\\mvUCCU.dll");
		if (hQt5Core && hUCCU) {
			auto o_QTranslator_load = (p_QTranslator_load)GetProcAddress(hQt5Core, "?load@QTranslator@@QAE_NABVQString@@000@Z");
			auto o_qRegisterResourceData = (p_qRegisterResourceData)GetProcAddress(hQt5Core, "?qRegisterResourceData@@YA_NHPBE00@Z");
			auto o_qUnregisterResourceData = (p_qUnregisterResourceData)GetProcAddress(hQt5Core, "?qUnregisterResourceData@@YA_NHPBE00@Z");

			wrapped = new CQt5Wrpaaer(o_QTranslator_load, o_qRegisterResourceData, o_qUnregisterResourceData);
			InitUCCU initfunc = (InitUCCU)GetProcAddress(hUCCU, "InitUCCU");
			if (initfunc(wrapped)) {
				return TRUE;
			} else return FALSE;
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
		if (wrapped) {
			delete wrapped;
		}
	}

	return TRUE;
}

int GetWrapperVersion() {
	return 1;
}