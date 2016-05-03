#include "ModManager.h"

ModManager* ModManager::_instance;

bool ModManager::WaitingForRes() {
	return m_bWaitingForRes;
}

unsigned char * ModManager::RunMods() {
	m_bWaitingForRes = false;
	return nullptr;
}

ModManager::ModManager() {
	m_bWaitingForRes = false; // TODO: Make this be true;
}