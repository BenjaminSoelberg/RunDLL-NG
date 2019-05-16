/*
  A replacement 32/64 bit for RunDLL32 which works better than the buildin when working with malware.
*/
#include <iostream>
#include <string>
#include <charconv>
#include <iomanip>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <signal.h> 

using namespace std;

constexpr auto MAX_IMPORT_NAME_SIZE = 4096;
typedef UINT(CALLBACK * IMPORTED_FUNCTION)();

void SignalHandler(const int signal) {
	wcout << L"exception (" << signal << L")" << endl;
	exit(-1);
}

void PrintUsage() {
	wcout << L"Usage:  rundll32-ng.exe dllname <optional string arguments> <optional list of entrypoints>" << endl << endl;
	wcout << L"Help:   rundll32-ng random.dll" << endl;
	wcout << L"        Will load random.dll" << endl << endl;
	wcout << L"        rundll32-ng random.dll @SomeNamedImport" << endl;
	wcout << L"        Will load random.dll and execute named import \"SomeNamedImport\"" << endl << endl;
	wcout << L"        rundll32-ng random.dll #1" << endl;
	wcout << L"        Will load random.dll and execute ordinal import 1" << endl << endl;
	wcout << L"        rundll32-ng random.dll Some String Argument #1 #2 @SomeNamedImport" << endl;
	wcout << L"        Will load random.dll with argument \"Some String Argument\" and" << endl;
	wcout << L"        execute ordinal import 1, 2 and named import SomeNamedImport " << endl << endl;
}

void PrintError() {
	auto const errorCode = GetLastError();

	TCHAR* pMsgBuf = NULL;
	DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&pMsgBuf),
		0,
		NULL);

	if (nMsgLen) {
		wcout << L"0x" << setfill(L'0') << setw(8) << hex << errorCode << L" " << pMsgBuf << endl;
	} else {
		wcout << L"0x" << setfill(L'0') << setw(8) << hex << errorCode << L" Unknown error" << endl;
	}

	if (pMsgBuf)
		LocalFree(pMsgBuf);
}
string w2s(const wchar_t* w) {
	const auto ws = wstring(w);
#pragma warning(push)
#pragma warning(disable: 4244)
	return string(ws.begin(), ws.end());
#pragma warning(pop)
}

int w2word(const wchar_t* w) {
	auto const ns = w2s(w);
	int value = 0;
	const auto end = ns.data() + ns.size();
	const auto res = from_chars(ns.data(), end, value);
	return res.ec == errc() && res.ptr == end && value >= 0 && value <= 0xffff ? value : -1;
}

IMPORTED_FUNCTION ImportNamedFunction(const HINSTANCE hinstLib, const wchar_t* importName) {
	// Convert W to A
	auto const importNameA = w2s(importName).c_str();

	if (!importNameA) {
		return nullptr;
	}

	// Find import
	return (IMPORTED_FUNCTION)GetProcAddress(hinstLib, importNameA);
}

IMPORTED_FUNCTION ImportOrdinalFunction(HINSTANCE hinstLib, int importOrdinal) {
	// Find import
	return (IMPORTED_FUNCTION)GetProcAddress(hinstLib, MAKEINTRESOURCEA(importOrdinal));
}

HMODULE ImportLibrary(wchar_t* libraryPath) {
	HMODULE hinstLib = LoadLibraryEx(libraryPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
	wcout << L"Loading module \"" << libraryPath << L"\", ";
	if (hinstLib == NULL) {
		wcout << L"failed" << endl;
		PrintError();
	} else {
		wcout << L"ok" << endl;
	}

	return hinstLib;
}

void SetupConsole() {
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
}

int wmain(int argc, wchar_t* argv[])
{
	SetupConsole();
	signal(SIGSEGV, SignalHandler);

	wcout << L"RunDLL-NG, Version 1.2.0, (c) 2019 Benjamin Sølberg" << endl;
	wcout << L"---------------------------------------------------" << endl;
	wcout << L"Email   benjamin.soelberg@gmail.com" << endl;
	wcout << L"Github  https://github.com/benjaminsoelberg/rundll-ng" << endl << endl;

	if (argc < 2) {
		PrintUsage();
		return 1;
	}
	
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
					IMPORTED_FUNCTION importedFunction = nullptr;
					if (ch == L'@') {
						firstImportArgFound = true;
						importedFunction = ImportNamedFunction(hinstLib, &arg[1]); // &arg[1] either points to first char or to null byte
					} else if (ch == L'#') {
						firstImportArgFound = true;
						int importOrdinal = w2word(&arg[1]);
						if (importOrdinal >= 0) {
							importedFunction = ImportOrdinalFunction(hinstLib, importOrdinal);
						}
					} else if (firstImportArgFound) {
						result = 4;
						wcout << L"Unknown argument \"" << arg << L"\"" << endl;
						break;
					} else {
						continue;	// Ignore arguments pre import arguments 
									// as they are currently treated as arguments available to dll main (as well as the rest)
					}

					wcout << L"Calling import \"" << arg << L"\", ";
					if (importedFunction == nullptr) {
						result = 3;
						wcout << L"failed" << endl;
						break;
					}
					importedFunction(); // Execute the imported function
					wcout << L"ok" << endl;
				}
			}
		}
		FreeLibrary(hinstLib);
	} else {
		result = 2;
	}

	return result;
}
