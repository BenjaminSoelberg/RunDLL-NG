//
// My first attempt to create a new and better version of RUNDLL32.EXE
//

#include "stdafx.h"

#include <windows.h>
#include <string.h>
#include <atlstr.h>
#include <signal.h> 
#include "Rundll32-ng.h"

#define MAX_IMPORT_NAME_SIZE 4096
typedef UINT(CALLBACK* IMPORTED_FUNCTION)();
// DEBUG: if import fails with exception no error is thrown !!!
void PrintUsage() {
	wprintf(L"Usage:  rundll32-ng.exe dllname <optional string arguments> <optional list of entrypoints>\r\n\r\n");
	wprintf(L"Help:   rundll32-ng random.dll\r\n");
	wprintf(L"        Will load random.dll\r\n\r\n");
	wprintf(L"        rundll32-ng random.dll @SomeNamedImport\r\n");
	wprintf(L"        Will load random.dll and execute named import \"SomeNamedImport\"\r\n\r\n");
	wprintf(L"        rundll32-ng random.dll #1\r\n");
	wprintf(L"        Will load random.dll and execute ordinal import 1\r\n\r\n");
	wprintf(L"        rundll32-ng random.dll Some String Argument #1 #2 @SomeNamedImport\r\n");
	wprintf(L"        Will load random.dll with argument \"Some String Argument\" and\r\n");
	wprintf(L"        execute ordinal import 1, 2 and named import SomeNamedImport \r\n\r\n");
}

void FormatErrorMessage(DWORD ErrorCode) {
	if (ErrorCode == 0x13d) {
		wprintf(L"0x%08X Unknown error", ErrorCode);
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

	if (!nMsgLen) {
		wprintf(L"0x%08X Unknown error", ErrorCode);
		return;
	}

	wprintf(L"0x%08X %s", ErrorCode, pMsgBuf);
	LocalFree(pMsgBuf);
}

void PrintError() {
	FormatErrorMessage(GetLastError());
}

// I hate C so much
char* w2a(wchar_t* w) {
	size_t sizeA;
	wcstombs_s(&sizeA, NULL, 0, w, _TRUNCATE);
	char* a = (char*)malloc(sizeA);
	wcstombs_s(&sizeA, a, sizeA, w, _TRUNCATE);
	return a;
}


// I hate C even more
int w2word(wchar_t* w) {
	char* startA = w2a(w);
	errno = 0;
	char* endA = NULL;
	long word = strtol(startA, &endA, 10);

	if (errno == ERANGE || word < 0 || word > 0xFFFF || static_cast<unsigned>(endA - startA) != strlen(startA)) {
		return -1;
	}

	if (startA) {
		free(startA);
	}

	return word;
}


IMPORTED_FUNCTION ImportNamedFunction(HINSTANCE hinstLib, wchar_t* importName) {
	// Convert W to A
	char* importNameA = w2a(importName);

	// Find import
	IMPORTED_FUNCTION importedFunction = (IMPORTED_FUNCTION)GetProcAddress(hinstLib, importNameA);

	if (importNameA) {
		free(importNameA);
	}

	return importedFunction;
}

IMPORTED_FUNCTION ImportOrdinalFunction(HINSTANCE hinstLib, int importOrdinal) {
	// Find import
	return (IMPORTED_FUNCTION)GetProcAddress(hinstLib, MAKEINTRESOURCEA(importOrdinal));
}

HMODULE ImportLibrary(wchar_t* libraryPath) {
	HMODULE hinstLib = LoadLibraryEx(libraryPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
	wprintf(L"Loading module \"%s\", ", libraryPath);
	if (hinstLib == NULL) {
		wprintf(L"failed\r\n");
		PrintError();
	}
	else {
		wprintf(L"ok\r\n");
	}

	return hinstLib;
}
void SignalHandler(int signal)
{
	wprintf(L"exception");
	exit(-1);
}

int wmain(int argc, wchar_t* argv[]) {
	wprintf(L"RunDLL-NG, Version 1.10, (c) 2019 Benjamin Soelberg\r\n");
	wprintf(L"---------------------------------------------------\r\n");
	wprintf(L"Email   benjamin.soelberg@gmail.com\r\n");
	wprintf(L"Github  https://github.com/BenjaminSoelberg/RunDLL-NG\r\n\r\n");

	if (argc < 2) {
		PrintUsage();
		return 1;
	}

	typedef void (*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGSEGV, SignalHandler);

	int result = 0;
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	HINSTANCE hinstLib = ImportLibrary(argv[1]);
	if (hinstLib != NULL) {
		if (argc >= 3) {
			const int START_INDEX = 2;
			bool firstImportArgFound = false;
			for (int i = START_INDEX; i < argc; i++) {
				wchar_t* arg = argv[i];
				size_t argLen = wcslen(arg);
				if (argLen > 0) {
					wchar_t ch = arg[0];
					IMPORTED_FUNCTION importedFunction = NULL;
					if (ch == L'@') {
						firstImportArgFound = true;
						importedFunction = ImportNamedFunction(hinstLib, &arg[1]); // &arg[1] either points to first char or to null byte
					}
					else if (ch == L'#') {
						firstImportArgFound = true;
						int importOrdinal = w2word(&arg[1]);
						if (importOrdinal >= 0) {
							importedFunction = ImportOrdinalFunction(hinstLib, importOrdinal);
						}
					}
					else if (firstImportArgFound) {
						result = 4;
						wprintf(L"Unknown argument \"%s\"\r\n", arg);
						break;
					}
					else {
						continue;	// Ignore arguments pre import arguments 
									// as they are currently treated as arguments available to dll main (as well as the rest)
					}

					wprintf(L"Calling import \"%s\", ", arg);
					if (importedFunction == NULL) {
						result = 3;
						wprintf(L"failed\r\n");
						break;
					}
					importedFunction(); // Execute the imported function
					wprintf(L"ok\r\n");
				}
			}
		}
		FreeLibrary(hinstLib);
	}
	else {
		result = 2;
	}

	return result;
}
