#include "uccuConfig.h"

#include <QtCore/qfile.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qjsonarray.h>

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

uccuConfig::uccuConfig(QString cfgFile) :enableLang(0), enableConsole(0), enableLogFile(0), enableDebug(0), v8DebugPort(5959), waitDebugger(0) {
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
			if (obj["v8"].isObject()) {
				auto v8 = obj["v8"].toObject();
				if (v8["flags"].isArray()) {
					v8Flags.clear();
					auto ary = v8["flags"].toArray();
					for (auto x : ary) {
						if (x.isString())
							v8Flags.append(x.toString().toUtf8());
					}
				}
				enableDebug = false;
				if (v8["debugger"].isObject()) {
					auto debugger = v8["debugger"].toObject();
					if (debugger["enable"].isBool()) {
						enableDebug = debugger["enable"].toBool();
					}
					if (debugger["port"].toInt() != 0) {
						int port = debugger["port"].toInt();
						if (port > 0 && port < 65536)
							v8DebugPort = port;
					}
					if (debugger["waitForConnection"].isBool())
						waitDebugger = debugger["waitForConnection"].toBool();
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
	enableDebug = 0;
	v8DebugPort = 5858;
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

QList<QByteArray> uccuConfig::GetV8Flags()
{
	return v8Flags;
}

bool uccuConfig::enableV8Debug()
{
	return enableDebug;
}

int uccuConfig::GetV8DebugPort()
{
	return v8DebugPort;
}

bool uccuConfig::waitForConnection()
{
	return waitDebugger;
}


uccuConfig *uccuConfig::_instance = 0;