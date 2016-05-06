#include "ScriptCore.h"
#include "uccuConfig.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

#include <libplatform/libplatform.h>
#include <v8.h>

#pragma comment(lib,"icui18n.lib")
#pragma comment(lib,"v8_libplatform.lib")
#pragma comment(lib,"v8_libbase.lib")
#pragma comment(lib,"v8_base_0.lib")
#pragma comment(lib,"v8_base_1.lib")
#pragma comment(lib,"v8_base_2.lib")
#pragma comment(lib,"v8_base_3.lib")
#pragma comment(lib,"v8_nosnapshot.lib")
#pragma comment(lib,"icuuc.lib")

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

void ScriptCore::RunScript() {
	v8::V8::InitializeICU();
	v8::V8::InitializeExternalStartupData("RPGMV.exe");
	v8::Platform* platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();
	// v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
	ArrayBufferAllocator array_buffer_allocator;
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = &array_buffer_allocator;
	v8::Isolate* isolate = v8::Isolate::New(create_params);
	{

	}
	isolate->Dispose();
	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete platform;
}
