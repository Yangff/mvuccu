#pragma once
#include <qtcore/qstring>
#include <QtCore/qmap.h>

class uccuConfig {
private:
	uccuConfig(QString cfgFile = "uccu.json");
	static uccuConfig *_instance;
public:
	static uccuConfig& instance() {
		if (_instance) {
			return *_instance;
		}
		else {
			return *(_instance = new uccuConfig());
		};
	}

	QString GetLanguageFile();
	bool enableLanguageFix();
	bool enableConsoleWindow();
	bool enableLog();
	int GetCategoryMode(QString);
private:
	QMap<QString, int> categoryMode;
	QString langFile;
	bool enableLang;
	bool enableLogFile;
	bool enableConsole;
};
