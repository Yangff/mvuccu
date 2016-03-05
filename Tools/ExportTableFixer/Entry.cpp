#include <Windows.h>
#include <dbghelp.h>
#include <ImageHlp.h>
#include <cstdio>
#include <string>

using namespace std;

int main(int argc, char** argv) {
	if (argc != 3) {
		printf("ExportTableFixer.exe FileToFix FileToDuplicate");
		return -1;
	}
	string org(argv[1]);
	string source(argv[2]);

	
}