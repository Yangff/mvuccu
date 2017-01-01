#include "ScriptCore.h"
#include "uccuConfig.h"
#include "LogManager.h"
#include "ModManager.h"
#include "JSCore.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

#include <libplatform/libplatform.h>
#include <v8.h>

#include <QtCore/qcoreapplication.h>

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
	virtual void* Allocate(size_t length) {
		void* data = AllocateUninitialized(length);
		return data == NULL ? data : memset(data, 0, length);
	}
	virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
	virtual void Free(void* data, size_t) { free(data); }
};


ScriptCore::ScriptCore() {

}

#include <v8pp/call_v8.hpp>
#include <v8pp/object.hpp>

void ScriptCore::RunScript() {

	// Step1. Init Js Cxt

	v8::V8::InitializeICU();
	
	v8::V8::InitializeExternalStartupData(QCoreApplication::applicationFilePath().toStdString().c_str());
	v8::V8::InitializePlatform(0);
	v8::V8::Initialize();
	// v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
	ArrayBufferAllocator array_buffer_allocator;
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = &array_buffer_allocator;
	v8::Isolate* isolate = v8::Isolate::New(create_params);
	{
		// Step2. Init C++/JS Objects
		JSCore::initAll(isolate);
		// Step3. Excute loader, pass uccu namespace and returns $loader
		v8::Handle<v8::Function> f;
		if (v8pp::get_option(isolate, JSCore::require("module").As<v8::Object>(), "runMain", f)) {

		}
	}

	// Step6. Finalized
	isolate->Dispose();
	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	
}

ScriptCore *ScriptCore::_ins;