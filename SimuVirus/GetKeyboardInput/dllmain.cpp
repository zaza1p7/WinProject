// APIHook.cpp : ���� DLL Ӧ�ó���ĵ���������
//

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

	//�ҵ�IATλ��
	hMod = GetModuleHandle(NULL);
	pAddr = (PBYTE)hMod;

	pAddr += *((DWORD*)&pAddr[0x3C]); //NTͷ

	dwRVA = *((DWORD*)&pAddr[0x80]);  //IAT��ƫ����

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hMod + dwRVA);

	//����Ѱ��Kernel32.dll
	for (; pImportDesc->Name; pImportDesc++)
	{
		szLibName = (LPCSTR)((DWORD)hMod + pImportDesc->Name);

		if (!_stricmp(szLibName, szDllName))
		{

			pThunk = (PIMAGE_THUNK_DATA)((DWORD)hMod + pImportDesc->FirstThunk);

			//����Ѱ��WriteFile
			for (; pThunk->u1.Function; pThunk++)
			{
				if (pThunk->u1.Function == (DWORD)pfnOrg)
				{
					VirtualProtect((LPVOID)&pThunk->u1.Function, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
					pThunk->u1.Function = (DWORD)pfnNew;
					VirtualProtect((LPVOID)&pThunk->u1.Function, 4, dwOldProtect, &dwOldProtect);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

//MyWriteFile����WriteFile�ĸ�ʽ��ȫһ��
BOOL WINAPI MyWriteFile(HANDLE hFile,
	LPCVOID lpBuffer, //ָ����±����ݵ�ָ��
	DWORD nNumberOfBytesToWrite, //���ݵĳ���
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	//MessageBox(NULL, L"Hook successfully!", L"Hello", MB_OK);
	for (DWORD i = 0; i < nNumberOfBytesToWrite; i++)
	{
		if (0x61 <= *((char *)lpBuffer + i) && *((char *)lpBuffer + i) <= 0x7A) //ö��ÿһ�������Ƿ���Сд��ĸ
			*((char *)lpBuffer + i) -= 0x20;  //ת��д

	}
	return ((PFWRITEFILE)g_function)(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//��ȡWriteFileԭʼ��ַ
		g_function = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "WriteFile");
		//Hook�ú���
		Hook("kernel32.dll", g_function, (PROC)MyWriteFile);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		//��Hook
		Hook("kernel32.dll", (PROC)MyWriteFile, g_function);
		break;
	}
	return TRUE;
}
