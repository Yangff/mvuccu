#pragma once

class CQt5WrpaaerV1 : public IQt5Wrpaaer {
private:
	p_QTranslator_load AddrTranslator;
	p_qRegisterResourceData AddrRegister;

public:

	p_QTranslator_load LoadHooked;
	p_qRegisterResourceData RegisterHooked;
public:
	CQt5WrpaaerV1(p_QTranslator_load AddrTranslator, p_qRegisterResourceData AddrRegister) : 
		AddrTranslator(AddrTranslator), AddrRegister(AddrRegister), 
		LoadHooked(AddrTranslator), RegisterHooked(AddrRegister) {};

	virtual void Set_QTranslator_load(p_QTranslator_load f) {
		LoadHooked = f;
	}

	virtual void Set_qRegisterResourceData(p_qRegisterResourceData f) {
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
};