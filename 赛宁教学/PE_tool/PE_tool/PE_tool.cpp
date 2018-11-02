// PE_tool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>

HANDLE fileHandle, hMod;
DWORD fileSize, rva, dwBytesWritten;
PVOID virtualpointer;
LPCTSTR fileName;
PIMAGE_DOS_HEADER pfileDosHeader;
PIMAGE_NT_HEADERS pfileNtHeaders;
IMAGE_IMPORT_DESCRIPTOR *pfileImport;
PIMAGE_SECTION_HEADER pfileSection;
PIMAGE_THUNK_DATA pfileThunk;
PIMAGE_IMPORT_BY_NAME pImportName;
LPDWORD fileNum;
char buffer[100];
int length = 100;

typedef struct dllData {
	LPWSTR szLibName;
	wchar_t** funcNames;
	int count;
} DllData, *pDllData;


PVOID MultiByte2WideChar(LPSTR multiByte)
{
	int wideSize = 0;
	PVOID wideChar = NULL;
	wideSize = MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, NULL, 0);
	wideChar = VirtualAlloc(NULL, wideSize * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
	MultiByteToWideChar(CP_UTF8, 0, multiByte, -1, (LPWSTR)wideChar, wideSize * sizeof(wchar_t));
	return wideChar;
}

void Open() {

	if ((fileHandle = CreateFileA(
		"D:\\tmp\\pe.exe" ,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL)) == INVALID_HANDLE_VALUE)
	{
		_tprintf(L"File open error!The error code is %x\n", GetLastError());
	}
	hMod = fileHandle;
	fileSize = GetFileSize(fileHandle, NULL);
	virtualpointer = VirtualAlloc(NULL, fileSize, MEM_COMMIT, PAGE_READWRITE);
}

void Write() {
	SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN);
	WriteFile(fileHandle,
		virtualpointer,      // start of data to write
		fileSize,  // number of bytes to write
		&dwBytesWritten, // number of bytes that were written
		NULL);
}

void Read() {
	if (!ReadFile(fileHandle, virtualpointer, fileSize, fileNum, NULL))
	{
		_tprintf(L"File read error!The error code is %x\n", GetLastError());
	}
}

void GetDosHeader() {
	pfileDosHeader = PIMAGE_DOS_HEADER(virtualpointer);
}

void GetNtHeader() {
	pfileNtHeaders = PIMAGE_NT_HEADERS((DWORD_PTR)virtualpointer + pfileDosHeader->e_lfanew);
	pfileSection = IMAGE_FIRST_SECTION(pfileNtHeaders);
}

DWORD Rva2Offset(DWORD rva, PIMAGE_SECTION_HEADER psh, PIMAGE_NT_HEADERS pnt)
{
	size_t i = 0;
	PIMAGE_SECTION_HEADER pSeh;
	if (rva == 0)
	{
		return (rva);
	}
	pSeh = psh;
	for (; i < pnt->FileHeader.NumberOfSections; i++)
	{
		if (rva >= pSeh->VirtualAddress && rva < pSeh->VirtualAddress +
			pSeh->Misc.VirtualSize)
		{
			break;
		}
		pSeh++;
	}
	return (rva - pSeh->VirtualAddress + pSeh->PointerToRawData);
}

pDllData RetriveDll(int index) {
	LPSTR szLibName;
	PVOID wszLibName = NULL;
	PVOID wc = NULL;
	int p = NULL;
	pDllData dll = NULL;
	wchar_t **funcNames = NULL;
	int* pRva;
	int count = 0;


	szLibName = (PCHAR)((DWORD_PTR)virtualpointer + Rva2Offset(pfileImport[index].Name, pfileSection, pfileNtHeaders));
	wszLibName = MultiByte2WideChar(szLibName);
	pRva = (int*)((DWORD_PTR)virtualpointer + Rva2Offset(pfileImport[index].OriginalFirstThunk, pfileSection, pfileNtHeaders));
	for (int* i = pRva; (*i) != 0; i++)
		count++;
	funcNames = (wchar_t **)malloc(count * 4);
	count = 0;
	for (int* i = pRva; *i != 0; i++)
	{
		size_t tmp;
		pImportName = PIMAGE_IMPORT_BY_NAME((DWORD_PTR)virtualpointer + Rva2Offset(*i, pfileSection, pfileNtHeaders));
		int len = MultiByteToWideChar(CP_UTF8, 0, pImportName->Name, -1, NULL, 0);
		//int len = strlen(pImportName->Name);
		funcNames[count] = (wchar_t *)malloc(len * sizeof(wchar_t));
		wc = MultiByte2WideChar(pImportName->Name);
		//mbstowcs_s(&tmp, wc, len, pImportName->Name, len);
		wcscpy_s(funcNames[count++], len, (LPWSTR)wc);
	}
	dll = (pDllData)malloc(sizeof(dllData));
	dll->szLibName = (LPWSTR)wszLibName;
	dll->funcNames = funcNames;
	dll->count = count;
	return dll;
}

void GetRVA() {
	rva = Rva2Offset(pfileNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
		pfileSection,
		pfileNtHeaders);
}

void GetImportTable() {
	pfileImport = (IMAGE_IMPORT_DESCRIPTOR *)((DWORD_PTR)virtualpointer + rva);
}

void Print(pDllData dll)
{
	_tprintf(L"DLL ===> %s\n", dll->szLibName);

	for (int i = 0; i < dll->count; i++)
	{
		_tprintf(L"    %s\n", dll->funcNames[i]);
	}
}


int main()
{
	memset(buffer, 0, 100);
	Open();
	Read();
	GetDosHeader();
	GetNtHeader();
	GetRVA();
	GetImportTable(); 
	int lenOfImport = sizeof(pfileImport);
	for (int i = 0; i >= 0; i++)
	{
		if (pfileImport[i].Characteristics == 0)
			break;
		Print(RetriveDll(i));
	}
	pfileNtHeaders->OptionalHeader.AddressOfEntryPoint = 10;
	Write();
	system("pause");
    return 0;
}

