#pragma once

#include <IQt5Wrapper.h>

class Injector {
public:
	static Injector *instance() { return ins ? ins : ins = new Injector(); }
private:
	Injector();
private:
	static Injector* ins;
public:
	bool Init(const IQt5Wrpaaer* wrapper);
};