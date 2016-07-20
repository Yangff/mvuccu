#pragma once

#include <QtCore/qmap.h>
#include <QtCore/qdir.h>

#include "Mod.h"

class ModManager {
private:
	static ModManager * _instance;
	ModManager();

	bool m_bWaitingForRes;

public:
	QString rootPath = "./mvuccu/mods/";

	static ModManager & instance() {
		if (!_instance) {
			return *(_instance = new ModManager());
		}
		else return *_instance;
	};

	bool WaitingForRes();
	unsigned char* RunMods();
	QMap<QString, Mod> LoadMods();
	Version UCCUVersion();
};