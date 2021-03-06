// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <Windows.h>
#include <tchar.h>

FARPROC g_function = NULL;
typedef BOOL(WINAPI *PFWRITEFILE)(HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped);

BOOL Hook(LPCSTR szDllName, PROC pfnOrg, PROC pfnNew)
{
	HMODULE hMod;
	LPCSTR szLibName;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PIMAGE_THUNK_DATA pThunk;
	DWORD dwOldProtect;
	DWORD dwRVA;
	PBYTE pAddr;
	PIMAGE_OPTIONAL_HEADER64 pOptionalHeader;
	PIMAGE_NT_HEADERS64 pNtheaders;
	PIMAGE_DOS_HEADER pDosHeader;

	hMod = GetModuleHandle(NULL);
	pAddr = (PBYTE)hMod;
	pDosHeader = (PIMAGE_DOS_HEADER)pAddr;
	pNtheaders = (PIMAGE_NT_HEADERS64)(pAddr + pDosHeader->e_lfanew);
	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)(pAddr + pNtheaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	/*
	pAddr = (PBYTE)hMod;

	pAddr += *((DWORD*)&pAddr[0x3C]); 

	optionalHeader = (PIMAGE_OPTIONAL_HEADER64)(pAddr + 4 + sizeof(IMAGE_FILE_HEADER));

	dwRVA = *((DWORD*)&pAddr[0x80]);  
	//dwRVA = optionalHeader->DataDirectory[1].VirtualAddress;

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hMod + dwRVA);
	*/

	TCHAR s[100];

	_stprintf_s(s, _T("%X"), pImportDesc->Name);
	OutputDebugString(L"Start finding-->");
	OutputDebugString(s);
	//MessageBox(NULL, s, L"Hello", MB_OK);
	for (; pImportDesc->Name; pImportDesc++)
	{
		szLibName = (LPCSTR)(pAddr + pImportDesc->Name);
		OutputDebugStringA(szLibName);
		if (!_stricmp(szLibName, szDllName))
		{
			OutputDebugString(L"FOUND KERNEL32");
			pThunk = (PIMAGE_THUNK_DATA)(pAddr + pImportDesc->FirstThunk);

			for (; pThunk->u1.Function; pThunk++)
			{
				_stprintf_s(s, _T("WriteFile : %llX"), pfnOrg);
				OutputDebugString(s);
				_stprintf_s(s, _T("Now : %llX"), pThunk->u1.Function);
				OutputDebugString(s);
				if ((DWORD)pThunk->u1.Function == (DWORD)pfnOrg)
				{
					//MessageBox(NULL, L"FOUND WRITE FILE", L"Hello", MB_OK);
					OutputDebugString(L"FOUND WRITEFILE");
					VirtualProtect((LPVOID)&pThunk->u1.Function, 8, PAGE_EXECUTE_READWRITE, &dwOldProtect);
					pThunk->u1.Function = (ULONGLONG)pfnNew;
					VirtualProtect((LPVOID)&pThunk->u1.Function, 8, dwOldProtect, &dwOldProtect);
					return TRUE;
				}
			}
			return FALSE;
		}
	}
	return FALSE;
}

BOOL WINAPI MyWriteFile(HANDLE hFile,
	LPCVOID lpBuffer, 
	DWORD nNumberOfBytesToWrite, 
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	//MessageBox(NULL, L"Hook successfully!", L"Hello", MB_OK);
	OutputDebugString(L"HOOK WRITEFILE");
	for (DWORD i = 0; i < nNumberOfBytesToWrite; i++)
	{
		if (0x61 <= *((char *)lpBuffer + i) && *((char *)lpBuffer + i) <= 0x7A) 
			*((char *)lpBuffer + i) -= 0x20;  //×ª´óÐ´

	}
	return ((PFWRITEFILE)g_function)(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		g_function = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "WriteFile");
		Hook("kernel32.dll", g_function, (PROC)MyWriteFile);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

