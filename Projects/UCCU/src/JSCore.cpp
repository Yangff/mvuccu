#include <v8pp/module.hpp>
#include <v8.h>

#include <QtCore/qfileinfo>

#include "LogManager.h"

namespace console {
	void log(v8::FunctionCallbackInfo<v8::Value> const& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		for (int i = 0; i < args.Length(); i++) {
			if (i > 0) LogManager::instance().log(" ");
			v8::String::Utf8Value str(args[i]);
			LogManager::instance().log(*str);
		}
		LogManager::instance().log("\n");
	}

	void err(v8::FunctionCallbackInfo<v8::Value> const& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		for (int i = 0; i < args.Length(); i++) {
			if (i > 0) LogManager::instance().err(" ");
			v8::String::Utf8Value str(args[i]);
			LogManager::instance().err(*str);
		}
		LogManager::instance().err("\n");
	}

	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::module m(iso);
		m.set("log", &log);
		m.set("err", &err);
		return m.new_instance();
	}
};

#define V8Func(name) v8::Handle<v8::Value> name(v8::FunctionCallbackInfo<v8::Value> const& args)

namespace fs {
	const int F_OK = 1;
	const int R_OK = 2;
	const int W_OK = 4;
	const int X_OK = 8;
	v8::Handle<v8::Value> accessSync(v8::FunctionCallbackInfo<v8::Value> const& args) {
		QFileInfo f(*v8::String::Utf8Value(args[0]));
		int p = 0;
		p |= (f.exists()) * F_OK;
		p |= (f.isReadable()) * R_OK;
		p |= (f.isWritable()) * W_OK;
		p |= (f.isExecutable()) * X_OK;
		int mode = 1;
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		if (args.Length() == 2) {
			mode = v8pp::from_v8<int>(isolate, args[1]);
		}
		if ((mode != 0) && ((mode & p) != 0))
			return v8::Null(isolate);
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "no access"));
	}


	v8::Handle<v8::Value> readFileSync(v8::FunctionCallbackInfo<v8::Value> const& args) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		return v8::Null(isolate);
	}
	
	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::module m(iso);
		m.set("readFileSync", &readFileSync);
		m.set("accessSync", &accessSync);

		return m.new_instance();
	}
};
