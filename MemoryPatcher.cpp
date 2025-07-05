#include <iostream>
#include <windows.h>
using namespace std;

int main()
{
    int GoalData = 5000;
    int ChangeData = 10000;

    DWORD pid = 7716;
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (!hProcess) {
        std::cerr << "Failed to open process. Error code: " << GetLastError() << "\n";
        return 1;
    }

    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    LPVOID pMemory = SystemInfo.lpMinimumApplicationAddress;

    MEMORY_BASIC_INFORMATION mbi;

    while (pMemory < SystemInfo.lpMaximumApplicationAddress)
    {
        if (VirtualQueryEx(hProcess, pMemory, &mbi, sizeof(mbi)) == 0)
            break;

        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && (mbi.Protect & (PAGE_READWRITE | PAGE_READONLY | PAGE_EXECUTE_READWRITE)))
        {
            SIZE_T regionSize = mbi.RegionSize;
            LPVOID buffer = malloc(regionSize);

            if (buffer && ReadProcessMemory(hProcess, pMemory, buffer, regionSize, nullptr)) {
                for (size_t i = 0; i < regionSize - sizeof(int); ++i) {
                    int value = *(int*)((char*)buffer + i);
                    if (value == GoalData) {
                        LPVOID targetAddr = (LPVOID)((SIZE_T)pMemory + i);
                        WriteProcessMemory(hProcess, targetAddr, &ChangeData, sizeof(int), nullptr);
                        cout << "변경 완료 at address: " << targetAddr << endl;
                    }
                }
            }
            free(buffer);
        }

        pMemory = (LPVOID)((SIZE_T)pMemory + mbi.RegionSize);
    }

    CloseHandle(hProcess);
    return 0;
}
