#pragma once

class CQt5WrapperV1 : public IQt5Wrapper {
private:
	p_QTranslator_load AddrTranslator;
	p_qRegisterResourceData AddrRegister;
	p_qRegisterResourceData AddrUnregister;

public:

	p_QTranslator_load LoadHooked;
	p_qRegisterResourceData RegisterHooked;
	p_Callback AfterSetOrganizationDomainHooked;

	p_QCoreApplication_setOrganizationDomain AddrSetOrganizationDomain;
public:
	CQt5WrapperV1(p_QTranslator_load AddrTranslator, p_qRegisterResourceData AddrRegister, p_qRegisterResourceData AddrUnregister, p_QCoreApplication_setOrganizationDomain AddrSetOrganizationDomain) :
		AddrTranslator(AddrTranslator), AddrRegister(AddrRegister), AddrUnregister(AddrUnregister),
		LoadHooked(AddrTranslator), RegisterHooked(AddrRegister), AddrSetOrganizationDomain(AddrSetOrganizationDomain), AfterSetOrganizationDomainHooked(nullptr){};

	virtual void Set_QTranslator_load(p_QTranslator_load f) {
		LoadHooked = f;
	}

	virtual void Set_qRegisterResourceData(p_qRegisterResourceData f) {
		RegisterHooked = f;
	}

	virtual void Set_AfterQCoreApplication_setOrganizationDomain(p_Callback callback) {
		AfterSetOrganizationDomainHooked = callback;
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
};