#include "LogManager.h"

LogManager* LogManager::ins;

void LogManager::log(QString s) {
	platformLog(s);
}
void LogManager::err(QString s) {
	platformErr(s);
}