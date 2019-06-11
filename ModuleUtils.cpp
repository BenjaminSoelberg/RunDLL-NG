#include "ModuleUtils.h"

using namespace std;

#include <iostream>
#include <iomanip>

template <typename T>
inline T PTR_ADD_OFFSET(PVOID p, ULONG delta) noexcept {
	return reinterpret_cast<T>((ULONG_PTR)(p)+(ULONG_PTR)(delta));
}

NTSTATUS PhGetLoaderEntryImageDirectory(
	_In_ PVOID BaseAddress,
	_In_ PIMAGE_NT_HEADERS ImageNtHeader,
	_In_ ULONG ImageDirectoryIndex,
	_Out_ PIMAGE_DATA_DIRECTORY* ImageDataDirectoryEntry,
	_Out_ PVOID* ImageDirectoryEntry,
	_Out_opt_ SIZE_T* ImageDirectoryLength
)
{
	PIMAGE_DATA_DIRECTORY directory;

	directory = &ImageNtHeader->OptionalHeader.DataDirectory[ImageDirectoryIndex];

	if (directory->VirtualAddress == 0 || directory->Size == 0)
		return STATUS_INVALID_FILE_FOR_SECTION;

	*ImageDataDirectoryEntry = directory;
	*ImageDirectoryEntry = PTR_ADD_OFFSET<PVOID>(BaseAddress, directory->VirtualAddress);
	if (ImageDirectoryLength)* ImageDirectoryLength = directory->Size;

	return STATUS_SUCCESS;
}

NTSTATUS PhGetLoaderEntryImageNtHeaders(
	_In_ PVOID BaseAddress,
	_Out_ PIMAGE_NT_HEADERS* ImageNtHeaders
)
{
	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS ntHeader;
	ULONG ntHeadersOffset;

	dosHeader = PTR_ADD_OFFSET<PIMAGE_DOS_HEADER>(BaseAddress, 0);

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT;

	ntHeadersOffset = (ULONG)dosHeader->e_lfanew;

	if (ntHeadersOffset == 0 || ntHeadersOffset >= 0x10000000)
		return STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT;

	ntHeader = PTR_ADD_OFFSET<PIMAGE_NT_HEADERS>(BaseAddress, ntHeadersOffset);

	if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
		return STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT;

	*ImageNtHeaders = ntHeader;
	return STATUS_SUCCESS;
}

NTSTATUS GetExports(_In_ PVOID DllBase, _Out_ vector<EXPORT_INFO>** exports)
{
	PIMAGE_NT_HEADERS imageNtHeader;
	PIMAGE_DATA_DIRECTORY dataDirectory;
	PIMAGE_EXPORT_DIRECTORY exportDirectory;

	auto result = new vector<EXPORT_INFO>;
	NTSTATUS status;

	if (!NT_SUCCESS(status = PhGetLoaderEntryImageNtHeaders(DllBase, &imageNtHeader)))
		return status;

	if (!NT_SUCCESS(status = PhGetLoaderEntryImageDirectory(
		DllBase,
		imageNtHeader,
		IMAGE_DIRECTORY_ENTRY_EXPORT,
		&dataDirectory,
		reinterpret_cast<PVOID*>(&exportDirectory),
		NULL
	)))
		return status;

	auto exportAddressTable = PTR_ADD_OFFSET<PULONG>(DllBase, exportDirectory->AddressOfFunctions);
	auto exportNameTable = PTR_ADD_OFFSET<PULONG>(DllBase, exportDirectory->AddressOfNames);
	auto exportOrdinalNameTable = PTR_ADD_OFFSET<PUSHORT>(DllBase, exportDirectory->AddressOfNameOrdinals);
	for (ULONG i = 0; i < exportDirectory->NumberOfFunctions; i++)
	{
		auto exportAddress = PTR_ADD_OFFSET<PULONG>(DllBase, exportAddressTable[i]);
		WORD ordinal = IMAGE_ORDINAL(exportDirectory->Base + i);
		string name = "";
		for (ULONG j = 0; j < exportDirectory->NumberOfNames; j++)
		{
			if (exportOrdinalNameTable[j] == i)
			{
				name = string(PTR_ADD_OFFSET<LPSTR>(DllBase, exportNameTable[j]));
				break;
			}
		}

		EXPORT_INFO exportInfo = EXPORT_INFO{ name, ordinal, exportAddress };
		result->push_back(exportInfo);
	}
	*exports = result;
	return STATUS_SUCCESS;
}
