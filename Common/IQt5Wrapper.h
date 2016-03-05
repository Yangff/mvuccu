#include <functional>
#include <QtCore/qstring.h>

class IQt5Wrpaaer {
public:
	virtual void Set_QTranslator_load(std::function<bool(void const *thus, const QString & filename,
		const QString & directory,
		const QString & search_delimiters,
		const QString & suffix)>) = NULL;

	virtual void Set_qRegisterResourceData(std::function<bool(int version,
		const unsigned char *tree,
		const unsigned char *name,
		const unsigned char *data)>) = NULL;

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

typedef bool(__stdcall *InitUCCU)(const IQt5Wrpaaer *);