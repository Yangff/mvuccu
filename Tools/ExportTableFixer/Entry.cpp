#include <Windows.h>
#include <cstdio>
#include <set>
#include <string>
#include <ImageHlp.h>
#include <vector>
#include <map>
using namespace std;

#define rva2va(rva) ImageRvaToVa(_ntheader, base, rva, NULL);

// from http://bbs.pediy.com/showthread.php?t=206197
DWORD size2AligentSize(DWORD dwFileSize, DWORD dwAligent)
{
	return
		((dwFileSize / dwAligent)*dwAligent)
		+
		(dwFileSize%dwAligent > 0 ? dwAligent : 0);
}

class PE {
	void* base;
	DWORD fsize;
	map<string, string> forwarding;
	PIMAGE_DOS_HEADER _dosHeader;
	PIMAGE_NT_HEADERS32 _ntheader;
public:
	PE(string fname):base(0) {
		HANDLE fHandle;
		fHandle = CreateFile(fname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (fHandle == INVALID_HANDLE_VALUE) {
			throw "Bad File";
		}
		fsize = GetFileSize(fHandle, NULL);

		base = VirtualAlloc(NULL, fsize, MEM_COMMIT, PAGE_READWRITE);

		DWORD readed;

		ReadFile(fHandle, base, fsize, &readed, NULL);

		CloseHandle(fHandle);

		_dosHeader = (PIMAGE_DOS_HEADER)base;
		_ntheader = (PIMAGE_NT_HEADERS32)((long)_dosHeader + (long)_dosHeader->e_lfanew);
	}
	~PE() { if (base) { VirtualFree(base, fsize, MEM_DECOMMIT); base = NULL; fsize = 0; } }
	
	void AddForwarding(string func_name, string module_name) {
		forwarding[func_name] = module_name + "." + func_name;
	}

	PIMAGE_EXPORT_DIRECTORY GetExportDirectory() {
		return (PIMAGE_EXPORT_DIRECTORY)
			rva2va(_ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	}

	DWORD GetFunctionNameCount() {
		auto export_dir = GetExportDirectory();
		return export_dir->NumberOfNames;
	}

	string GetFunctionName(DWORD id) {
		auto export_dir = GetExportDirectory();
		auto names = (char**)rva2va(export_dir->AddressOfNames);
		void *ptr = rva2va((ULONG)names[id]);
		return string((char*)ptr);
	}

	DWORD GetOrdId(DWORD id) {
		auto export_dir = GetExportDirectory();
		auto OrdId = (USHORT*)rva2va(export_dir->AddressOfNameOrdinals);
		return OrdId[id];
	}

	DWORD GetRVA(DWORD id) {
		auto export_dir = GetExportDirectory();
		auto RVAs = (DWORD*)rva2va(export_dir->AddressOfFunctions);
		return RVAs[id];
	}

	void dump(string fname) {
		if (forwarding.empty())
			return;

		map<string, DWORD> orgFunctions;

		auto cnt = GetFunctionNameCount();
		for (int i = 0; i < cnt; i++) {
			orgFunctions[(GetFunctionName(i))] = GetRVA(GetOrdId(i));
		}

		map<string, string> forwarding_map;

		for (auto x : forwarding) {
			if (orgFunctions.find(x.first) == orgFunctions.end()) {
				forwarding_map[x.first] = x.second;
			}
		}

		size_t size = 0;
		size += sizeof IMAGE_EXPORT_DIRECTORY;
		size += fname.size() + 1;
		size += (orgFunctions.size() + forwarding_map.size()) * sizeof DWORD;
		for (int i = 0; i < cnt; i++)
			size += GetFunctionName(i).size() + 1;
		for (auto x : forwarding_map)
			size += x.first.size() + 1 + x.second.size() + 1;
		size += (orgFunctions.size() + forwarding_map.size()) * 2 * sizeof DWORD;

		PIMAGE_FILE_HEADER pFileNt = &(_ntheader->FileHeader);
		PIMAGE_OPTIONAL_HEADER pOptNt = &(_ntheader->OptionalHeader);
		DWORD dwAligentSectionSize = pOptNt->SizeOfHeaders;
		DWORD dwLengthOfAllSection = pFileNt->NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

		DWORD secbase = 0;
		if ((dwLengthOfAllSection + sizeof(IMAGE_SECTION_HEADER) <= dwAligentSectionSize)) {
			const wchar_t *pszName = L".nsec"; DWORD dwNameLen = 5;
			PIMAGE_SECTION_HEADER  nesSection = &(IMAGE_FIRST_SECTION(_ntheader)[pFileNt->NumberOfSections]);
			WideCharToMultiByte(CP_ACP, 0,
				pszName, dwNameLen,
				(char*)nesSection->Name, dwNameLen,
				NULL, NULL);
			nesSection->Name[dwNameLen] = 0;
			
			nesSection->Misc.VirtualSize = size2AligentSize(size, pOptNt->SectionAlignment);
			nesSection->SizeOfRawData = size2AligentSize(size, pOptNt->FileAlignment);
			PIMAGE_SECTION_HEADER pOldSec = &(IMAGE_FIRST_SECTION(_ntheader)[pFileNt->NumberOfSections - 1]);
			nesSection->PointerToRawData = pOldSec->PointerToRawData + pOldSec->SizeOfRawData;
			nesSection->VirtualAddress = size2AligentSize(pOldSec->VirtualAddress + pOldSec->SizeOfRawData, pOptNt->SectionAlignment);
			++pFileNt->NumberOfSections; pOptNt->SizeOfImage += size2AligentSize(size, pOptNt->SectionAlignment);
			DWORD  dwSecAligSize = size2AligentSize(size, pOptNt->FileAlignment);
			char* lpNewFile = (char*)VirtualAlloc(NULL, fsize + dwSecAligSize, MEM_COMMIT, PAGE_READWRITE);//new char[fsize + dwSecAligSize];
			memcpy_s(lpNewFile, fsize + dwSecAligSize, base, fsize);
			memset(lpNewFile + fsize, 0, dwSecAligSize);

			secbase = nesSection->PointerToRawData;

			auto oldBase = base;

			base = lpNewFile;
			_dosHeader = (PIMAGE_DOS_HEADER)base;
			_ntheader = (PIMAGE_NT_HEADERS32)((long)_dosHeader + (long)_dosHeader->e_lfanew);


			auto imageStart = ((long)base + secbase);
			auto rvaStart = nesSection->VirtualAddress;

			PIMAGE_EXPORT_DIRECTORY nDic = (PIMAGE_EXPORT_DIRECTORY)imageStart;
			imageStart += sizeof IMAGE_EXPORT_DIRECTORY;
			rvaStart += sizeof IMAGE_EXPORT_DIRECTORY;

			memcpy_s(nDic, dwSecAligSize, GetExportDirectory(), sizeof IMAGE_EXPORT_DIRECTORY);

			nDic->NumberOfFunctions = nDic->NumberOfNames = (orgFunctions.size() + forwarding_map.size());

			char * name = (char*)imageStart; 
			nDic->Name = rvaStart;
			imageStart += fname.size() + 1; rvaStart += fname.size() + 1;
			memcpy_s(name, fname.size() + 1, fname.c_str(), fname.size());
			name[fname.size()] = 0;

			USHORT* ord = (USHORT*)imageStart;
			nDic->AddressOfNameOrdinals = rvaStart;
			imageStart += (orgFunctions.size() + forwarding_map.size()) * sizeof USHORT;
			rvaStart += (orgFunctions.size() + forwarding_map.size()) * sizeof USHORT;
			
			for (int i = 1; i <= (orgFunctions.size() + forwarding_map.size()); i++)
				ord[i - 1] = i;

			{
				int i = 0;
				DWORD* names = (DWORD*)imageStart;
				nDic->AddressOfNames = rvaStart; imageStart += (orgFunctions.size() + forwarding_map.size()) * sizeof DWORD;
				rvaStart += (orgFunctions.size() + forwarding_map.size()) * sizeof DWORD;

				DWORD* funcs = (DWORD*)imageStart;
				nDic->AddressOfFunctions = rvaStart; imageStart += (orgFunctions.size() + forwarding_map.size()) * sizeof DWORD;
				rvaStart += (orgFunctions.size() + forwarding_map.size()) * sizeof DWORD;

				for (auto x : orgFunctions) {
					char * buff = (char*)imageStart; names[i] = rvaStart; 
					imageStart += x.first.size() + 1; rvaStart += x.first.size() + 1;
					memcpy(buff, x.first.c_str(), x.first.size());
					buff[x.first.size()] = 0;
					funcs[i] = x.second;
					i++;
				}
				for (auto x : forwarding_map) {
					char * buff = (char*)imageStart; 
					names[i] = rvaStart; imageStart += x.first.size() + 1; rvaStart += x.first.size() + 1;
					memcpy(buff, x.first.c_str(), x.first.size());
					buff[x.first.size()] = 0;
					
					buff = (char*)imageStart;
					funcs[i] = rvaStart; imageStart += x.second.size() + 1; rvaStart += x.second.size() + 1;
					memcpy(buff, x.second.c_str(), x.second.size());
					buff[x.second.size()] = 0;

					i++;
				}
			}

			_ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = size;
			_ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = nesSection->VirtualAddress;

			VirtualFree(oldBase, fsize, MEM_DECOMMIT); oldBase = NULL;

			auto hNew = CreateFile(fname.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			DWORD writed;
			WriteFile(hNew, base, fsize + dwSecAligSize, &writed, NULL);
			CloseHandle(hNew);
		}

	}
};

int main(int argc, char** argv) {
	/*if (argc != 3) {
		printf("ExportTableFixer.exe FileToFix FileToDuplicate");
		return -1;
	}*/

	string org("C:\\Users\\SHERMAN\\Documents\\Projects\\mvuccu\\Build-results\\Windows\\Qt5Core"); //(argv[1]);
	string source("C:\\Users\\SHERMAN\\Documents\\Projects\\mvuccu\\Build-results\\Windows\\Qt5CoreOLD"); //(argv[2]);

	PE orgPE(org + ".dll"), srcPE(source + ".dll");

	int cnt = srcPE.GetFunctionNameCount();
	for (int i = 0; i < cnt; i++) {
		orgPE.AddForwarding(srcPE.GetFunctionName(i), "Qt5CoreOLD");
	}

	orgPE.dump(org + ".dll");
}