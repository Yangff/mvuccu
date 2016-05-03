#define _CRT_SECURE_NO_WARNINGS

#include <QtCore/qdir.h>
#include <cstdio>

#include "../../src/LogManager.h"
#include "../../src/uccuConfig.h"

FILE *fLog;

wchar_t* getTag(QtMsgType type) {
	switch (type) {
	case QtDebugMsg:
		return L"[Debug]";
	case QtWarningMsg:
		return L"[Warning]";
	case QtCriticalMsg:
		return L"[Critical]";
	case QtFatalMsg:
		return L"[Fatal]";
	}
	return L"[Unknown]";
}

void output(wchar_t *tag, QString msg) {
	/*
	if (uccuConfig::instance().enableLog() && !fLog) {
		fwprintf(stdout, (QDir::tempPath() + "/RMMVLog.log\n").toStdWString().c_str());
		fLog = _wfopen((QDir::tempPath() + "/RMMVLog.log").toStdWString().c_str(), L"wb");
	}
	if (msg.length() <= 500) {
		if (uccuConfig::instance().enableConsoleWindow())
			fwprintf(stdout, L"%s %s\n", tag, msg.toStdWString().c_str());
		if (uccuConfig::instance().enableLog())
			fwprintf(fLog, L"%s %s\n", tag, msg.toStdWString().c_str());
	}
	else {
		if (uccuConfig::instance().enableConsoleWindow())
			fwprintf(stdout, L"%s %s...\n", tag, msg.left(500).toStdWString().c_str());
		if (uccuConfig::instance().enableLog()) {
			fwprintf(fLog, L"%s %s", tag, msg.left(500).toStdWString().c_str());
			for (int i = 500; i < msg.length(); i += 500) {
				fwprintf(fLog, L"%s", msg.mid(i, 500).toStdWString().c_str());
			}
			fwprintf(fLog, L"\n");
		}
	}
	if (uccuConfig::instance().enableLog()) fflush(fLog);
	*/
}

void platformQtMessageHandler(QtMsgType type, const QMessageLogContext & context, const QString & msg) {
	output(getTag(type), msg);
}

void platformLog(QString s) {
	output(L"[Debug]", s);
}

void platformErr(QString s) {
	output(L"[Fatal]", s);
}
