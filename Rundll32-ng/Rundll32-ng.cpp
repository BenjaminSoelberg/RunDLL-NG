//
// My first attempt to create a new and better version of RUNDLL32.EXE
//

#include "stdafx.h"

#include <windows.h>
#include <string.h>
#include <atlstr.h>
#include "Rundll32-ng.h"

#define MAX_IMPORT_NAME_SIZE 4096
typedef UINT(CALLBACK* IMPORTED_FUNCTION)();

void printUsage()
{
	_tprintf(_T("RUNDLL32-NG.EXE <dllname> <optional entrypoint>"));
}

void FormatErrorMessage(DWORD ErrorCode)
{
	if (ErrorCode == 0x13d)
	{
		_tprintf(_T("0x%08X Unknown error"), ErrorCode);
		return;
	}

	TCHAR* pMsgBuf = NULL;
	DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		ErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&pMsgBuf),
		0,
		NULL);

	if (!nMsgLen)
	{
		_tprintf(_T("0x%08X Unknown error"), ErrorCode);
		return;
	}

	_tprintf(_T("0x%08X %s"), ErrorCode, pMsgBuf);
	LocalFree(pMsgBuf);
}

void printError()
{
	FormatErrorMessage(GetLastError());
}

IMPORTED_FUNCTION importFunction(HINSTANCE hinstLib, wchar_t* importNameW)
{
	// Convert W to A
	size_t i;
	char* importNameA = (char *)malloc(MAX_IMPORT_NAME_SIZE);
	wcstombs_s(&i, importNameA, (size_t)MAX_IMPORT_NAME_SIZE, importNameW, (size_t)MAX_IMPORT_NAME_SIZE);

	// Find import
	IMPORTED_FUNCTION importedFunction = (IMPORTED_FUNCTION)GetProcAddress(hinstLib, importNameA);

	if (importNameA)
	{
		free(importNameA);
	}

	if (importedFunction == NULL)
	{
		printError();
	}

	return importedFunction;
}

HMODULE loadLibrary(wchar_t* libraryNameW)
{
	HMODULE hinstLib = LoadLibrary(libraryNameW);
	if (hinstLib == NULL)
	{
		printError();
	}

	return hinstLib;
}

int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2 || argc > 3)
	{
		printUsage();
		return 1;
	}

	int result = 0;
	HINSTANCE hinstLib = loadLibrary(argv[1]);
	if (hinstLib != NULL)
	{
		if (argc == 3)
		{
			IMPORTED_FUNCTION importedFunction = importFunction(hinstLib, argv[2]);
			if (importedFunction != NULL)
			{
				importedFunction();
			}
			else {
				result = 3;
			}
		}
		FreeLibrary(hinstLib);
	}
	else {
		result = 2;
	}

	return result;
}
