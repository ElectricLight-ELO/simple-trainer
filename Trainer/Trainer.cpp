#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
using namespace std;
DWORD GetProcessId(const char* processName)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create snapshot handle.\n";
        return 0;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnap, &pe32))
    {
        std::cerr << "Failed to get first process.\n";
        CloseHandle(hSnap);
        return 0;
    }

    do
    {
        if (strcmp(pe32.szExeFile, processName) == 0)
        {
            CloseHandle(hSnap);
            return pe32.th32ProcessID;
        }
    } while (Process32Next(hSnap, &pe32));

    std::cerr << "Process not found.\n";
    CloseHandle(hSnap);
    return 0;
}

void* GetModuleBaseAddress(DWORD procId, const char* modName)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create snapshot handle.\n";
        return nullptr;
    }

    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);

    if (!Module32First(hSnap, &me32))
    {
        std::cerr << "Failed to get first module.\n";
        CloseHandle(hSnap);
        return nullptr;
    }

    do
    {
        if (strcmp(me32.szModule, modName) == 0)
        {
            CloseHandle(hSnap);
            return me32.modBaseAddr;
        }
    } while (Module32Next(hSnap, &me32));

    std::cerr << "Module not found.\n";
    CloseHandle(hSnap);
    return nullptr;
}

void* findAddress(HANDLE processHandle, std::uint32_t moduleBaseAddress, const std::vector<std::uint32_t>& offsets)
{
    std::uint32_t prevAddress = moduleBaseAddress;
    for (int i = 0; i < offsets.size(); i++)
    {
        auto offset = offsets[i];
        prevAddress += offset;
        if (offsets.size() - i == 1)
        {
            break;
        }
        std::uint32_t readBuffer = 0;
        if (ReadProcessMemory(processHandle, reinterpret_cast<void*>(prevAddress), &readBuffer, sizeof(readBuffer), NULL) == FALSE)
        {
            return nullptr;
        }
        prevAddress = readBuffer;
    }
    return reinterpret_cast<void*>(prevAddress);
}
void work(HANDLE hProc, void* bAdress, vector<std::uint32_t> offsets, int need_value)
{
    void* finalAddress = findAddress(hProc, reinterpret_cast<std::uint32_t>(bAdress), offsets);
    std::cout << "Final address: " << finalAddress << "\n";

    WriteProcessMemory(hProc, reinterpret_cast<LPVOID>((DWORD)finalAddress), &need_value, sizeof(need_value), 0);
}
int main()
{
    int maxim_v = 9999999;
    const char* processName = "EarnToDie_2.exe";
    const char* moduleName = "earntodie_2.s86";
    std::vector<std::uint32_t> offsets_money = { 0x001E7BE8, 0x4, 0x1C, 0xE78 };
    std::vector<std::uint32_t> offsets_bullets = { 0x001E812C, 0x18, 0x1AC};
    DWORD processId = GetProcessId(processName);
    if (processId == 0)
    {
        return 1;
    }

    void* baseAddress = GetModuleBaseAddress(processId, moduleName);
    if (baseAddress == nullptr)
    {
        return 1;
    }
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (processHandle == NULL)
    {
        std::cerr << "Failed to open process.\n";
        return 1;
    }
    choice:
    cout << "Method work: " << endl;
    cout << "1. money" << endl;
    cout << "2. bullets" << endl;
    int method = 0;
    cin >> method;
    switch (method)
    {
    case 1:
    {
        work(processHandle, baseAddress, offsets_money, maxim_v);

        goto choice;
    }
    case 2:
    {
        work(processHandle, baseAddress, offsets_bullets, maxim_v);
        goto choice;
    }
    default:
        exit(0);
        break;
    }

    

    CloseHandle(processHandle);
    return 0;
}