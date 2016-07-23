#include <v8pp/module.hpp>
#include <v8.h>

#include <QtCore/qfileinfo>

#include "LogManager.h"

namespace global {
	v8::Persistent<v8::ObjectTemplate> global_templ;

	v8::Handle<v8::ObjectTemplate> getGlobal(v8::Isolate *iso) {
		return v8::Local<v8::ObjectTemplate>::New(iso, global_templ);
	}

	void init(v8::Isolate *iso) {
		auto g = v8::ObjectTemplate::New(iso);
		global::global_templ.Reset(iso, g);
	}
};

namespace console {
	v8::Persistent<v8::Value> _console;
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

	void init(v8::Isolate *iso) {
		v8pp::module m(iso);
		m.set("log", &log);
		m.set("err", &err);
		auto c = m.new_instance();
		_console.Reset(iso, c);
		global::getGlobal(iso)->Set(iso, "console", c);
	}
};

#define V8Func(name) v8::Handle<v8::Value> name(v8::FunctionCallbackInfo<v8::Value> const& args)

#include <QtCore/qmap.h>
#include <QtCore/qstack.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>

#include <v8pp/call_v8.hpp>

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
			mode = v8pp::from_v8<int>(isolate, args[1], 1);
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


#include <QtCore/qcoreapplication.h>
#include <QtCore/QProcessEnvironment>
namespace process {
	QProcessEnvironment q;
	void GetEnv(v8::Local<v8::String> _property, const v8::PropertyCallbackInfo<v8::Value>& info) {
		v8::String::Utf8Value v(_property);
		
		if (q.contains(*v))
			info.GetReturnValue().Set(v8::String::NewFromUtf8(info.GetIsolate(), q.value(*v).toUtf8().data()));
		else info.GetReturnValue().SetNull();
	}

	void SetEnv(v8::Local<v8::String> _property, v8::Local<v8::Value> value,
		const v8::PropertyCallbackInfo<v8::Value>& info) {
		v8::String::Utf8Value p(_property);
		v8::String::Utf8Value v(value);
		q.insert(*p, *v);
	}

	void exit(int code = 0) {
		QCoreApplication::exit(code);
	}

	v8::Handle<v8::String> cwd() {
		return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), QDir::currentPath().toUtf8().data());
	}

	v8::Handle<v8::Array> hrtime() {

	}
	char * path;
	v8::Persistent<v8::Value> _process;
	void init(v8::Isolate *iso) {
		v8pp::module m(iso);

		// platform
#ifdef Q_OS_WIN
		m.set_const("platform", "win32");
#endif
#ifdef Q_OS_MACX
		m.set_const("platform", "darwin");
#endif
		// env
		q = QProcessEnvironment::systemEnvironment();
		
		v8::Handle<v8::ObjectTemplate> result = v8::ObjectTemplate::New(iso);
		result->SetNamedPropertyHandler(GetEnv, SetEnv);
		m.set("env", result);

		// execPath

		const char * _path = QCoreApplication::applicationFilePath().toUtf8().data();
		int l = strlen(_path) + 1;
		path = new char[l];
		for (int i = 0; i < l; i++)
			path[i] = _path[i];

		m.set("execPath", path);

		m.set("exit", &exit);

		m.set("cwd", &cwd);
		
		m.set("hrtime", &hrtime);

		auto args = QCoreApplication::arguments();
		v8::Local<v8::Array> argv = v8::Array::New(iso, args.length());
		for (int i = 0; i < args.length(); i++) {
			argv->Set(i, v8::String::NewFromUtf8(iso, args[i].toUtf8().data()));
		}
		m.set_const("argv", argv);

		auto i = m.new_instance();
		i->Set(v8::String::NewFromUtf8(iso, "mainModule"), v8::String::NewFromUtf8(iso, ""));

		_process.Reset(iso, i);
		
		global::getGlobal(iso)->Set(iso, "process", i);
	}

	v8::Handle<v8::Value> getProcess(v8::Isolate *iso) {
		return v8::Local<v8::Value>::New(iso, _process);
	}

};

namespace vm {
	v8::Local<v8::Script> _compile(v8::Isolate *iso, v8::Local<v8::Context> cxt, v8::Local<v8::String> source, v8::Local<v8::String> filename, int line, int col) {
		v8::ScriptOrigin origin(filename, v8::Integer::New(iso, line), v8::Integer::New(iso, col));
		v8::Local<v8::Script> script = v8::Script::Compile(cxt, source, &origin).ToLocalChecked();
		return script;
	}

	v8::Local<v8::Value> _runInContext(v8::Isolate *iso, v8::Local<v8::Context> cxt, v8::Local<v8::String> source, v8::Local<v8::String> filename, int line, int col) {
		v8::EscapableHandleScope handle_scope(iso);
		
		v8::Context::Scope context_scope(cxt);

		v8::TryCatch try_catch(iso);

		auto script = vm::_compile(iso, cxt, source, filename, 0, 0);
		v8::Local<v8::Value> result = script->Run(cxt).ToLocalChecked();

		if (try_catch.HasCaught()) {
			if (try_catch.HasTerminated())
				iso->CancelTerminateExecution();
			try_catch.ReThrow();
			return handle_scope.Escape(v8::Null(iso));
		}
		if (result.IsEmpty()) {
			try_catch.ReThrow();
		}
		return result;
	}

	v8::Local<v8::String> GetFilename(v8::Local<v8::Value> x) {
		using v8::String;
		using v8::Local;
		using v8::Value;
		auto isolate = v8::Isolate::GetCurrent();
		Local<String> defaultFilename =
			String::NewFromUtf8(isolate, "evalmachine.<anonymous>");

		if (x->IsUndefined()) {
			return defaultFilename;
		}
		if (x->IsString()) {
			return x.As<String>();
		}
		if (!x->IsObject()) {
			v8pp::throw_ex(isolate, "options must be an object");
			return Local<String>();
		}
		Local<String> key = String::NewFromUtf8(isolate, "filename");
		Local<Value> value = x.As<v8::Object>()->Get(key);
		if (value->IsUndefined())
			return defaultFilename;
		return value->ToString(isolate);
	}

	int GetOffsetArg(v8::Local<v8::Value> x, const char * name) {
		using v8::Local;
		using v8::Value;
		auto isolate = v8::Isolate::GetCurrent();
		auto defaultOffset = 0;

		if (!x->IsObject()) {
			return defaultOffset;
		}

		Local<v8::String> key = v8::String::NewFromUtf8(isolate, name);
		Local<Value> value = x.As<v8::Object>()->Get(key);

		return value->IsUndefined() ? defaultOffset : v8pp::from_v8<int>(isolate, value, 0);
	}
	
	v8::Local<v8::Value> runInThisContext(v8::FunctionCallbackInfo<v8::Value> const& args) {
		auto isolate = v8::Isolate::GetCurrent();
		v8::EscapableHandleScope handle_scope(isolate);
		v8::TryCatch try_catch(isolate);
		auto code = args[0].As<v8::String>();
		auto filename = GetFilename(args[1]);
		auto lineOffset = GetOffsetArg(args[1], "lineOffset");
		auto columnOffset = GetOffsetArg(args[1], "columnOffset");

		auto context = v8::Context::New(isolate, NULL, global::getGlobal(isolate));
		
		auto result = _runInContext(
			v8::Isolate::GetCurrent(),
			context, 
			code, 
			filename,
			lineOffset,
			columnOffset
		);
		if (try_catch.HasCaught())
			try_catch.ReThrow();
		if (result.IsEmpty())
			try_catch.ReThrow();
		return result;
	};
	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::module m(iso);
		m.set("runInThisContext", &runInThisContext);
		return m.new_instance();
	}
};

#include <QtCore/qmap.h>
#include <QtCore/qstring>

namespace native_module {
	const char * _wrapper[] = {"(function (exports, require, module, __filename, __dirname) {\n", "\n});"};
	v8::Persistent<v8::Array> wrapper;
	QMap<QString, v8::Persistent<v8::Value>> modules;

	void addmodule(const char * name, v8::Handle<v8::Object> obj) {
		modules[name] = v8::Persistent<v8::Object>(v8::Isolate::GetCurrent(), obj);
	}
	
	v8::Handle<v8::Value> require(const char * module) {
		if (modules.find(module) != modules.end())
			return v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), modules[module]);
	};

	bool nonInternalExists(const char * module) {
		if (modules.find(module) != modules.end())
			return true;
		return false;
	};

	QString _wrap(QString code) {
		return _wrapper[0] + code + _wrapper[1];
	}

	v8::Handle<v8::String> wrap(v8::Handle<v8::String> code) {
		v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());
		v8::String::Utf8Value s(code);
		QString qs(*s);
		QString r = _wrap(qs);
		return handle_scope.Escape(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), r.toUtf8().data()));
	}

	v8::Handle<v8::Value> load(const char * name, const char * path) {
		auto isolate = v8::Isolate::GetCurrent();
		v8::EscapableHandleScope handle_scope(isolate);
		QFileInfo f(path);
		if (f.exists()) {
			QFile _f(path);
			if (_f.open(QIODevice::ReadOnly)) {
				QString s = _wrap(_f.readAll());
				v8::Local<v8::Object> module = v8::Object::New(isolate);
				v8::Local<v8::Object> exports = v8::Object::New(isolate);
				module->Set(v8::String::NewFromUtf8(isolate, "exports"), exports);

				auto filename = v8::String::NewFromUtf8(isolate, f.fileName().toUtf8().data());
				auto dirname = v8::String::NewFromUtf8(isolate, f.absoluteDir().absolutePath().toUtf8().data());

				v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global::getGlobal(isolate));
				v8::Local<v8::String> source =
					v8::String::NewFromUtf8(isolate, s.toUtf8().data(),
						v8::NewStringType::kNormal).ToLocalChecked();
				
				v8::TryCatch try_catch(isolate);
				v8::Local<v8::Value> result = vm::_runInContext(isolate, context, source, filename, 0, 0);
				if (try_catch.HasCaught()) {
					try_catch.ReThrow();
					return handle_scope.Escape(v8::Null(isolate));
				}

				auto func = v8::Local<v8::Function>::Cast(result);
				v8pp::call_v8(isolate, func, context->Global(), exports, &require, module, dirname, filename);
				if (try_catch.HasCaught()) {
					if (try_catch.HasTerminated())
						isolate->CancelTerminateExecution();
					try_catch.ReThrow();
					return handle_scope.Escape(v8::Null(isolate));
				}
				addmodule(name, exports);
				return handle_scope.Escape(exports);
			}

		}
		else {
			isolate->ThrowException(v8::String::NewFromUtf8(isolate, QString("File (%1) not found.").arg(f.absoluteFilePath()).toUtf8().data));
			return handle_scope.Escape(v8::Null(isolate));
		}
	}
	
	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::module m(iso);
		auto ary = v8::Array::New(iso, 2);
		ary->Set(0, v8::String::NewFromUtf8(iso, _wrapper[0]));
		ary->Set(1, v8::String::NewFromUtf8(iso, _wrapper[1]));
		wrapper.Reset(iso, ary);
		m.set("wrapper", wrapper);
		m.set("wrap", &wrap);
		m.set("nonInternalExists", &nonInternalExists);
		m.set("require", &require);
		return m.new_instance();
	}

};