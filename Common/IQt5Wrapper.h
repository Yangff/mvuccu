#pragma once


#include <QtCore/qstring.h>

typedef bool(__thiscall *p_QTranslator_load)(
	void const *thus,
	const QString & filename,
	const QString & directory,
	const QString & search_delimiters,
	const QString & suffix
	);

typedef bool(*p_QCoreApplication_setOrganizationDomain)(
	const QString & domain
	);

typedef bool(*p_qRegisterResourceData)(
	int version,
	const unsigned char *tree,
	const unsigned char *name,
	const unsigned char *data
	);

typedef bool(__thiscall *p_Callback)();


class IQt5Wrapper {
public:
	virtual void Set_QTranslator_load(p_QTranslator_load) = NULL;

	virtual void Set_qRegisterResourceData(p_qRegisterResourceData) = NULL;

	virtual void Set_AfterQCoreApplication_setOrganizationDomain(p_Callback) = NULL;

	virtual bool Call_QTranslator_load(void const *thus, const QString & filename,
		const QString & directory,
		const QString & search_delimiters,
		const QString & suffix) = NULL;

	virtual bool Call_qRegisterResourceData(int version,
		const unsigned char *tree,
		const unsigned char *name,
		const unsigned char *data) = NULL;

	virtual bool Call_qUnregisterResourceData(int version,
		const unsigned char *tree,
		const unsigned char *name,
		const unsigned char *data) = NULL;
};

typedef bool (__stdcall*pInitUCCU)(IQt5Wrapper *);