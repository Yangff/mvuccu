#define V8PP_ISOLATE_DATA_SLOT 0
#include <v8pp/module.hpp>
#include <v8pp/utility.hpp>
#include <v8pp/convert.hpp>
#include <v8pp/object.hpp>
#include <v8pp/call_v8.hpp>
#include <v8pp/class.hpp>

#include <v8.h>

#include <QtCore/qfileinfo>

#include "LogManager.h"
#include "JSCore.h"

#include "PlatformEnv.h"

namespace console {
	v8::UniquePersistent<v8::Value> _console;

	QString apply(QString x, v8::Handle<v8::Value> o) {
		auto iso = v8::Isolate::GetCurrent();
		QString current = x;
		if (o->IsString()) {
			return current.arg(*v8::String::Utf8Value(o));
		}
		if (o->IsInt32()) {
			return current.arg(v8pp::from_v8<int>(iso, o));
		}
		if (o->IsUint32()) {
			return current.arg(v8pp::from_v8<unsigned int>(iso, o));
		}
		if (o->IsNumber()) {
			return current.arg(v8pp::from_v8<double>(iso, o));
		}
		if (o->IsUndefined()) {
			return current.arg("undefined");
		}
		if (o->IsNull()) {
			return current.arg("null");
		}
		if (o->IsBoolean()) {
			return current.arg(v8pp::from_v8<bool>(iso, o));
		}
		if (o->IsObject()) {
			return current.arg(*v8::String::Utf8Value(o->ToString()));
		}
		return current.arg("?");
	}

	QString applyFormat(QString format, v8::FunctionCallbackInfo<v8::Value> const& args) {
		QString current(format);
		for (int i = 1; i < args.Length(); i++) {
			auto o = args[i];
			current = apply(current, o);
		}
		return current;
	}

	void log(v8::FunctionCallbackInfo<v8::Value> const& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		if (args.Length() == 0) {
			v8pp::throw_ex(v8::Isolate::GetCurrent() ,"console.log takes at least one argument");
			return;
		}
		v8::String::Utf8Value _format(args[0]);
		QString result = applyFormat(*_format, args);
		LogManager::instance().log(result);
		args.GetReturnValue().SetNull();
	}

	void err(v8::FunctionCallbackInfo<v8::Value> const& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		if (args.Length() == 0) {
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "console.log takes at least one argument");
			return;
		}
		v8::String::Utf8Value _format(args[0]);
		QString result = applyFormat(*_format, args);
		LogManager::instance().err(result);
		args.GetReturnValue().SetNull();
	}

	void init(v8::Isolate *iso) {
		v8pp::module m(iso);
		m.set("log", &log);
		m.set("err", &err);
		auto c = m.new_instance();
		_console.Reset(iso, c);
		iso->GetCurrentContext()->Global()->Set(v8::String::NewFromUtf8(iso, "console"), c);
	}

	void clear() {
		_console.Reset();
	}

	v8::Handle<v8::Value> console(v8::Isolate *iso) {
		return v8pp::to_local(iso, _console);
	}
};

namespace buffer {
	class buffer {
	public:
		QByteArray _buff;

		void SetBuff(QByteArray buff) {
			_buff = buff;
		}
	public:
		v8::Handle<v8::Value> ToString() {
			auto iso = v8::Isolate::GetCurrent();
			v8::EscapableHandleScope handle_scope(iso);
			v8::TryCatch try_catch;

			auto maybe = v8::String::NewFromOneByte(iso, (uint8_t*)_buff.data(), v8::NewStringType::kNormal, _buff.length());
			if (maybe.IsEmpty() || try_catch.HasCaught())
				return handle_scope.Escape(try_catch.ReThrow());;
			v8::Handle<v8::String> s = maybe.ToLocalChecked();
			return handle_scope.Escape(s);
		}

		explicit buffer(int len) {
			_buff.resize(len);
		}

		int GetLength() const {
			return _buff.length();
		}

		v8::Handle<v8::Value> clone() {
			return buffer::New(_buff);
		}

		void resize(int sz) {
			_buff.resize(sz);
		}

		void reserve(int sz) {
			_buff.reserve(sz);
		}

		buffer() { }
	public:
		static v8::Handle<v8::Value> New(QByteArray buff) {
			buffer *n = new buffer();
			n->SetBuff(buff);
			return v8pp::class_<buffer>::import_external(v8::Isolate::GetCurrent(), n);
		}

		static v8::Handle<v8::Value> fromString(const char* str) {
			buffer *n = new buffer();
			n->SetBuff(QByteArray(str));
			return v8pp::class_<buffer>::import_external(v8::Isolate::GetCurrent(), n);
		}
	};

	v8::Handle<v8::Function> init(v8::Isolate *iso) {
		v8pp::class_<buffer> cbuffer(iso);
		cbuffer.ctor<int>()
				.set("ToString", &buffer::ToString)
				.set("length", v8pp::property(&buffer::GetLength))
				.set("clone", &buffer::clone)
				.set("reserve", &buffer::reserve)
				.set("resize", &buffer::resize)
				.set("fromString", &buffer::fromString);
		cbuffer.class_function_template()->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			[](uint32_t index, v8::PropertyCallbackInfo<v8::Value> const& info) {
				auto klass = v8pp::from_v8<buffer&>(info.GetIsolate(), info.This());
				if (index >= (unsigned int)klass._buff.length()) {
					info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer out of range"));
				}
				else
					info.GetReturnValue().Set(v8pp::to_v8<int>(info.GetIsolate(), (int)klass._buff.at(index)));
			},
			[] (uint32_t index,
				v8::Local<v8::Value> value,
				const v8::PropertyCallbackInfo<v8::Value>& info) {
					auto& klass = v8pp::from_v8<buffer&>(info.GetIsolate(), info.This());
					if (value.IsEmpty()) {
						info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer accept only bytes"));
						return;
					}
					if (!(value->IsInt32()) && !(value->IsUint32()) && !(value->IsString())) {
						info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer accept only bytes"));
						return;
					}
					if (((value->IsInt32()) || (value->IsUint32())) && !((value->Int32Value()) <= 255 && (value->Int32Value()) >= 0)) {
						info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer accept only bytes"));
						return;
					}
					if ((value->IsString()) && ((value.As<v8::String>()->Length()) != 1)) {
						info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer accept only bytes"));
						return;
					}
					if (index >= (unsigned int)klass._buff.length()) {
						info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer out of range"));
					}
					else {
						if (value->IsString()) {
							klass._buff[index] = (*v8::String::Utf8Value(value.As<v8::String>()))[0];
						} else {
							klass._buff[index] = value->Int32Value();
						}
						info.GetReturnValue().Set(value);
					}
			}
		));
		cbuffer.class_function_template()->SetClassName(v8pp::to_v8(iso, "Buffer"));
		return cbuffer.js_function_template()->GetFunction();
	}

};

#include <QtCore/qmap.h>
#include <QtCore/qstack.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>


namespace fs {
	const int F_OK = 1;
	const int R_OK = 2;
	const int W_OK = 4;
	const int X_OK = 8;

	class Stats {
	private:
		QString path;
		bool lstat;
	public:
		bool isFile() {
			return QFileInfo(path).isFile();
		}
		bool isDirectory() {
			return QFileInfo(path).isDir();
		}
		bool isSymbolicLink() {
			return QFileInfo(path).isSymLink();
		}

		uint uid, gid; uint64_t size;

		void post_stat() {
			uid = QFileInfo(path).ownerId();
			gid = QFileInfo(path).groupId();
			size = QFileInfo(path).size();
		}

		Stats(const char *path, bool lstat):path(path), lstat(lstat) { 
			if (!lstat) {
				this->path = QFileInfo(path).canonicalFilePath();
			}
			post_stat();
		}
	};

	void accessSync(v8::FunctionCallbackInfo<v8::Value> const& args) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		v8::TryCatch try_catch;
		v8::String::Utf8Value file(args[0]);
		if (try_catch.HasCaught()) {
			return args.GetReturnValue().SetNull();
		}
		QFileInfo f(*file);
		int p = 0;
		p |= (f.exists()) * F_OK;
		p |= (f.isReadable()) * R_OK;
		p |= (f.isWritable()) * W_OK;
		p |= (f.isExecutable()) * X_OK;
		int mode = 1;
		if (args.Length() == 2) {
			mode = v8pp::from_v8<int>(isolate, args[1], 1);
		}
		if ((mode != 0) && ((mode & p) != 0))
			return args.GetReturnValue().SetNull();
		return args.GetReturnValue().Set(isolate->ThrowException(v8::String::NewFromUtf8(isolate, "no access")));
	}

	v8::Handle<v8::Value> readFileSync(const char * file) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(file);
		if (f.exists() && f.open(QIODevice::ReadOnly)) {
			auto d = f.readAll();
			f.close();
			return v8::String::NewFromOneByte(isolate, (uint8_t*)d.data(), v8::NewStringType::kNormal, d.length()).ToLocalChecked();
		}
		return v8::Null(isolate);
	}

	v8::Handle<v8::Value> readFileBufferSync(const char * file) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(file);
		if (f.exists() && f.open(QIODevice::ReadOnly)) {
			auto d = f.readAll();
			f.close();
			return buffer::buffer::New(d);
		}
		return v8::Null(isolate);
	}

	uint64_t writeFileSync(const char * file, const char *data) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(file);
		if (f.open(QIODevice::WriteOnly)) {
			uint64_t  l = f.write(data);
			f.close();
			return l;
		}
		return 0;
	}

	uint64_t writeFileBufferSync(const char * file, buffer::buffer* data) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		if (data->_buff.isNull()) {
			v8pp::throw_ex(isolate, "Bad buffer");
			return 0;
		}
		QFile f(file);
		if (f.open(QIODevice::WriteOnly)) {
			uint64_t  l = f.write(data->_buff);
			f.close();
			return l;
		}
		return 0;
	}

	uint64_t writeFileLenSync(const char * file, const char *data, uint64_t len) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(file);
		if (f.open(QIODevice::WriteOnly)) {
			uint64_t l = f.write(data, len);
			f.close();
			return l;
		}
		return 0;
	}

	uint64_t appendFileSync(const char * file, const char *data) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(file);
		if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
			uint64_t  l = f.write(data);
			f.close();
			return l;
		}
		return 0;
	}

	uint64_t appendFileBufferSync(const char * file, buffer::buffer* data) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		if (data->_buff.isNull()) {
			v8pp::throw_ex(isolate, "Bad buffer");
			return 0;
		}
		QFile f(file);
		if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
			uint64_t  l = f.write(data->_buff);
			f.close();
			return l;
		}
		return 0;
	}

	uint64_t appendFileLenSync(const char * file, const char *data, uint64_t len) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(file);
		if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
			uint64_t l = f.write(data, len);
			f.close();
			return l;
		}
		return 0;
	}

	v8::Handle<v8::Value> realpathSync(const char *file) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFileInfo f(file);
		if (f.exists()) {
			QString s = f.canonicalFilePath();
			auto str = v8::String::NewFromUtf8(isolate, s.toUtf8().data());
			return str;
		}
		return v8::Null(isolate);
	}

	bool renameSync(const char *oldPath, const char *newPath) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(oldPath);
		if (f.exists()) {
			return f.rename(newPath);
		}
		return false;
	}

	bool rmdirSync(const char *path) {
		QDir p(path);
		if (p.exists()) {
			return QDir().rmdir(path);
		}
		return false;
	}

	v8::Handle<v8::Value> statSync(const char *path) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		Stats *s = new Stats(path, false);
		return v8pp::class_<Stats>::import_external(isolate, s);
	}

	int internalModuleStat(const char *path) {
		QFileInfo info(path);
		if (info.exists()) {
			if (info.isFile())
				return 0;
			if (info.isDir())
				return 1;
			return -2;
		} return -2;
	}

	bool unlinkSync(const char *path) {
		return QFile(path).remove();
	}

	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::module m(iso);

		m.set_const("F_OK", F_OK);
		m.set_const("R_OK", R_OK);
		m.set_const("W_OK", W_OK);
		m.set_const("X_OK", X_OK);

		m.set("accessSync", &accessSync);
		m.set("readFileSync", &readFileSync);
		m.set("writeFileSync", &writeFileSync);
		m.set("readFileBufferSync", &readFileBufferSync);
		m.set("writeFileBufferSync", &writeFileBufferSync);
		m.set("writeFileLenSync", &writeFileLenSync);
		m.set("appendFileSync", &appendFileSync);
		m.set("appendFileBufferSync", &appendFileBufferSync);
		m.set("appendFileLenSync", &appendFileLenSync);
		m.set("realpathSync", &realpathSync);
		m.set("renameSync", &renameSync);
		m.set("rmdirSync", &rmdirSync);
		m.set("statSync", &statSync);
		m.set("internalModuleStat", &internalModuleStat);
		m.set("unlinkSync", &unlinkSync);

		v8pp::class_<Stats> stats(iso);
		stats.set("isFile", &Stats::isFile);
		stats.set("isDirectory", &Stats::isDirectory);
		stats.set("isSymbolicLink", &Stats::isSymbolicLink);

		stats.set("uid", &Stats::uid, true);
		stats.set("gid", &Stats::gid, true);
		stats.set("size", &Stats::size, true);
		
		m.set("Stats", stats);

		return m.new_instance();
	}
};

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

	v8::Persistent<v8::ObjectTemplate, v8::CopyablePersistentTraits<v8::ObjectTemplate>> environment;


	void exit(int code = 0) {
		::exit(code);
	}

	v8::Handle<v8::String> cwd() {
		return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), QDir::currentPath().toUtf8().data());
	}

	v8::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>> _process;

	void clear() {
		environment.Reset();
		_process.Reset();
	}

	void dlopen() {

	}

	void init(v8::Isolate *iso) {
		v8pp::module m(iso);

		// platform
#ifdef Q_OS_WIN
		m.set_const("platform", v8::String::NewFromTwoByte(iso, (uint16_t*) L"win32"));
#else
		#ifdef Q_OS_MACX
			m.set_const("platform", "darwin");
		#else
			m.set_const("platform", "unsupported");
		#endif
#endif
		// env
		q = QProcessEnvironment::systemEnvironment();
		
		v8::Handle<v8::ObjectTemplate> result = v8::ObjectTemplate::New(iso);
		result->SetNamedPropertyHandler(GetEnv, SetEnv);
		//environment.Reset(iso, result);
		m.set_const("env", result);
		
		// execPath

		auto _path = GetExecPathUTF8();
		m.set_const("execPath", (const char *)_path);
		delete[] _path;
		m.set_const("pid", (unsigned int)GetPid());
		m.set_const("version", "1.0.0");
		

		m.set("exit", &exit);

		m.set("cwd", &cwd);

/*		auto args = QCoreApplication::arguments();
		v8::Local<v8::Array> argv = v8::Array::New(iso, args.length());
		for (int i = 0; i < args.length(); i++) {
			argv->Set(i, v8::String::NewFromUtf8(iso, args[i].toUtf8().data()));
		}
		m.set_const("argv", argv);*/

		auto i = m.new_instance();
		i->Set(v8::String::NewFromUtf8(iso, "mainModule"), v8::String::NewFromUtf8(iso, ""));

		_process.Reset(iso, i);
		
		iso->GetCurrentContext()->Global()->Set(v8::String::NewFromUtf8(iso, "process"), i);
	}

	v8::Handle<v8::Value> getProcess(v8::Isolate *iso) {
		return v8::Local<v8::Value>::New(iso, _process);
	}

};

namespace vm {

	v8::Local<v8::Value> _runInContext(v8::Isolate *iso, v8::Local<v8::Context> cxt, v8::Local<v8::String> source, v8::Local<v8::String> filename, int line, int col) {
		v8::EscapableHandleScope handle_scope(iso);

		v8::TryCatch try_catch(iso);
		v8::ScriptOrigin origin(filename, v8::Integer::New(iso, line), v8::Integer::New(iso, col));
		auto _script = v8::Script::Compile(cxt, source, &origin);

		if (_script.IsEmpty()) {
			if (try_catch.HasCaught()) {
				if (try_catch.HasTerminated())
					iso->CancelTerminateExecution();
				return try_catch.ReThrow();
			}
		}

		auto script = _script.ToLocalChecked();
		
		auto result = script->Run(cxt);

		if (result.IsEmpty()) {
			return try_catch.ReThrow();
		}

		if (try_catch.HasCaught()) {
			if (try_catch.HasTerminated())
				iso->CancelTerminateExecution();
			return try_catch.ReThrow();
		}
		return handle_scope.Escape(result.ToLocalChecked());
	}

	v8::Local<v8::Value> GetFilename(v8::Local<v8::Value> x) {
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
			return v8pp::throw_ex(isolate, "options must be an object");
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
	
	void runInThisContext(v8::FunctionCallbackInfo<v8::Value> const& args) {
		auto isolate = v8::Isolate::GetCurrent();
		v8::EscapableHandleScope handle_scope(isolate);
		v8::TryCatch try_catch(isolate);
		auto code = args[0].As<v8::String>();
		auto filename = GetFilename(args[1]);
		auto lineOffset = GetOffsetArg(args[1], "lineOffset");
		auto columnOffset = GetOffsetArg(args[1], "columnOffset");

		if (try_catch.HasCaught()) {
			args.GetReturnValue().Set(try_catch.ReThrow());
			return;
		}

		// auto context = v8::Context::New(isolate, NULL, global::global(isolate));
		
		auto result = _runInContext(
			v8::Isolate::GetCurrent(),
			isolate->GetCurrentContext(),
			code, 
			filename.As<v8::String>(),
			lineOffset,
			columnOffset
		);
		if (try_catch.HasCaught() || result.IsEmpty()) {
			args.GetReturnValue().Set(try_catch.ReThrow());
			return;
		}
		args.GetReturnValue().Set(result);
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
	v8::Persistent<v8::Array, v8::CopyablePersistentTraits<v8::Array>> wrapper;
	QMap<QString, v8::Persistent<v8::Value, v8::CopyablePersistentTraits<v8::Value>>> modules;

	void clear() {
		modules.clear();
		wrapper.Reset();
	}

	void addmodule(const char * name, v8::Handle<v8::Value> obj) {
		modules[name].Reset(v8::Isolate::GetCurrent(), v8::Persistent<v8::Object>(v8::Isolate::GetCurrent(), obj.As<v8::Object>()));
	}
	
	v8::Handle<v8::Value> require(const char * module) {
		if (modules.find(module) != modules.end())
			return v8::Local<v8::Value>::New(v8::Isolate::GetCurrent(), modules[module]);
		QString e("Cannot find internal module ("); e += module; e += ").";
		return v8pp::throw_ex(v8::Isolate::GetCurrent(), e.toUtf8().data());
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
				_f.close();
				v8::Local<v8::Object> module = v8::Object::New(isolate);
				v8::Local<v8::Object> exports = v8::Object::New(isolate);
				auto str_exports = v8::String::NewFromUtf8(isolate, "exports");
				module->Set(str_exports, exports);

				auto filename = v8::String::NewFromUtf8(isolate, f.fileName().toUtf8().data());
				auto dirname = v8::String::NewFromUtf8(isolate, f.absoluteDir().absolutePath().toUtf8().data());

				// v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global::global(isolate));
				v8::Local<v8::String> source =
					v8::String::NewFromUtf8(isolate, s.toUtf8().data(),
						v8::NewStringType::kNormal).ToLocalChecked();
				
				v8::TryCatch try_catch(isolate);
				v8::Local<v8::Value> result = vm::_runInContext(isolate, isolate->GetCurrentContext(), source, filename, 0, 0);
				if (try_catch.HasCaught()) {
					if (try_catch.HasTerminated())
						isolate->CancelTerminateExecution();
					return try_catch.ReThrow();
				}
				if (result.IsEmpty()) {
					return try_catch.ReThrow();
				}
				
				auto func = v8::Local<v8::Function>::Cast(result);
				result = v8pp::call_v8(isolate, func, isolate->GetCurrentContext()->Global(), exports, v8pp::wrap_function(isolate, "require", &require), module, dirname, filename);
				if (try_catch.HasCaught()) {
					if (try_catch.HasTerminated())
						isolate->CancelTerminateExecution();
					return try_catch.ReThrow();
				}
				if (result.IsEmpty()) {
					return try_catch.ReThrow();
				}
				exports = module->Get(str_exports)->ToObject();
				addmodule(name,  exports);
				return handle_scope.Escape(exports);
			}
			return isolate->ThrowException(v8::String::NewFromUtf8(isolate, QString("Open file (%1) failed.").arg(f.absoluteFilePath()).toUtf8().data()));
		}
		return isolate->ThrowException(v8::String::NewFromUtf8(isolate, QString("File (%1) not found.").arg(f.absoluteFilePath()).toUtf8().data()));
		
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

#include "QmlProcesser.h"
#include "QmlNode.h"
#include "ResourceManager.h"

namespace ModAPI {
	using namespace Qml;
	using ::buffer::buffer;

	v8::Handle<v8::Value> get(const char *file) {
		auto iso = v8::Isolate::GetCurrent();
		QByteArray content = ResourceManager::instance().GetFileContent(":/qml/" + QString(file));
		//v8::Handle<v8::String> s = v8::String::NewFromOneByte(iso, (uint8_t*)content.data(), v8::NewStringType::kNormal, content.length()).ToLocalChecked();
		//return s;
		return buffer::New(content);
	}
	bool update(const char *file, buffer* context) {
		//v8::String::Utf8Value d(context);
		if (context->_buff == nullptr) {
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.");
			return false;
		}
		return ResourceManager::instance().UpdateFileContent(":/qml/" + QString(file), context->_buff);
	}
	bool add(const char *file, buffer* context) {
		//v8::String::Utf8Value d(context);
		if (context->_buff == nullptr) {
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.");
			return false;
		}
		return ResourceManager::instance().AddFile(":/qml/" + QString(file), context->_buff);
	}
	void each(const char * path, v8::Handle<v8::Function> callback) {
		ResourceManager::instance().Each(":/qml/" + QString(path), [callback] (QString path, bool isDir) {
			auto iso = v8::Isolate::GetCurrent();
			if (path.startsWith(":/qml/")) {
				auto _path = path.right(path.length() - 6);
				if (_path.length() != 0)
					v8pp::call_v8(iso, callback, iso->GetCurrentContext()->Global(), (const char*)_path.toUtf8().data(), !!isDir);
			}
		});
	}

	class qmlref {
	private:
		QSharedPointer<QmlNode> m_pNode;
		QString m_sField;
	public:
		qmlref(QSharedPointer<QmlNode> x, QString field) :m_pNode(x), m_sField(field) {} // no cons from js
		
	};

	class qmlnode {
	private:
		QSharedPointer<QmlNode> m_pNode;
	private:
		void create(QString typeId) { m_pNode = QSharedPointer<Qml::QmlNode>(new QmlNode(typeId)); };
	public:
		explicit qmlnode(const char *typeId):m_pNode() {
			create(typeId);
		}
		qmlnode(QSharedPointer<QmlNode> x):m_pNode(x){};
	public:
		static v8::Handle<v8::Value> New(QSharedPointer<QmlNode> x) {
			qmlnode *n = new qmlnode(x);
			return v8pp::class_<qmlnode>::import_external(v8::Isolate::GetCurrent(), n);
		}
	};

	class qmldocument {
	private:
		QSharedPointer<Document> m_pDoc;
	public:
		explicit qmldocument(buffer* buffer) {
			if (buffer->_buff == nullptr) {
				v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.");
				return;
			}
			QmlProcessor process(buffer->_buff);
			m_pDoc = QSharedPointer<Qml::Document>(process.GenerateDocument());
		}
	public:
		static v8::Handle<v8::Value> New(buffer* buffer) {
			if(buffer->_buff == nullptr) {
				return v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.");
			}
			qmldocument *n = new qmldocument(buffer);
			return v8pp::class_<qmldocument>::import_external(v8::Isolate::GetCurrent(), n);
		}
	public:
		v8::Handle<v8::Value> get_root() {
			return qmlnode::New(m_pDoc->root);
		}
	};

	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		
		v8pp::class_<qmldocument> cdoc(iso);
		v8pp::class_<qmlnode> cnode(iso);
		v8pp::class_<qmlref> cref(iso);

		cdoc.ctor<buffer*>()
			.set("node", v8pp::property(&qmldocument::get_root));
		cnode.ctor<const char*>();

		v8pp::module m(iso);
		m.set_const("api", "1.0.0");
		m.set("QMLDocument", cdoc);
		m.set("QMLNode", cnode);

		m.set("get", &get);
		m.set("update", &update);
		m.set("add", &add);
		m.set("each", &each);

		return m.new_instance();
	}
};


void JSCore::initAll(v8::Isolate * iso) {
	v8::TryCatch try_catch;
	//global::init(iso);
	console::init(iso);
	

	if (try_catch.HasCaught()) {
		try_catch.ReThrow();
		return;
	}
	process::init(iso);
	if (try_catch.HasCaught()) {
		try_catch.ReThrow();
		return;
	}
	auto nm = native_module::init(iso);
	if (try_catch.HasCaught()) {
		try_catch.ReThrow();
		return;
	}

	native_module::addmodule("native_module", nm);
	native_module::addmodule("fs", fs::init(iso));
	native_module::addmodule("buffer", buffer::init(iso));
	native_module::addmodule("vm", vm::init(iso));
	native_module::addmodule("modapi", ModAPI::init(iso));

	native_module::load("util", "./mvuccu/lib/util.js");
	if (try_catch.HasCaught()) {
		try_catch.ReThrow();
		return;
	}
	native_module::load("assert", "./mvuccu/lib/assert.js");
	if (try_catch.HasCaught()) {
		try_catch.ReThrow();
		return;
	}
	native_module::load("path", "./mvuccu/lib/path.js");
	if (try_catch.HasCaught()) {
		try_catch.ReThrow();
		return;
	}
	native_module::load("module", "./mvuccu/lib/module.js");
	if (try_catch.HasCaught()) {
		try_catch.ReThrow();
		return;
	}
}

v8::Handle<v8::Object> JSCore::getGlobal(v8::Isolate * iso) {
	return iso->GetCurrentContext()->Global();
}

v8::Handle<v8::Value> JSCore::require(const char * module) {
	return native_module::require(module);
}

v8::Handle<v8::Value> JSCore::load(const char * name, const char * path) {
	return native_module::load(name, path);
}

void JSCore::clearAll(v8::Isolate*iso)
{
	native_module::clear();
	console::clear();
	process::clear();
	v8pp::cleanup(iso);
}
