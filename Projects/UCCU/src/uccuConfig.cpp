#include "uccuConfig.h"

#include <QtCore/qfile.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qvariant.h>

int parseFlagString(QString s) {
	int result = 0;
	for (auto x : s) {
		if (x == QChar('D')) {
			result |= 1;
		}
		if (x == QChar('C')) {
			result |= 2;
		}
		if (x == QChar('W')) {
			result |= 4;
		}
		if (x == QChar('F')) {
			result |= 8;
		}
	}
	return result;
}

uccuConfig::uccuConfig(QString cfgFile) :enableLang(0), enableConsole(0), enableLogFile(0) {
	// strange bug
	// vs said it's esp changed.
	// wired...

	QFile f(cfgFile);
	if (f.open(QFileDevice::OpenModeFlag::ReadOnly)) {
		QJsonParseError err;
		auto x = f.readAll();
		auto doc = QJsonDocument::fromJson(x, &err);
		if (err.error == QJsonParseError::NoError) {
			auto obj = doc.object();
			if (obj["EnableLang"].isBool() && (enableLang = obj["EnableLang"].toBool()))
				langFile = obj["LangFile"].toString();
			if (obj["EnableConsole"].isBool())
				enableConsole = obj["EnableConsole"].toBool();
			if (obj["EnableLog"].isBool())
				enableLogFile = obj["EnableLog"].toBool();
			if (obj["CategoryMode"].isObject()) {
				auto cate = obj["CategoryMode"].toObject();
				auto keys = cate.toVariantMap().keys();
				for (auto x : keys) {
					if (cate[x].isString())
						categoryMode[x] = parseFlagString(cate[x].toString());
				}
			}
		}
		else goto err;
	}
	else goto err;
	return;
err:
	langFile = "";
	enableLang = 0;
	enableConsole = 0;
	enableLogFile = 0;
	categoryMode.clear();
}

QString uccuConfig::GetLanguageFile()
{
	return langFile;
}

bool uccuConfig::enableLanguageFix()
{
	return enableLang;
}

bool uccuConfig::enableConsoleWindow()
{
	return enableConsole;
}

bool uccuConfig::enableLog()
{
	return enableLogFile;
}

int uccuConfig::GetCategoryMode(QString s)
{
	return categoryMode[s];
}

uccuConfig *uccuConfig::_instance = 0;