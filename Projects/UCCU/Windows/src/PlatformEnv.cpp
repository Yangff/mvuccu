#include <Windows.h>

char * GetExecPathUTF8() {
	int len = GetModuleFileNameW(NULL, NULL, 0);
	wchar_t * buf = new wchar_t[len + 1];
	GetModuleFileNameW(NULL, buf, len);
	buf[len] = 0;
	int utf8_len = WideCharToMultiByte(CP_UTF8,
		0,
		buf,
		-1,
		NULL,
		0,
		NULL,
		NULL);
	char *buf8 = new char[utf8_len];
	WideCharToMultiByte(CP_UTF8,
		0,
		buf,
		-1,
		buf8,
		utf8_len,
		NULL,
		NULL);
	delete[] buf;
	return buf8;
}

unsigned long long GetPid() {
	return GetCurrentProcessId();
}