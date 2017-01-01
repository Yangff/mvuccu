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

wchar_t * toWideString(QString str) {
	wchar_t * wstr = NULL;
	wstr = new wchar_t[str.length() + 1];
	str.toWCharArray(wstr);
	wstr[str.length()] = 0;
	return wstr;
}

void output(wchar_t *tag, QString msg) {
	if (uccuConfig::instance().enableLog() && !fLog) {
		auto tmp = QDir::tempPath();
		auto path = toWideString(tmp + "/RMMVLog.log");
		wprintf(L"%s\n", path);
		fLog = _wfopen(path, L"wb");
		delete[] path;
	}
	if (msg.length() <= 500) {
		auto _msg = toWideString(msg);
		if (uccuConfig::instance().enableConsoleWindow())
			wprintf(L"%s %s\n", tag, _msg);
		if (uccuConfig::instance().enableLog())
			fwprintf(fLog, L"%s %s\n", tag, _msg);
		delete[] _msg;
	}
	else {
		if (uccuConfig::instance().enableConsoleWindow()) {
			auto _msg = toWideString(msg.left(500));
			wprintf(L"%s %s...\n", tag, _msg);
			delete[] _msg;
		}
		if (uccuConfig::instance().enableLog()) {
			auto _msg0 = toWideString(msg.left(500));
			fwprintf(fLog, L"%s (Long Message) \n%s", tag, _msg0);
			delete _msg0;
			for (int i = 500; i < msg.length(); i += 500) {
				auto _msgi = toWideString(msg.mid(i, 500));
				fwprintf(fLog, L"%s", _msgi);
				delete[] _msgi;
			}
			fwprintf(fLog, L"(End Of Long Message)\n");
		}
	}
	if (uccuConfig::instance().enableLog()) fflush(fLog);
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
