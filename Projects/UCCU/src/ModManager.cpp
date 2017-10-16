#include "ModManager.h"
#include "ResourceManager.h"
#include "ScriptCore.h"

ModManager* ModManager::_instance;

bool ModManager::WaitingForRes() {
	return m_bWaitingForRes;
}

unsigned char * ModManager::RunMods() {
	// m_bWaitingForRes = false;
	ResourceManager::instance().AddFile(":/mod/version", QByteArray("2001", 4));

	// run mods

	ScriptCore::instance().RunScript();

	// cleanup

	QByteArray & x = ResourceManager::instance().finish();

	int size = x.size();

	unsigned char * buff = new unsigned char[size];
	memset(buff, 0, size);
	memcpy(buff, x.constData(), sizeof(char) * x.size());

	return buff;
}

QMap<QString, Mod> ModManager::LoadMods()
{
	QMap<QString, Mod> mods;
	QDir dir(rootPath);
	if (!dir.exists()) {
		dir.mkdir(".");
		return mods;
	}
	for (auto x : dir.entryInfoList()) {
		if (x.isDir()) {
			Mod mod(x.absoluteFilePath());
			if (mod.isMod()) {
				mods[mod.name] = mod;
			}
		}
	}
	return mods;
}

Version ModManager::UCCUVersion()
{
	return Version(1,0,0);
}

void ModManager::MarkFound()
{
	m_bWaitingForRes = false;
}

ModManager::ModManager() {
	m_bWaitingForRes = true; 
}