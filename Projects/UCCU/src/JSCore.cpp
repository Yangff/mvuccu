#define V8PP_ISOLATE_DATA_SLOT 0
#include <v8pp/module.hpp>
#include <v8pp/utility.hpp>
#include <v8pp/convert.hpp>
#include <v8pp/object.hpp>
#include <v8pp/call_v8.hpp>
#include <v8pp/class.hpp>
#include <v8pp/persistent.hpp>

#include <v8.h>

#include <QtCore/qfileinfo>

#include "LogManager.h"
#include "JSCore.h"
#include "uccuConfig.h"

#include "PlatformEnv.h"

#include <v8-debug.h>

namespace Helper {
	static v8::Handle<v8::String> QSTR2V8(QString str) {
		auto utf8 = str.toUtf8();
		return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), utf8.data(), v8::String::kNormalString, utf8.length());
	}
	static QString V82QSTR(v8::Handle<v8::Value> str) {
		return QString(*v8::String::Utf8Value(str));
	}
};

namespace v8stringlist {
	using namespace Helper;
	class list {
	private:
		QList<QString> &_list;
		list(QList<QString> l) :_list(l) {}
	public:
		static void index_get(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info) {
			list& klass = v8pp::from_v8<list&>(info.GetIsolate(), info.This());
			if (index < klass._list.length()) {
				info.GetReturnValue().Set(QSTR2V8(klass._list.at(index)));
			} else {
				info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "array out of range", v8::Exception::RangeError));
			}
		}
		static void index_set(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info) {
			list& klass = v8pp::from_v8<list&>(info.GetIsolate(), info.This());
			if (index < klass._list.length()) {
				klass._list[index] = V82QSTR(value);
				info.GetReturnValue().Set(value);
			} else {
				info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "array out of range", v8::Exception::RangeError));
			}
		}
		static void deleter(uint32_t index,
			const v8::PropertyCallbackInfo<v8::Boolean>& info) {
			list& klass = v8pp::from_v8<list&>(info.GetIsolate(), info.This());
			if (index < klass._list.length()) {
				klass._list.removeAt(index);
				info.GetReturnValue().Set(true);
			} else {
				info.GetReturnValue().Set(false);
			}
		}
		static void enums(const v8::PropertyCallbackInfo<v8::Array>& info) {
			list& klass = v8pp::from_v8<list&>(info.GetIsolate(), info.This());
			auto ary = v8::Array::New(v8::Isolate::GetCurrent(), klass._list.count());
			for (int i = 0; i < klass._list.count(); i++) {
				ary->Set(i, QSTR2V8(klass._list[i]));
			}
			info.GetReturnValue().Set(ary);
		}
		void push(const char* str) {
			_list.append(str);
		}
		void pop() {
			_list.pop_back();
		}
		int const GetLength() {
			return _list.length();
		}
	public:
		static v8::Handle<v8::Value> New(QList<QString> &l) {
			auto n = new list(l);
			return v8pp::class_<list>::import_external(v8::Isolate::GetCurrent(), n);
		}
	};
	class const_list {
	private:
		const QList<QString> &_list;
		const_list(const QList<QString> l) :_list(l) {}

	public:
		static void index_get(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info) {
			const_list& klass = v8pp::from_v8<const_list&>(info.GetIsolate(), info.This());
			if (index < klass._list.length()) {
				info.GetReturnValue().Set(QSTR2V8(klass._list.at(index)));
			} else {
				info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "array out of range", v8::Exception::RangeError));
			}
		}
		static void enums(const v8::PropertyCallbackInfo<v8::Array>& info) {
			const_list& klass = v8pp::from_v8<const_list&>(info.GetIsolate(), info.This());
			auto ary = v8::Array::New(v8::Isolate::GetCurrent(), klass._list.count());
			for (int i = 0; i < klass._list.count(); i++) {
				ary->Set(i, QSTR2V8(klass._list[i]));
			}
			info.GetReturnValue().Set(ary);
		}
		int const GetLength() {
			return _list.length();
		}
		static v8::Handle<v8::Value> New(const QList<QString> &l) {
			auto n = new const_list(l);
			return v8pp::class_<const_list>::import_external(v8::Isolate::GetCurrent(), n);
		}

	};
	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::module m(iso);

		v8pp::class_<list> clist(iso);
		clist.class_function_template()->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			list::index_get,
			list::index_set,
			(v8::IndexedPropertyQueryCallback)0,
			list::deleter,
			list::enums
		));
		clist.set("push", &list::push);
		clist.set("pop", &list::pop);
		clist.set("length", v8pp::property(&list::GetLength));
		clist.class_function_template()->SetClassName(v8pp::to_v8(iso, "ConstList"));

		v8pp::class_<const_list> cconst_list(iso);
		cconst_list.class_function_template()->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			const_list::index_get,
			0,
			(v8::IndexedPropertyQueryCallback)0,
			0,
			const_list::enums
		));
		cconst_list.set("length", v8pp::property(&const_list::GetLength));
		cconst_list.class_function_template()->SetClassName(v8pp::to_v8(iso, "ConstList"));

		m.set("list", clist);
		m.set("constlist", cconst_list);
		return m.new_instance();
	}
}

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
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "console.log takes at least one argument", v8::Exception::RangeError);
			return;
		}
		v8::String::Utf8Value _format(args[0]->ToString());
		QString result = applyFormat(*_format, args);
		LogManager::instance().log(result);
		args.GetReturnValue().SetNull();
	}

	void err(v8::FunctionCallbackInfo<v8::Value> const& args) {
		v8::HandleScope handle_scope(args.GetIsolate());
		if (args.Length() == 0) {
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "console.log takes at least one argument", v8::Exception::RangeError);
			return;
		}
		v8::String::Utf8Value _format(args[0]->ToString());
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

namespace debug {
	void _break() {
		if (uccuConfig::instance().enableV8Debug())
			v8::Debug::DebugBreak(v8::Isolate::GetCurrent());
	}
	void init(v8::Isolate *iso) {
		v8pp::module m(iso);
		m.set("break", &_break);
		iso->GetCurrentContext()->Global()->Set(v8::String::NewFromUtf8(iso, "debug"), m.new_instance());
	}
}

namespace buffer {
	class buffer {
	public:
		QByteArray _buff;

		void SetBuff(QByteArray buff) {
			_buff = buff;
		}
	public:
		v8::Handle<v8::Value> toString() {
			auto iso = v8::Isolate::GetCurrent();
			v8::EscapableHandleScope handle_scope(iso);
			v8::TryCatch try_catch;

			auto maybe = v8::String::NewFromUtf8(iso, _buff.data(), v8::NewStringType::kNormal, _buff.length());
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
			.set("toString", &buffer::toString)
			.set("length", v8pp::property(&buffer::GetLength))
			.set("clone", &buffer::clone)
			.set("reserve", &buffer::reserve)
			.set("resize", &buffer::resize)
			.set("fromString", &buffer::fromString);

		cbuffer.class_function_template()->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			[](uint32_t index, v8::PropertyCallbackInfo<v8::Value> const& info) {
			auto klass = v8pp::from_v8<buffer&>(info.GetIsolate(), info.This());
			if (index >= (unsigned int)klass._buff.length()) {
				info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer out of range", v8::Exception::RangeError));
			} else
				info.GetReturnValue().Set(v8pp::to_v8<int>(info.GetIsolate(), (int)klass._buff.at(index)));
		},
			[](uint32_t index,
				v8::Local<v8::Value> value,
				const v8::PropertyCallbackInfo<v8::Value>& info) {
			auto& klass = v8pp::from_v8<buffer&>(info.GetIsolate(), info.This());
			if (value.IsEmpty()) {
				info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer accept only bytes", v8::Exception::TypeError));
				return;
			}
			if (!(value->IsInt32()) && !(value->IsUint32()) && !(value->IsString())) {
				info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer accept only bytes", v8::Exception::TypeError));
				return;
			}
			if (((value->IsInt32()) || (value->IsUint32())) && !((value->Int32Value()) <= 255 && (value->Int32Value()) >= 0)) {
				info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer accept only bytes", v8::Exception::TypeError));
				return;
			}
			if ((value->IsString()) && ((value.As<v8::String>()->ContainsOnlyOneByte() != 1))) {
				info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer accept only bytes", v8::Exception::RangeError));
				return;
			}
			if (index >= (unsigned int)klass._buff.length()) {
				info.GetReturnValue().Set(v8pp::throw_ex(info.GetIsolate(), "Buffer out of range", v8::Exception::RangeError));
			}
			else {
				if (value->IsString()) {
					klass._buff[index] = (*v8::String::Utf8Value(value.As<v8::String>()))[0];
				}
				else {
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

		Stats(const char *path, bool lstat) :path(path), lstat(lstat) {
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
		return args.GetReturnValue().Set(v8pp::throw_ex(isolate, "no access"));
	}

	v8::Handle<v8::Value> readdirSync(const char * dir) {
		auto isolate = v8::Isolate::GetCurrent();
		v8::EscapableHandleScope handle_scope(isolate);
		QDir d(dir);
		auto fn = d.entryList({ "*" }, QDir::NoDotAndDotDot | QDir::AllEntries);
		
		v8::Handle<v8::Array> ary = v8::Array::New(isolate, fn.length());
		int cnt = 0;
		for (auto &s : fn) {
			ary->Set(cnt++, Helper::QSTR2V8(s));
		}

		return handle_scope.Escape(ary);
	}

	v8::Handle<v8::Value> readFileSync(const char * file) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(file);
		if (f.exists() && f.open(QIODevice::ReadOnly)) {
			auto d = f.readAll();
			f.close();
			return v8::String::NewFromUtf8(isolate, d.data(), v8::NewStringType::kNormal, d.length()).ToLocalChecked();
		}
		return v8::Undefined(isolate);
	}

	v8::Handle<v8::Value> readFileBufferSync(const char * file) {
		v8::Isolate* isolate = v8::Isolate::GetCurrent();
		QFile f(file);
		if (f.exists() && f.open(QIODevice::ReadOnly)) {
			auto d = f.readAll();
			f.close();
			return buffer::buffer::New(d);
		}
		return v8::Undefined(isolate);
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
		return v8::Undefined(isolate);
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

	void mkdirSync(const char *path) {
		QDir p(path);
		if (!p.exists()) {
			p.mkdir("./");
		}
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

	bool existsSync(const char *path) {
		return internalModuleStat(path) != -2;
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
		m.set("readdirSync", &readdirSync);
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
		m.set("existsSync", &existsSync);
		m.set("unlinkSync", &unlinkSync);
		m.set("mkdirSync", &mkdirSync);

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
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlibrary.h>
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

	bool dlopen(const char *fname, v8::Handle<v8::Value> _module) {
		QLibrary lib(fname);
		if (lib.load()) {
			if (auto init = lib.resolve("init")) {
				typedef bool(*pjsmodule_init)(v8::Handle<v8::Value>);
				return (pjsmodule_init(init))(_module);
			}
			return false;
		}
		return false;
	}

	void init(v8::Isolate *iso) {
		v8pp::module m(iso);

		// platform
#ifdef Q_OS_WIN
		m.set_const("platform", "win32");
#else
#ifdef Q_OS_MACX
		m.set_const("platform", "darwin");
#elsif Q_OS_LINUX
		m.set_const("platform", "linux");
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

		//auto _path = GetExecPathUTF8();
		auto _path = QCoreApplication::applicationDirPath();
		m.set_const("execPath", _path.toUtf8().constData());
		//delete[] _path;
		m.set_const("pid", (unsigned int)GetPid());
		m.set_const("version", "1.0.0");


		m.set("exit", &exit);
		m.set("cwd", &cwd);
		m.set("dlopen", &dlopen);

		auto i = m.new_instance();
		i->Set(v8::String::NewFromUtf8(iso, "mainModule"), v8::String::NewFromUtf8(iso, ""));

		auto ary = v8::Array::New(iso, QCoreApplication::arguments().length());
		auto arg = QCoreApplication::arguments();
		for (int i = 0; i < arg.length(); i++)
			ary->Set(i, Helper::QSTR2V8(arg[i]));
		v8pp::set_option(iso, i, "argv", ary);

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
			return v8pp::throw_ex(isolate, "options must be an object", v8::Exception::TypeError);
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
	const char * _wrapper[] = { "(function (exports, require, module, __filename, __dirname) {\n", "\n});" };
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
		return handle_scope.Escape(Helper::QSTR2V8(r));
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

				auto filename = Helper::QSTR2V8(f.fileName());
				auto dirname = Helper::QSTR2V8(f.absoluteDir().absolutePath());

				// v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global::global(isolate));
				v8::Local<v8::String> source = Helper::QSTR2V8(s);

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
				addmodule(name, exports);
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

namespace _ModAPI {
	using ::buffer::buffer;
	v8::Handle<v8::Value> get(const char *file) {
		auto iso = v8::Isolate::GetCurrent();
		QByteArray content = ResourceManager::instance().GetFileContent(":/" + QString(file));
		return buffer::New(content);
	}
	bool update(const char *file, buffer* context) {
		//v8::String::Utf8Value d(context);
		if (context->_buff == nullptr) {
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.", v8::Exception::TypeError);
			return false;
		}
		return ResourceManager::instance().UpdateFileContent(":/" + QString(file), context->_buff);
	}
	bool add(const char *file, buffer* context) {
		//v8::String::Utf8Value d(context);
		if (context->_buff == nullptr) {
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.", v8::Exception::TypeError);
			return false;
		}
		return ResourceManager::instance().AddFile(":/" + QString(file), context->_buff);
	}
	void each(const char * path, v8::Handle<v8::Function> callback) {
		ResourceManager::instance().Each(":/" + QString(path), [callback](QString path, bool isDir) {
			auto iso = v8::Isolate::GetCurrent();
			if (path.startsWith(":/")) {
				auto _path = path.right(path.length() - 2);
				if (_path.length() != 0)
					v8pp::call_v8(iso, callback, iso->GetCurrentContext()->Global(), (const char*)_path.toUtf8().data(), !!isDir);
			}
		});
	}
	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::module m(iso);
		m.set_const("api", "0.0.1");
		m.set_const("rmmv", QCoreApplication::applicationVersion().toUtf8().constData());
		m.set_const("build", "placeholder");

		m.set("get", &get);
		m.set("update", &update);
		m.set("add", &add);
		m.set("each", &each);

		return m.new_instance();
	}
};

namespace ModAPI {
	using namespace Qml;
	using ::buffer::buffer;

	v8::Handle<v8::Value> get(const char *file) {
		auto iso = v8::Isolate::GetCurrent();
		QByteArray content = ResourceManager::instance().GetFileContent(":/qml/" + QString(file));
		return buffer::New(content);
	}
	bool update_buffer(QString file, buffer* context) {
		//v8::String::Utf8Value d(context);
		if (context == nullptr || context->_buff == nullptr) {
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.", v8::Exception::TypeError);
			return false;
		}
		return ResourceManager::instance().UpdateFileContent(":/qml/" + file, context->_buff);
	}
	bool add_buffer(QString file, buffer* context) {
		//v8::String::Utf8Value d(context);
		if (context == nullptr || context->_buff == nullptr) {
			v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.", v8::Exception::TypeError);
			return false;
		}
		return ResourceManager::instance().AddFile(":/qml/" + file, context->_buff);
	}
	bool update_string(QString file, v8::Handle<v8::String> context) {
		v8::String::Utf8Value d(context);
		return ResourceManager::instance().UpdateFileContent(":/qml/" + file, *d);
	}
	bool update_qstring(QString file, QString context) {
		return ResourceManager::instance().UpdateFileContent(":/qml/" + file, context.toUtf8());
	}
	bool add_string(QString file, v8::Handle<v8::String> context) {
		v8::String::Utf8Value d(context);
		return ResourceManager::instance().AddFile(":/qml/" + file, *d);
	}
	bool add(const char *file, v8::Handle<v8::Value> v) {
		auto isolate = v8::Isolate::GetCurrent();
		auto x = v8pp::from_v8<buffer*>(isolate, v);
		if (x != nullptr) {
			return add_buffer(file, x);
		}
		if (v->IsString()) {
			return add_string(file, v.As<v8::String>());
		}
		v8pp::throw_ex(isolate, "ModAPI::add need string or buffer", v8::Exception::TypeError);
		return false;
	}
	bool update(const char *file, v8::Handle<v8::Value> v) {
		auto isolate = v8::Isolate::GetCurrent();
		auto x = v8pp::from_v8<buffer*>(isolate, v);
		if (x != nullptr) {
			return update_buffer(file, x);
		}
		if (v->IsString()) {
			return update_string(file, v.As<v8::String>());
		}
		v8pp::throw_ex(isolate, "ModAPI::update need string or buffer", v8::Exception::TypeError);
		return false;
	}
	void each(const char * path, v8::Handle<v8::Function> callback) {
		ResourceManager::instance().Each(":/qml/" + QString(path), [callback](QString path, bool isDir) {
			auto iso = v8::Isolate::GetCurrent();
			if (path.startsWith(":/qml/")) {
				auto _path = path.right(path.length() - 6);
				if (_path.length() != 0)
					v8pp::call_v8(iso, callback, iso->GetCurrentContext()->Global(), (const char*)_path.toUtf8().data(), !!isDir);
			}
		});
	}

	class qmlref;

	class qmlref {
	private:
		QSharedPointer<QmlNode> m_pNode;
		QString m_sField;

		bool m_bNull;
	public:
		qmlref(QSharedPointer<QmlNode> x, QString field) :m_pNode(x), m_sField(field) {} // no cons from js
		qmlref() { m_bNull = true; }
		qmlref(const qmlref& r) :m_pNode(r.m_pNode), m_sField(r.m_sField), m_bNull(r.m_bNull) { ; }
		static v8::Handle<v8::Value> New(QSharedPointer<QmlNode> x, QString field) {
			auto n = new qmlref(x, field);
			return v8pp::class_<qmlref>::import_external(v8::Isolate::GetCurrent(), n);
		}
	public: // property
		v8::Handle<v8::String> GetName();

		void remove();

		void val(v8::FunctionCallbackInfo<v8::Value> const&info);

		v8::Handle<v8::Value> info();

		void on(v8::FunctionCallbackInfo<v8::Value> const&info);

		void _default(v8::FunctionCallbackInfo<v8::Value> const&info);

		void readonly(v8::FunctionCallbackInfo<v8::Value> const&info);
		void ret(v8::FunctionCallbackInfo<v8::Value> const&info);

		bool isNull();

		bool isNameExists();
		bool isValueExists();
		v8::Handle<v8::Value> end();

	};


	using namespace Helper;
	class qmlnode {
	private:
		QSharedPointer<QmlNode> m_pNode;
	private:
		void create(QString typeId) { m_pNode = QSharedPointer<Qml::QmlNode>(new QmlNode(typeId)); };
		void init_v8obj() {
			/// auto iso = v8::Isolate::GetCurrent();
			/// = v8pp::persistent<v8::Value>(iso, v8stringlist::list::New(m_pNode->GetVars()));
			/// = v8pp::persistent<v8::Value>(iso, v8stringlist::const_list::New(m_pNode->GetNames()));
		}
	public:
		explicit qmlnode(const char *typeId) :m_pNode() {
			create(typeId);
			init_v8obj();
		}
		qmlnode(QSharedPointer<QmlNode> x) :m_pNode(x) {
			init_v8obj();
		};
	public:
		static v8::Handle<v8::Value> New(QSharedPointer<QmlNode> x) {
			qmlnode *n = new qmlnode(x);
			return v8pp::class_<qmlnode>::import_external(v8::Isolate::GetCurrent(), n);
		}

	public: // property
		v8::Handle<v8::Value> GetParent() {
			if (m_pNode->GetParent().isNull()) {
				return v8pp::throw_ex(v8::Isolate::GetCurrent(), "no parent", v8::Exception::ReferenceError);
			}
			qmlnode *n = new qmlnode(m_pNode->GetParent());
			return v8pp::class_<qmlnode>::import_external(v8::Isolate::GetCurrent(), n);
		}

		v8::Handle<v8::String> GetName() {
			if (auto x = m_pNode->GetParent()) {
				Qml::QmlNode::Property p = x.data()->GetPropertyByObject(x);
				if (p.eSymbolType != Qml::QmlNode::SymbolType::NoSymbol && p.eSymbolType != Qml::QmlNode::SymbolType::NoNameObject)
					return Helper::QSTR2V8(x.data()->GetNameByObject(m_pNode));
			}
			return v8::String::Empty(v8::Isolate::GetCurrent());
		}

		v8::Handle<v8::String> GetType() {
			return Helper::QSTR2V8(m_pNode->GetTypeId());
		}
		void SetType(const char* t) {
			m_pNode->SetTypeId(t);
		}

	private:
		//v8pp::persistent<v8::Value> vars;
		//v8pp::persistent<v8::Value> names;
		v8::Handle<v8::Array> wrap(const QList<QSharedPointer<QmlNode>> &l) {
			auto ary = v8::Array::New(v8::Isolate::GetCurrent(), l.length());
			for (int i = 0; i < l.length(); i++) {
				ary->Set(i, qmlnode::New(l.at(i)));
			}
			return ary;
		}
		v8::Handle<v8::Array> wrap(const QList<QWeakPointer<Qml::QmlNode>> &l) {
			auto ary = v8::Array::New(v8::Isolate::GetCurrent(), l.length());
			for (int i = 0; i < l.length(); i++) {
				ary->Set(i, qmlnode::New(l.at(i)));
			}
			return ary;
		}
		v8::Handle<v8::Array> wrap(const QList<v8::Handle<v8::Value>> &l) {
			auto ary = v8::Array::New(v8::Isolate::GetCurrent(), l.length());
			for (int i = 0; i < l.length(); i++) {
				ary->Set(i, l.at(i));
			}
			return ary;
		}
		v8::Handle<v8::Array> wrap(const QList<QString> &l) {
			auto ary = v8::Array::New(v8::Isolate::GetCurrent(), l.length());
			for (int i = 0; i < l.length(); i++) {
				ary->Set(i, QSTR2V8(l.at(i)));
			}
			return ary;
		}
	public:
		v8::Handle<v8::Value> GetVars() {
			return wrap(m_pNode->GetVars());//vars;
		}
		v8::Handle<v8::Array> GetObjs() {
			return wrap(m_pNode->GetUnnamedObjects());
		}
		v8::Handle<v8::Array> GetAll() {
			return wrap(m_pNode->GetObjects());
		}
		v8::Handle<v8::Value> GetNames() {
			return wrap(m_pNode->GetNames()); //v8stringlist::const_list::New(m_pNode->GetNames());
		}
	private:
	public: // functions

		bool exists(const char * name) {
			return m_pNode->NameExists(name);
		}

		v8::Handle<v8::Array> _on(QString name) {
			Qml::QmlNode::Value v = m_pNode->GetValueByName(name);
			if (v.type == Qml::QmlNode::ValueType::BindingObject) {
				return wrap(m_pNode->GetOnObjects(v.o));
			}
			return v8::Array::New(v8::Isolate::GetCurrent());
		}
		v8::Handle<v8::Value> _on(QString name, qmlnode* q) {
			if (q) {
				Qml::QmlNode::Value v = m_pNode->GetValueByName(name);
				if (v.type == Qml::QmlNode::ValueType::BindingObject) {
					m_pNode->AddOnObject(v.o, q->m_pNode);
				}
				return v8pp::to_v8<qmlnode>(v8::Isolate::GetCurrent(), *q);
			}
			else return v8::Null(v8::Isolate::GetCurrent());
		}

		void on(v8::FunctionCallbackInfo<v8::Value> const&info) {
			v8::EscapableHandleScope handle_scope(info.GetIsolate());
			if (info.Length() == 1) {
				info.GetReturnValue().Set(handle_scope.Escape(_on(V82QSTR(info[0]))));
				return;
			}
			else if (info.Length() == 2) {
				info.GetReturnValue().Set(handle_scope.Escape(_on(V82QSTR(info[0]), v8pp::from_v8<qmlnode*>(info.GetIsolate(), info[1]))));
				return;
			}
			info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(info.GetIsolate(), "wrong number of argument(s)", v8::Exception::RangeError)));
		}

		bool binded(const char *name) {
			return m_pNode->ValueExists(name);
		}

		v8::Handle<v8::Value> get(const char* name) {
			Qml::QmlNode::Value v = m_pNode->GetValueByName(name);
			if (v.type == Qml::QmlNode::ValueType::BindingObject) {
				return qmlnode::New(v.o);
			}
			if (v.type == Qml::QmlNode::ValueType::Array) {
				return wrap(v.l);
			}
			if (v.type == Qml::QmlNode::ValueType::RawCode) {
				return QSTR2V8(v.s);
			}
			return v8::Null(v8::Isolate::GetCurrent());
		}

		v8::Handle<v8::Value> ref(const char* name) {
			return qmlref::New(m_pNode, name);
		}
		v8::Handle<v8::Value> qref(QString name) {
			return qmlref::New(m_pNode, name);
		}

		v8::Handle<v8::Value> end() {
			return GetParent();
		}

		v8::Handle<v8::Value> clr(const char *str) {
			Qml::QmlNode::Property p = m_pNode->GetPropertyByName(str);
			remove(str);
			p.eValueType = Qml::QmlNode::ValueType::NoValue;
			if (p.Assert()) {
				Qml::QmlNode::Value v; v.type = Qml::QmlNode::ValueType::NoValue;
				m_pNode->AddNameValueProperty(str, v, p);
			}
			return v8pp::to_v8<qmlnode>(v8::Isolate::GetCurrent(), *this);
		}

		v8::Handle<v8::Value> set(const char *_str, v8::Handle<v8::Value> val) {
			QString str(_str);
			v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());
			if (val->IsArray()) {
				QList<QSharedPointer<Qml::QmlNode>> l1;
				auto ary = val.As<v8::Array>();
				uint32_t n = ary->Length();
				for (uint32_t i = 0; i < n; i++) {
					qmlnode* o = v8pp::from_v8<qmlnode*>(v8::Isolate::GetCurrent(), ary->Get(i));
					if (o) {
						l1.append(o->m_pNode);
					}
				}
				Qml::QmlNode::Property p = m_pNode->GetPropertyByName(str);
				p.o.bHasOnToken = false;
				if (p.eSymbolType == Qml::QmlNode::SymbolType::NoSymbol)
					p.eSymbolType = Qml::QmlNode::SymbolType::Object;
				p.eValueType = Qml::QmlNode::ValueType::Array;
				if (p.Assert()) {
					Qml::QmlNode::Value v;
					v.type = Qml::QmlNode::ValueType::Array;
					v.l = l1;
					m_pNode->AddNameValueProperty(str, v, p);
				}
				return handle_scope.Escape(ref(_str));
			}
			else {
				if (val->IsString()) {
					
					Qml::QmlNode::Property p = m_pNode->GetPropertyByName(str);
					p.o.bHasOnToken = false;
					if (p.eSymbolType == Qml::QmlNode::SymbolType::NoSymbol)
						p.eSymbolType = Qml::QmlNode::SymbolType::Object;
					p.eValueType = Qml::QmlNode::ValueType::RawCode;
					if (p.Assert()) {
						Qml::QmlNode::Value v;
						v.type = Qml::QmlNode::ValueType::RawCode;
						auto str1 = val.As<v8::String>();
						v.s = V82QSTR(str1);
						m_pNode->AddNameValueProperty(str, v, p);
					}
					return handle_scope.Escape(ref(_str));
				}
				else {
					qmlnode* n = v8pp::from_v8<qmlnode*>(v8::Isolate::GetCurrent(), val);
					if (n) {
						Qml::QmlNode::Property p = m_pNode->GetPropertyByName(str);
						p.o.bHasOnToken = false;
						if (p.eSymbolType == Qml::QmlNode::SymbolType::NoSymbol)
							p.eSymbolType = Qml::QmlNode::SymbolType::Object;
						p.eValueType = Qml::QmlNode::ValueType::BindingObject;
						if (p.Assert()) {
							Qml::QmlNode::Value v;
							v.type = Qml::QmlNode::ValueType::BindingObject;
							v.o = n->m_pNode;
							m_pNode->AddNameValueProperty(str, v, p);
						}
						return handle_scope.Escape(ref(_str));
					}
				}
			}
			return v8::Null(v8::Isolate::GetCurrent());
		}


		v8::Handle<v8::Value> _def(QString name, QString type, QString val) {
			Qml::QmlNode::Property p;
			p.clear();
			if (type.toLower() == "object")
				p.eSymbolType = Qml::QmlNode::SymbolType::Object;
			if (type.toLower() == "function")
				p.eSymbolType = Qml::QmlNode::SymbolType::Function;
			if (type.toLower() == "property")
				p.eSymbolType = Qml::QmlNode::SymbolType::Prop;
			if (type.toLower() == "signal")
				p.eSymbolType = Qml::QmlNode::SymbolType::Signal;

			p.eValueType = Qml::QmlNode::ValueType::RawCode;
			if (p.Assert()) {
				Qml::QmlNode::Value v; v.type = Qml::QmlNode::ValueType::RawCode;
				v.s = val;
				m_pNode->AddNameValueProperty(name, v, p);
				return qref(name);
			}
			return v8::Null(v8::Isolate::GetCurrent());
		}
		v8::Handle<v8::Value> _def(QString name, QString type) {
			Qml::QmlNode::Property p;
			p.clear();
			if (type.toLower() == "object")
				p.eSymbolType = Qml::QmlNode::SymbolType::Object;
			if (type.toLower() == "function")
				p.eSymbolType = Qml::QmlNode::SymbolType::Function;
			if (type.toLower() == "property")
				p.eSymbolType = Qml::QmlNode::SymbolType::Prop;
			if (type.toLower() == "signal")
				p.eSymbolType = Qml::QmlNode::SymbolType::Signal;
			p.eValueType = Qml::QmlNode::ValueType::NoValue;

			if (p.Assert()) {
				Qml::QmlNode::Value v; v.type = Qml::QmlNode::ValueType::NoValue;
				m_pNode->AddNameValueProperty(name, v, p);
				return qref(name);
			}
			return v8::Null(v8::Isolate::GetCurrent());
		}
		v8::Handle<v8::Value> __default(QString name) {
			auto p = m_pNode->GetPropertyByName(name);
			if (p.eSymbolType == Qml::QmlNode::SymbolType::Prop) {
				return v8::Boolean::New(v8::Isolate::GetCurrent(), p.p.bDefault);
			}
			return v8::Boolean::New(v8::Isolate::GetCurrent(), false);
		}
		v8::Handle<v8::Value> __default(QString name, v8::Handle<v8::Value> val) {
			auto p = m_pNode->GetPropertyByName(name);
			if (p.eSymbolType == Qml::QmlNode::SymbolType::Prop) {
				p.p.bDefault = val->BooleanValue();
				m_pNode->SetPropertyByName(name, p);
			}
			return v8pp::to_v8<qmlnode*>(v8::Isolate::GetCurrent(), this);
		}
		v8::Handle<v8::Value> ___readonly(QString name) {
			auto p = m_pNode->GetPropertyByName(name);
			if (p.eSymbolType == Qml::QmlNode::SymbolType::Prop) {
				return v8::Boolean::New(v8::Isolate::GetCurrent(), p.p.bReadOnly);
			}
			return v8::Boolean::New(v8::Isolate::GetCurrent(), false);
		}
		v8::Handle<v8::Value> ___readonly(QString name, v8::Handle<v8::Value> val) {
			auto p = m_pNode->GetPropertyByName(name);
			if (p.eSymbolType == Qml::QmlNode::SymbolType::Prop) {
				p.p.bReadOnly = val->BooleanValue();
				m_pNode->SetPropertyByName(name, p);
			}
			return v8pp::to_v8<qmlnode*>(v8::Isolate::GetCurrent(), this);
		}

		void def(v8::FunctionCallbackInfo<v8::Value> const&info) {
			v8::EscapableHandleScope handle_scope(info.GetIsolate());
			if (info.Length() == 2) {
				info.GetReturnValue().Set(handle_scope.Escape(_def(V82QSTR(info[0]), V82QSTR(info[1]))));
				return;
			}
			else if (info.Length() == 3) {
				info.GetReturnValue().Set(handle_scope.Escape(_def(V82QSTR(info[0]), V82QSTR(info[1]), V82QSTR(info[2]))));
				return;
			}
			info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(info.GetIsolate(), "wrong number of argument(s)", v8::Exception::RangeError)));
		}

		void _default(v8::FunctionCallbackInfo<v8::Value> const&info) {
			v8::EscapableHandleScope handle_scope(info.GetIsolate());
			if (info.Length() == 1) {
				info.GetReturnValue().Set(handle_scope.Escape(__default(V82QSTR(info[0]))));
				return;
			}
			else if (info.Length() == 2) {
				info.GetReturnValue().Set(handle_scope.Escape(__default(V82QSTR(info[0]), info[1])));
				return;
			}
			info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(info.GetIsolate(), "wrong number of argument(s)", v8::Exception::RangeError)));
		}

		void readonly(v8::FunctionCallbackInfo<v8::Value> const&info) {
			v8::EscapableHandleScope handle_scope(info.GetIsolate());
			if (info.Length() == 1) {
				info.GetReturnValue().Set(handle_scope.Escape(___readonly(V82QSTR(info[0]))));
				return;
			}
			else if (info.Length() == 2) {
				info.GetReturnValue().Set(handle_scope.Escape(___readonly(V82QSTR(info[0]), info[1])));
				return;
			}
			info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(info.GetIsolate(), "wrong number of argument(s)", v8::Exception::RangeError)));
		}

		v8::Handle<v8::Object> info(const char *name) {
			// ?
			// v8pp::from_v8
			v8::Handle<v8::Object> m = v8::Object::New(v8::Isolate::GetCurrent());

			auto p = m_pNode->GetPropertyByName(name);
			if (p.eSymbolType != Qml::QmlNode::SymbolType::NoSymbol) {
				v8pp::set_option(v8::Isolate::GetCurrent(), m, "name", name);
				if (p.eSymbolType == Qml::QmlNode::SymbolType::Prop) {
					v8pp::set_option(v8::Isolate::GetCurrent(), m, "kind", "Property");
					v8pp::set_option(v8::Isolate::GetCurrent(), m, "_default", p.p.bDefault);
					v8pp::set_option(v8::Isolate::GetCurrent(), m, "readonly", p.p.bReadOnly);
					v8pp::set_option(v8::Isolate::GetCurrent(), m, "ret", QSTR2V8(p.p.retType));
				}
#define b(x) \
				if (p.eSymbolType == Qml::QmlNode::SymbolType::x) \
					v8pp::set_option(v8::Isolate::GetCurrent(), m, "kind", #x);
				
				b(Object)
				b(Signal)
				b(Function)
#undef b
				v8pp::set_option(v8::Isolate::GetCurrent(), m, "binded", binded(name));
			}
			return m;
		}

		v8::Handle<v8::Value> _ret(QString name) {
			auto p = m_pNode->GetPropertyByName(name);
			if (p.eSymbolType == Qml::QmlNode::SymbolType::Prop) {
				return QSTR2V8(p.p.retType);
			}
			return v8::String::Empty(v8::Isolate::GetCurrent());
		}
		v8::Handle<v8::Value> _ret(QString name, QString val) {
			auto p = m_pNode->GetPropertyByName(name);
			if (p.eSymbolType == Qml::QmlNode::SymbolType::Prop) {
				p.p.retType = val;
				m_pNode->SetPropertyByName(name, p);
			}
			return v8pp::to_v8<qmlnode*>(v8::Isolate::GetCurrent(), this);
		}

		void ret(v8::FunctionCallbackInfo<v8::Value> const&info) {
			v8::EscapableHandleScope handle_scope(info.GetIsolate());
			if (info.Length() == 1) {
				if (info[0]->IsString()) {
					info.GetReturnValue().Set(handle_scope.Escape(_ret(V82QSTR(info[0]))));
					return;
				} else {
					info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(info.GetIsolate(), "string is required", v8::Exception::TypeError)));
					return;
				}
			}
			else if (info.Length() == 2) {
				if (!(info[0]->IsString() && info[1]->IsString())) {
					info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(info.GetIsolate(), "string is required", v8::Exception::TypeError)));
					return;
				}
				info.GetReturnValue().Set(handle_scope.Escape(_ret(V82QSTR(info[0]), V82QSTR(info[1]))));
				return;
			}
			info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(info.GetIsolate(), "wrong number of argument(s)", v8::Exception::RangeError)));
		}

		qmlnode* add(qmlnode* node) {
			m_pNode->AddUnnamedObject(node->m_pNode);
			return this;
		}

		qmlnode* addBefore(qmlnode* node) {
			if (m_pNode->GetParent())
				m_pNode->GetParent().data()->AddUnnamedObjectBefore(node->m_pNode, m_pNode);
			return this;
		}

		qmlnode* makeObject(const char* name) {
			Qml::QmlNode::Property p = m_pNode->GetPropertyByName(name);
			if (p.eSymbolType == Qml::QmlNode::SymbolType::NoSymbol)
				return this;
			auto v = m_pNode->GetValueByName(name);
			p.eSymbolType = Qml::QmlNode::SymbolType::Object;
			if (p.Assert())
				m_pNode->AddNameValueProperty(name, v, p);
			return this;
		}

		qmlnode* makeProperty(const char* name, const char* ret) {
			Qml::QmlNode::Property p = m_pNode->GetPropertyByName(name);
			p.p.retType = ret;
			if (p.eSymbolType == Qml::QmlNode::SymbolType::NoSymbol)
				return this;
			auto v = m_pNode->GetValueByName(name);
			p.eSymbolType = Qml::QmlNode::SymbolType::Prop;
			if (p.Assert())
				m_pNode->AddNameValueProperty(name, v, p);
			return this;
		}

		void kill() {
			if (auto x = m_pNode->GetParent().data()) {
				x->EraseObject(m_pNode);
			}
		}

		qmlnode* remove(const char *name) {
			m_pNode->EraseByName(name);
			return this;
		}

		void getObjectById(v8::FunctionCallbackInfo<v8::Value> const&info) {
			v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());
			// const char* target, int max_deep = 0
			int max_deep = 0;
			if (info.Length() == 2) {
				if (info[1]->IsNumber()) {
					max_deep = info[1]->Int32Value();
				}
			}
			if (info.Length() == 1 || info.Length() == 2) {
				if (info[0]->IsString()) {
					QString target = V82QSTR(info[0]);
					auto tag = _find_id(m_pNode.toWeakRef(), target, max_deep);
					if (tag.isNull())
						info.GetReturnValue().SetNull();
					else
						info.GetReturnValue().Set(handle_scope.Escape(qmlnode::New(tag.toStrongRef())));
				} else {
					info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong type of arguments", v8::Exception::TypeError)));
				}
			} else {
				info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments", v8::Exception::TypeError)));
			}
		}

		void getObjectsByType(v8::FunctionCallbackInfo<v8::Value> const&info) {
			// const char* target, int max_deep = 0
			v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());
			int max_deep = 0;
			if (info.Length() == 2) {
				if (info[1]->IsNumber()) {
					max_deep = info[1]->Int32Value();
				}
			}
			if (info.Length() == 1 || info.Length() == 2) {
				if (info[0]->IsString()) {
					QString target = V82QSTR(info[0]);
					auto tag = _find_type(m_pNode.toWeakRef(), target, max_deep);
					info.GetReturnValue().Set(handle_scope.Escape(wrap(tag)));
				} else {
					info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong type of arguments", v8::Exception::RangeError)));
				}
			} else {
				info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments", v8::Exception::RangeError)));
			}
		}

		void getValueByName(v8::FunctionCallbackInfo<v8::Value> const&info) {
			// const char* target, int max_deep = 0
			v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());
			int max_deep = 0;
			if (info.Length() == 2) {
				if (info[1]->IsNumber()) {
					max_deep = info[1]->Int32Value();
				}
			}
			if (info.Length() == 1 || info.Length() == 2) {
				if (info[0]->IsString()) {
					QString target = V82QSTR(info[0]);
					auto tag = _find_name(m_pNode.toWeakRef(), target, max_deep);
					info.GetReturnValue().Set(handle_scope.Escape(wrap(tag)));
				} else {
					info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong type of arguments", v8::Exception::TypeError)));
				}
			} else {
				info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments", v8::Exception::RangeError)));
			}
		}

		void select(v8::FunctionCallbackInfo<v8::Value> const&info) {
			// const char* target, int max_deep = 0
			v8::EscapableHandleScope handle_scope(v8::Isolate::GetCurrent());
			int max_deep = 0;
			if (info.Length() == 3) {
				if (info[2]->IsNumber()) {
					max_deep = info[1]->Int32Value();
				}
			}
			if (info.Length() == 2 || info.Length() == 3) {
				if (info[0]->IsString() && info[1]->IsString()) {
					QString field = V82QSTR(info[0]);
					QString value = V82QSTR(info[1]);
					auto tag = _select(m_pNode.toWeakRef(), field, value, max_deep);
					info.GetReturnValue().Set(handle_scope.Escape(wrap(tag)));
				} else {
					info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong type of arguments", v8::Exception::TypeError)));
				}
			} else {
				info.GetReturnValue().Set(handle_scope.Escape(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments", v8::Exception::RangeError)));
			}
		}


	private:
		static QWeakPointer<Qml::QmlNode> _find_id(QWeakPointer<Qml::QmlNode> x, QString target, int max_deep = 0) {
			auto v = (x.data()->GetValueByName("id"));
			if (v.type == Qml::QmlNode::ValueType::RawCode) {
				if (v.s == target)
					return x;
			}
			if (max_deep == 1) {
				return QWeakPointer<Qml::QmlNode>();
			}
			for (auto y : x.data()->GetObjects()) {
				auto z = _find_id(y, target, max_deep == 0 ? 0 : max_deep - 1);
				if (!z.isNull())
					return z;
			}
			return QWeakPointer<Qml::QmlNode>();
		}

		static QList<QWeakPointer<Qml::QmlNode>> _find_type(QWeakPointer<Qml::QmlNode> x, QString target, int max_deep = 0) {
			QList<QWeakPointer<Qml::QmlNode>> L;
			auto v = (x.data()->GetTypeId());
			if (v == target) {
				L.append(x);
			}
			if (max_deep == 1)
				return L;
			for (auto y : x.data()->GetObjects()) {
				L.append(_find_type(y, target, max_deep == 0 ? 0 : max_deep - 1));
			}
			return L;
		}

		static QList<v8::Handle<v8::Value> > _find_name(QWeakPointer<Qml::QmlNode> x, QString field, int max_deep) {
			QList< v8::Handle<v8::Value> > L;

			if (x.data()->NameExists(field)) {
				L.append(qmlref::New(x, field));
			}
			if (max_deep == 1)
				return L;
			for (auto y : x.data()->GetObjects()) {
				L.append(_find_name(y, field, max_deep == 0 ? 0 : max_deep - 1));
			}
			return L;
		}

		static QList<v8::Handle<v8::Value>> _select(QWeakPointer<Qml::QmlNode> x, QString field, QString val, int max_deep) {
			QList<v8::Handle<v8::Value>> L;
			if (x.data()->NameExists(field)) {
				auto v = x.data()->GetValueByName(field);
				if (v.type == Qml::QmlNode::RawCode && v.s == val)
					L.append(qmlnode::New(x));
			}
			if (max_deep == 1)
				return L;
			for (auto y : x.data()->GetObjects()) {
				L.append(_select(y, field, val, max_deep == 0 ? 0 : max_deep - 1));
			}
			return L;
		}
	};

	v8::Handle<v8::String> qmlref::GetName() {
		return Helper::QSTR2V8(m_sField);
	}

	void qmlref::remove() {
		m_pNode->EraseByName(m_sField);
	}

	void qmlref::val(v8::FunctionCallbackInfo<v8::Value> const&info) {
		if (info.Length() == 0) {
			info.GetReturnValue().Set(qmlnode(m_pNode).get(m_sField.toUtf8().data()));
		} else if (info.Length() == 1) {
			qmlnode(m_pNode).set(m_sField.toUtf8().data(), info[0]);
			info.GetReturnValue().SetNull();
		} else {
			info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments.", v8::Exception::RangeError));
		}
		// (QVariant)
	}

	v8::Handle<v8::Value> qmlref::info() {
		return qmlnode(m_pNode).info(m_sField.toUtf8().data());
	}

	void qmlref::on(v8::FunctionCallbackInfo<v8::Value> const&info) {
		if (info.Length() == 0) {
			info.GetReturnValue().Set(qmlnode(m_pNode)._on(m_sField));
		} else if (info.Length() == 1) {
			//_on(info[0]);
			qmlnode(m_pNode)._on(m_sField, v8pp::from_v8<qmlnode*>(v8::Isolate::GetCurrent(), info[0]));
			info.GetReturnValue().Set(v8pp::to_v8<qmlref*>(v8::Isolate::GetCurrent(), this));
		} else info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments", v8::Exception::RangeError));
		// (JsQmlNode*)
	}


	void qmlref::_default(v8::FunctionCallbackInfo<v8::Value> const&info) {
		// (bool)
		if (info.Length() == 0) {
			info.GetReturnValue().Set(qmlnode(m_pNode).__default(m_sField));
		} else if (info.Length() == 1) {
			//_on(info[0]);
			qmlnode(m_pNode).__default(m_sField, info[0]);
			info.GetReturnValue().Set(v8pp::to_v8<qmlref*>(v8::Isolate::GetCurrent(), this));
		} else info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments", v8::Exception::RangeError));
	}

	void qmlref::readonly(v8::FunctionCallbackInfo<v8::Value> const&info) {
		// (bool)
		if (info.Length() == 0) {
			info.GetReturnValue().Set(qmlnode(m_pNode).___readonly(m_sField));
		} else if (info.Length() == 1) {
			//_on(info[0]);
			qmlnode(m_pNode).___readonly(m_sField, info[0]);
			info.GetReturnValue().Set(v8pp::to_v8<qmlref*>(v8::Isolate::GetCurrent(), this));
		} else info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments", v8::Exception::RangeError));
	}

	void qmlref::ret(v8::FunctionCallbackInfo<v8::Value> const&info) {
		// (const char*)
		if (info.Length() == 0) {
			info.GetReturnValue().Set(qmlnode(m_pNode)._ret(m_sField));
		} else if (info.Length() == 1) {
			//_on(info[0]);
			if (info[0]->IsString()) {
				qmlnode(m_pNode)._ret(m_sField, V82QSTR(info[0]));
				info.GetReturnValue().Set(v8pp::to_v8<qmlref*>(v8::Isolate::GetCurrent(), this));
			} else info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong type of arguments", v8::Exception::TypeError));
		} else info.GetReturnValue().Set(v8pp::throw_ex(v8::Isolate::GetCurrent(), "wrong number of arguments", v8::Exception::RangeError));
	}

	bool qmlref::isNull() {
		return m_bNull;
	}

	bool qmlref::isNameExists() {
		return m_pNode ? m_pNode->NameExists(m_sField) : 0;
	}

	bool qmlref::isValueExists() {
		return m_pNode ? m_pNode->ValueExists(m_sField) : 0;
	}

	v8::Handle<v8::Value> qmlref::end() {
		return qmlnode::New(m_pNode);
	}



	class qmldocument {
	private:
		QSharedPointer<Document> m_pDoc;

		v8pp::persistent<v8::Value> roots;
		v8pp::persistent<v8::Array> imports;
		v8pp::persistent<v8::Array> parmas;

		QString file;
	public:
		explicit qmldocument(QByteArray buff, const char* file) {
			if (file != nullptr) {
				this->file = file;
			}
			auto iso = v8::Isolate::GetCurrent();
			QmlProcessor process(buff);
			auto doc_ = process.GenerateDocument();
			if (doc_ == nullptr) {
				v8pp::throw_ex(v8::Isolate::GetCurrent(), "failed parse qml");
				return;
			}
			m_pDoc = QSharedPointer<Qml::Document>(doc_);

			auto _roots = qmlnode::New(m_pDoc->root);
			
			auto _imports = v8::Array::New(v8::Isolate::GetCurrent(), m_pDoc->vImports.count());

			int i = 0;
			for (auto s : m_pDoc->vImports) {
				_imports->Set(i++, v8pp::to_v8<const char*>(iso, (const char*)s.toUtf8().data()));
			}

			auto _parmas = v8::Array::New(v8::Isolate::GetCurrent(), m_pDoc->vParmas.count());

			i = 0;
			for (auto s : m_pDoc->vParmas) {
				_parmas->Set(i++, v8pp::to_v8<const char*>(iso, (const char*)s.toUtf8().data()));
			}
			
			roots = v8pp::persistent<v8::Value>(iso, _roots);
			imports = v8pp::persistent<v8::Array>(iso, _imports);
			parmas = v8pp::persistent<v8::Array>(iso, _parmas);
		}
	public:
		static v8::Handle<v8::Value> New(buffer* buffer, const char* file = nullptr) {
			if (buffer == nullptr || buffer->_buff == nullptr) {
				return v8pp::throw_ex(v8::Isolate::GetCurrent(), "Internal Buffer is not completed.");
			}
			v8::TryCatch try_catch;
			qmldocument *n = new qmldocument(buffer->_buff, file);
			if (try_catch.HasCaught()) {
				return try_catch.ReThrow();
			}
			return v8pp::class_<qmldocument>::import_external(v8::Isolate::GetCurrent(), n);
		}
		static v8::Handle<v8::Value> New(QByteArray buffer, const char* file = nullptr) {
			v8::TryCatch try_catch;
			qmldocument *n = new qmldocument(buffer, file);
			if (try_catch.HasCaught()) {
				return try_catch.ReThrow();
			}
			return v8pp::class_<qmldocument>::import_external(v8::Isolate::GetCurrent(), n);
		}
	public:
		v8::Handle<v8::Value> get_root() {
			return roots.Get(v8::Isolate::GetCurrent());
		}

		v8::Handle<v8::Value> get_imports() {
			return imports.Get(v8::Isolate::GetCurrent());
		}
		void set_imports(v8::Handle<v8::Array> ary) {
			for (int i = 0; i < ary->Length(); i++)
				if (!ary->Get(i)->IsString()) {
					v8pp::throw_ex(v8::Isolate::GetCurrent(), "Imports allow only string", v8::Exception::TypeError);
					return;
				}
			// imports = ary;
		}

		v8::Handle<v8::Value> get_parmas() {
			return parmas.Get(v8::Isolate::GetCurrent());
		}

		void set_parmas(v8::Handle<v8::Array> ary) {
			for (int i = 0; i < ary->Length(); i++)
				if (!ary->Get(i)->IsString()) {
					v8pp::throw_ex(v8::Isolate::GetCurrent(), "allow only string", v8::Exception::TypeError);
					return;
				}
			//return parmas = ary;
		}

		static QList<QString> toQStringArray(v8::Handle<v8::Array> ary) {
			QList<QString> list;
			for (int i = 0; i < ary->Length(); i++) {
				if (!ary->Get(i)->IsString()) {
					v8pp::throw_ex(v8::Isolate::GetCurrent(), "allow only string", v8::Exception::TypeError);
					return list;
				}
				list.append(V82QSTR(ary->Get(i)->ToString()));
			}
			return list;
		}

		v8::Handle<v8::Value> getCode() {
			// return buffer
			auto iso = v8::Isolate::GetCurrent();
			v8::EscapableHandleScope handle_scope(iso);
			m_pDoc->vImports = toQStringArray(imports.Get(iso));
			m_pDoc->vParmas = toQStringArray(parmas.Get(iso));
			auto buff = m_pDoc->GenCode();
			return handle_scope.Escape(buffer::New(buff.toUtf8()));
		}

		v8::Handle<v8::Value> getCodeString() {
			// return buffer
			auto iso = v8::Isolate::GetCurrent();
			v8::EscapableHandleScope handle_scope(iso);
			m_pDoc->vImports = toQStringArray(imports.Get(iso));
			m_pDoc->vParmas = toQStringArray(parmas.Get(iso));
			auto buff = m_pDoc->GenCode();
			return handle_scope.Escape(QSTR2V8(buff));
		}

		void save(v8::FunctionCallbackInfo<v8::Value> const& args) {
			QString nfile = file;
			
			auto iso = v8::Isolate::GetCurrent();

			if (args.Length() > 1) {
				args.GetReturnValue().Set(v8pp::throw_ex(iso, "save need 0 or 1 argument", v8::Exception::RangeError));
				return;
			}

			if (args.Length() == 1) {
				if (args[0]->IsString())
					nfile = V82QSTR(args[0]);
				else {
					args.GetReturnValue().Set(v8pp::throw_ex(iso, "save accept only string", v8::Exception::TypeError));
					return;
				}
			}

			v8::EscapableHandleScope handle_scope(iso);
			if (!nfile.isEmpty()) {
				m_pDoc->vImports = toQStringArray(imports.Get(iso));
				m_pDoc->vParmas = toQStringArray(parmas.Get(iso));
				auto buff = m_pDoc->GenCode();
				args.GetReturnValue().Set(update_qstring(nfile, buff));
			} else {
				args.GetReturnValue().Set(false);
			}
		}

	};

	v8::Handle<v8::Value> node(v8::Handle<v8::Value> x) {
		if (x->IsString()) {
			QmlProcessor process(V82QSTR(x)); 
			auto node_ = process.GenerateNode();
			if (node_ == nullptr) {
				return v8pp::throw_ex(v8::Isolate::GetCurrent(), "failed parse qml");
			}
			return qmlnode::New(node_);
		} else {
			auto buf = v8pp::from_v8<buffer*>(v8::Isolate::GetCurrent() ,x);
			if (buf != nullptr) {
				QmlProcessor process(buf->_buff);
				auto node_ = process.GenerateNode();
				if (node_ == nullptr) {
					return v8pp::throw_ex(v8::Isolate::GetCurrent(), "failed parse qml");
				}
				return qmlnode::New(node_);
			}
		}
		return v8pp::throw_ex(v8::Isolate::GetCurrent(), "need string or buffer", v8::Exception::TypeError);
	}

	v8::Handle<v8::Value> qml(const char* file) {
		QByteArray content = ResourceManager::instance().GetFileContent(":/qml/" + QString(file));
		if (content.isEmpty()) 
			return v8pp::throw_ex(v8::Isolate::GetCurrent(), "bad qml file");
		return qmldocument::New(content, file);
	}

	v8::Handle<v8::Value> doc(v8::Handle<v8::Value> val) {
		if (val->IsString()) {
			return qmldocument::New(V82QSTR(val).toUtf8());
		} else {
			auto x = v8pp::from_v8<buffer*>(v8::Isolate::GetCurrent(), val);
			if (x != nullptr)
				return qmldocument::New(x);
		}
		return v8pp::throw_ex(v8::Isolate::GetCurrent(), "need string or buffer", v8::Exception::TypeError);
	}

	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::class_<qmldocument> cdoc(iso);
		v8pp::class_<qmlnode> cnode(iso);
		v8pp::class_<qmlref> cref(iso);

		cdoc//.ctor<buffer*>() //
			.set("node", v8pp::property(&qmldocument::get_root))
			.set("root", v8pp::property(&qmldocument::get_root))
			.set("imports", v8pp::property(&qmldocument::get_imports, &qmldocument::set_imports))
			.set("parmas", v8pp::property(&qmldocument::get_parmas, &qmldocument::set_parmas))
			.set("toBuffer", &qmldocument::getCode)
			.set("toString", &qmldocument::getCodeString)
			.set("save", &qmldocument::save);

		/*
		cdoc_imports.class_function_template()->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			[](uint32_t index, v8::PropertyCallbackInfo<v8::Value> const& info) {
				auto klass = v8pp::from_v8<qmlducument_imports&>(info.GetIsolate(), info.This());

			}, ));
			*/
		cnode.ctor<const char*>();
		cnode.set("vars", v8pp::property(&qmlnode::GetVars))
			.set("name", v8pp::property(&qmlnode::GetName))
			.set("type", v8pp::property(&qmlnode::GetType, &qmlnode::SetType))
			.set("obj", &qmlnode::GetObjs)
			.set("all", &qmlnode::GetAll)
			.set("names", &qmlnode::GetNames)
			.set("parent", v8pp::property(&qmlnode::GetParent))
			.set("exists", &qmlnode::exists)
			.set("on", &qmlnode::on)
			.set("binded", &qmlnode::binded)
			.set("get", &qmlnode::get)
			.set("ref", &qmlnode::ref)
			.set("end", &qmlnode::end)
			.set("clr", &qmlnode::clr)
			.set("set", &qmlnode::set)
			.set("def", &qmlnode::def)
			.set("_default", &qmlnode::_default)
			.set("readonly", &qmlnode::readonly)
			.set("info", &qmlnode::info)
			.set("ret", &qmlnode::ret)
			.set("add", &qmlnode::add)
			.set("addBefore", &qmlnode::addBefore)
			.set("makeObject", &qmlnode::makeObject)
			.set("makeProperty", &qmlnode::makeProperty)
			.set("kill", &qmlnode::kill)
			.set("remove", &qmlnode::remove)
			.set("getObjectById", &qmlnode::getObjectById)
			.set("getObjectsByType", &qmlnode::getObjectsByType)
			.set("getValueByName", &qmlnode::getValueByName)
			.set("select", &qmlnode::select);
		cref.set("name", v8pp::property(&qmlref::GetName))
			.set("remove", &qmlref::remove)
			.set("val", &qmlref::val)
			.set("info", &qmlref::info)
			.set("_default", &qmlref::_default)
			.set("readonly", &qmlref::readonly)
			.set("ret", &qmlref::ret)
			.set("isNull", &qmlref::isNull)
			.set("isNameExists", &qmlref::isNameExists)
			.set("isValueExists", &qmlref::isValueExists)
			.set("end", &qmlref::end);
		v8pp::module m(iso);
		m.set_const("api", "1.0.0");
		m.set_const("rmmv", QCoreApplication::applicationVersion().toUtf8().constData());
#ifdef BUILD_VERSION
		m.set_const("build", #BUILD_VERSION);
#endif
		m.set("QMLDocument", cdoc);
		m.set("QMLNode", cnode);
		
		m.set("get", &get);
		m.set("update", &update);
		m.set("add", &add);
		m.set("each", &each);

		m.set("node", &node);
		m.set("qml", &qml);
		m.set("doc", &doc);
		
		return m.new_instance();
	}
};

#include <QtCore/qresource.h>
#include "Injector.h"
namespace platform {
	bool registerResourceBuffer(buffer::buffer *buff, const char* root) {
		return QResource::registerResource((const uchar*)buff->_buff.constData(), root);
	}

	bool registerResourceRCC(const char* rcc, const char* root) {
		return QResource::registerResource(QString(rcc), root);
	}

	void replaceTranslator(const char* org, const char* rep) {
		Injector::instance().replaceTranslator(org, rep);
	}

	void addTranslator(const char* fname) {
		Injector::instance().addTranslator(fname);
	}
	
	v8::Handle<v8::Value> init(v8::Isolate *iso) {
		v8pp::module m(iso);
		m.set_const("qt_version", QT_VERSION_STR);
#define CChar(x) ((const char*)((x).toUtf8().data()))
		m.set_const("os_type", CChar(QSysInfo::productType()));
		m.set_const("os_version", CChar(QSysInfo::productVersion()));
		m.set_const("os", CChar(QSysInfo::prettyProductName()));
		m.set_const("kernel", CChar(QSysInfo::kernelType()));
		m.set_const("kernel_version", CChar(QSysInfo::kernelVersion()));
		m.set_const("cpu", CChar(QSysInfo::currentCpuArchitecture()));
		m.set_const("app_home", CChar(QCoreApplication::applicationDirPath()));
		m.set_const("locale", CChar(QLocale::system().name()));
		
		m.set("registerResourceBuffer", &registerResourceBuffer);
		m.set("registerResourceRCC", &registerResourceRCC);
		
		m.set("replaceTranslatorFile", &replaceTranslator);
		m.set("addTraslatorFile", &addTranslator);
		
		auto ins = m.new_instance();
		return ins;
	}
};

/*
	TODO: use funciton template to create global classes and
	Give every module own sandbox
*/
void JSCore::initAll(v8::Isolate * iso) {
	v8::TryCatch try_catch;
	//global::init(iso);
	console::init(iso);
	debug::init(iso);

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
	// native_module::addmodule("_modapi", _ModAPI::init(iso));
	// TODO: Fix v8List or use something else...
	native_module::addmodule("v8list", v8stringlist::init(iso));
	native_module::addmodule("platform", platform::init(iso));

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
	native_module::load("lodash", "./mvuccu/lib/lodash.js");
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
