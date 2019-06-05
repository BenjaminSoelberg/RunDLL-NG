#pragma once

using namespace std;

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>

#include <string>
#include <vector>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _EXPORT_INFO {
	string name;
	WORD ordinal;
	LPVOID address;

} EXPORT_INFO, *PEXPORT_INFO;

NTSTATUS GetExports(_In_ PVOID DllBase, _Out_ std::vector<EXPORT_INFO>** exports);


