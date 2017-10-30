#pragma once

#include <IQt5Wrapper.h>
#include <QtCore/qstring.h>

class Injector {
public:
	static Injector& instance() { return *(ins ? ins : ins = new Injector()); }
private:
	Injector() {};
private:
	static Injector* ins;
public:
	bool Init(IQt5Wrapper* wrapper);
	bool OnExit();
	bool bQappTriggered;
	IQt5Wrapper* Wrapper;
public:
	void replaceTranslator(QString original, QString replace);
	bool addTranslator(QString fname);
};