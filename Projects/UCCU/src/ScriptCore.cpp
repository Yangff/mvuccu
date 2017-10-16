#include "ScriptCore.h"
#include "uccuConfig.h"
#include "LogManager.h"
#include "ModManager.h"
#include "JSCore.h"
#include "../../../v8-debug/DebugAgent.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

#include <libplatform/libplatform.h>
#include <v8.h>

#include <QtCore/qcoreapplication.h>

#include <v8pp/convert.hpp>

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


std::wstring GetErrorMessage(v8::Isolate*iso, v8::Handle<v8::Value> obj, v8::Handle<v8::Message> msg) {
	auto error = v8pp::convert<std::wstring>::from_v8(iso,  msg->Get());
	auto stack = msg->GetStackTrace();
	
	std::wstring errmsg = error + L"\n" ;

	if (!stack.IsEmpty()) {
		
		for (int i = 0; i < stack->GetFrameCount(); i++) {
			auto frame = stack->GetFrame(i);
			auto name = v8pp::convert<std::wstring>::from_v8(iso, frame->GetScriptName());
			auto line = std::to_wstring(frame->GetLineNumber());
			auto col = std::to_wstring(frame->GetColumn());
			auto func = v8pp::convert<std::wstring>::from_v8(iso, frame->GetFunctionName());
			if (func == L"")
				func = L"anonymous";
			errmsg += L"\t at <" + func + L">" + line + L":" + col + L" from " + name + L"\n";
		}
	} else {
		msg->GetLineNumber();
		auto line = std::to_wstring(msg->GetLineNumber());
		auto col = std::to_wstring(msg->GetStartColumn());
		errmsg += L"\t in " + line + L":" + col + L"\n";
	}
	return errmsg;
}

// TODO: Move this function to another thread and communite by Qt slot signal.
// TODO: Move v8 platform to ... wherever.. 

void ScriptCore::RunScript() {
	LogManager::instance().log("Start mod envoronment");
	// Step1. Init Js Cxt
	
	v8::V8::InitializeICU();
	// v8::V8::InitializeExternalStartupData(cpath);
	v8::V8::InitializePlatform(0);
	v8::V8::Initialize();
	auto flags = uccuConfig::instance().GetV8Flags();
	int argc = flags.length() + 1;
	char ** argv = new char*[argc];
	argv[0] = "RPGMV";
	for (int i = 1; i < argc; i++)
		argv[i] = flags[i].data();
	v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
	ArrayBufferAllocator array_buffer_allocator;
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = &array_buffer_allocator;
	v8::Isolate* isolate = v8::Isolate::New(create_params);
	{
		
		isolate->SetCaptureStackTraceForUncaughtExceptions(true);
		v8::Isolate::Scope isolate_scope(isolate);
		v8::HandleScope handle_scope(isolate); 
		if (uccuConfig::instance().enableV8Debug())
			DebugAgent::Enable("RMMV", uccuConfig::instance().GetV8DebugPort(), [](QString s) {LogManager::instance().log(s); }, uccuConfig::instance().waitForConnection());
		auto cxt = v8::Context::New(isolate);
		v8::Context::Scope context_scope(cxt);
		// Step2. Init C++/JS Objects
		v8::TryCatch try_catch;
		JSCore::initAll(isolate); 
		
		if (try_catch.HasCaught()) {
			auto error = GetErrorMessage(isolate, try_catch.Exception(), try_catch.Message());
			LogManager::instance().err(QString::fromWCharArray(error.c_str()));
			LogManager::instance().err("Cannot init runtime, give up");
		} else {
			// Step3. Excute loader, pass uccu namespace and returns $loader
			
			v8::Handle<v8::Function> f;
			if (v8pp::get_option(isolate, JSCore::require("module").As<v8::Object>(), "runMain", f)) {
				if (uccuConfig::instance().waitForConnection() && uccuConfig::instance().enableV8Debug())
					v8::Debug::DebugBreak(isolate);

				v8pp::call_v8(isolate, f, cxt->Global());
				if (try_catch.HasCaught() || try_catch.HasTerminated()) {
					if (!try_catch.Message().IsEmpty()) {
						auto error = GetErrorMessage(isolate, try_catch.Exception(), try_catch.Message());
						LogManager::instance().err(QString::fromWCharArray(error.c_str()));
					}
					v8::V8::TerminateExecution(isolate);
				}
			}
		}
		if (uccuConfig::instance().enableV8Debug())
			DebugAgent::Disable();
		JSCore::clearAll(isolate);
	}

	// Step6. Finalized
	isolate->Dispose();
	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete[] argv;
	LogManager::instance().log("Mod run over");
}

ScriptCore *ScriptCore::_ins;