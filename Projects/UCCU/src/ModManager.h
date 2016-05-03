#pragma once

#include <QtCore/qmap.h>
#include <QtCore/qdir.h>

class ModManager {
private:
	static ModManager * _instance;
	ModManager();

	bool m_bWaitingForRes;
public:
	QString rootPath = "./mods/";

	static ModManager & instance() {
		if (!_instance) {
			return *(_instance = new ModManager());
		}
		else return *_instance;
	};

	bool WaitingForRes();
	unsigned char* RunMods();
};