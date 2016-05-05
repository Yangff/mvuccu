#include "ScriptCore.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

#if defined(_MSC_VER)
// Sleep time for Windows is 1 ms while it's 1 ns for POSIX
// Beware using this for your app. This is just to give a
// basic idea on usage
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep(x)
#endif

#include "jx.h"

void callback(JXResult *results, int argc) {
	// do nothing
}

ScriptCore::ScriptCore() {
	JX_Initialize("RPGMV.exe", callback);
	JX_InitializeNewEngine();
	// JX_DefineMainFile("require(\"mvuccu\")");
	JX_DefineMainFile("console.log('hello js!');");
}

void ScriptCore::RunScript()
{
	JX_StartEngine();
	while (JX_LoopOnce() != 0) Sleep(1);
	JX_StopEngine();
}
