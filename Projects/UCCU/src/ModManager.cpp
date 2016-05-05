#include "ModManager.h"
#include "ResourceManager.h"
#include "ScriptCore.h"

ModManager* ModManager::_instance;

bool ModManager::WaitingForRes() {
	return m_bWaitingForRes;
}

unsigned char * ModManager::RunMods() {
	m_bWaitingForRes = false;
	ResourceManager::instance().AddFile(":/mod/version", QByteArray("2001", 4));

	// run mods

	

	// cleanup

	QByteArray & x = ResourceManager::instance().finish();

	int size = x.size();

	unsigned char * buff = new unsigned char[size];
	memset(buff, 0, size);
	memcpy(buff, x.constData(), sizeof(char) * x.size());

	return buff;
}

ModManager::ModManager() {
	m_bWaitingForRes = false; // TODO: Make this be true;
}