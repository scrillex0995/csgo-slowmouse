#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <comdef.h>

struct p_module
{
	DWORD dw_base;
	DWORD dw_size;
};

class memory
{
public:
	bool attach(const char* ProcessName, DWORD rights)
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (snapshot == INVALID_HANDLE_VALUE) {
			return false;
		}

		bool result = false;
		if (Process32First(snapshot, &entry)) {
			do {
				if (strcmp(entry.szExeFile, ProcessName) == 0) {
					HANDLE process = OpenProcess(rights, false, entry.th32ProcessID);
					if (process != NULL) {
						pID = entry.th32ProcessID;
						_process = process;
						result = true;
						break;
					}
				}
			} while (Process32Next(snapshot, &entry));
		}

		CloseHandle(snapshot);
		return result;
	}

	void close_process(const char* ProcessName)
	{

		HANDLE hProcess;
		DWORD pid;
		pid = get_pid(ProcessName);
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		TerminateProcess(hProcess, -1);
		CloseHandle(hProcess);
	}

	p_module get_module(const char* moduleName)
	{
		MODULEENTRY32 mEntry = { sizeof(mEntry) };
		HANDLE module = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID);
		if (module == INVALID_HANDLE_VALUE) {
			return { 0, 0 };
		}

		p_module mod = { 0, 0 };
		while (Module32Next(module, &mEntry)) {
			if (strcmp(mEntry.szModule, moduleName) == 0) {
				mod.dw_base = reinterpret_cast<DWORD>(mEntry.modBaseAddr);
				mod.dw_size = mEntry.modBaseSize;
				break;
			}
		}

		CloseHandle(module);
		return mod;
	}

	template <class T>
	T read(const DWORD addr) {
		T _read;
		ReadProcessMemory(_process, reinterpret_cast<LPVOID>(addr), &_read, sizeof(T), NULL);
		return _read;
	}

	template <class T>
	void write(const DWORD addr, T val) {
		WriteProcessMemory(_process, reinterpret_cast<LPVOID>(addr), &val, sizeof(T), NULL);
	}
	void exit()
	{
		CloseHandle(_process);
	}
private:
	bool data_compare(const BYTE* pData, const BYTE* pMask, const char* pszMask) {
		for (; *pszMask; ++pszMask, ++pData, ++pMask) {
			if (*pszMask == 'x' && *pData != *pMask) {
				return false;
			}
		}
		return (*pszMask == NULL);
	}
	DWORD get_pid(const char* szProcessName) {
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			return 0;
		}

		DWORD pid = 0;
		PROCESSENTRY32 pe = { sizeof(pe) };
		if (Process32First(hSnapshot, &pe)) {
			do {
				if (strcmp(pe.szExeFile, szProcessName) == 0) {
					pid = pe.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnapshot, &pe));
		}

		CloseHandle(hSnapshot);
		return pid;
	}
	HANDLE _process;
	DWORD pID;
};