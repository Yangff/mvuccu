#pragma once

#include <v8.h>

namespace JSCore{
	void initAll(v8::Isolate * iso);
	v8::Handle<v8::Object> getGlobal(v8::Isolate *iso);
	v8::Handle<v8::Value> require(const char * module);
	v8::Handle<v8::Value> load(const char * name, const char * path);
};