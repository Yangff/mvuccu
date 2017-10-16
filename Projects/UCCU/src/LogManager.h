#pragma once

#include <QtCore/qstring.h>


void platformQtMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & msg);
void platformLog(QString);
void platformErr(QString);


class LogManager {
public:
	static LogManager& instance() { return *(ins ? ins : ins = new LogManager()); }
private:
	LogManager() {};
private:
	static LogManager* ins;

public:
	void log(QString);
	void err(QString);

	static void qtMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & msg) {
		platformQtMessageHandler(type, context, msg);
	}
};

