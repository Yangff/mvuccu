#include <QtCore/qcoreapplication.h>
#include <QtCore/qstring>
#include <QtCore/qtranslator.h>
#include <IQt5Wrapper.h>

#include "Qt5Core.h"

CQt5WrapperV1 *wrapper;

bool QTranslator::load(const QString & filename,
	const QString & directory,
	const QString & search_delimiters,
	const QString & suffix) {
	return wrapper->LoadHooked(this, filename, directory, search_delimiters, suffix);
}

bool qRegisterResourceData(
	int version,
	const unsigned char *tree,
	const unsigned char *name,
	const unsigned char *data
	) {
	return wrapper->RegisterHooked(version, tree, name, data);
}