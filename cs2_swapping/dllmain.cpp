#include <iostream>
#include <Windows.h>
#include <cstdint>
#include <thread>
#include <chrono>
#include <process.h>
#include <filesystem>

#include <fmt/core.h>
#include <MinHook.h>

#pragma comment(lib, "libMinHook.x64.lib")

std::uintptr_t find_pattern(std::uintptr_t base, size_t length, const char* pattern, const char* mask)
{
    length -= strlen(mask);

    for (int i = 0; i <= length; i++)
    {
        bool found = true;

        for (int p = 0; mask[p]; p++)
        {
            if (mask[p] == 'x' && pattern[p] != reinterpret_cast<char*>(base)[i + p])
            {
                found = false;
                break;
            }
        }

        if (found)
            return base + i;
    }

    return NULL;
}

std::string to_str(std::wstring wstr)
{
    return std::string(wstr.begin(), wstr.end());
}

void replace_str(const std::string& find_, const std::string& to_, std::string& str)
{
    for (size_t npos = 0; (npos = str.find(find_)) != std::string::npos;)
        str.replace(npos, find_.size(), to_);
}

std::string normalize_path(std::string path)
{
    replace_str("\\\\", "/", path);
    replace_str("\\", "/", path);

    return path;
}

typedef __int64 (__fastcall* sub_180010B40_fn)(__int64 a1, const char* a2, __int64 a3, int a4, const char* a5);
sub_180010B40_fn o_sub_180010B40 = nullptr;

__int64 __fastcall sub_180010B40_hk(__int64 a1, const char* a2, __int64 a3, int a4, const char* a5)
{
    std::string path = normalize_path(a2);

    for (const auto& dir : std::filesystem::recursive_directory_iterator("./swapping"))
    {
        std::string absolute = to_str(std::filesystem::absolute(dir.path()));
        std::string relative = normalize_path(to_str(dir.path().c_str()));

        replace_str("./swapping/", "", relative);

        if (relative != path)
            continue;
        
        fmt::println("file {}", path.c_str());
        fmt::println("{:c}{:c} swapped to {}", 192, 196, relative.c_str());
        return o_sub_180010B40(a1, absolute.c_str(), a3, a4, "");
    }

    return o_sub_180010B40(a1, a2, a3, a4, a5);
}

void main_thread(void*)
{
    std::filesystem::create_directory("./swapping");

    fmt::print("waiting filesystem_stdio.dll module... ");

    std::uintptr_t base = NULL;
    for (; !base; base = std::uintptr_t(GetModuleHandle(L"filesystem_stdio.dll")))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    fmt::println("loaded.");

    std::uintptr_t fn_ptr = find_pattern(base, 0x6415C, "\x48\x8B\xC4\x44\x89\x48\x20\x48\x89\x50\x10", "xxxxxxxxxxx");
    if (!fn_ptr)
        return fmt::println("failed to find function");

    if (MH_Initialize() != MH_OK)
        return fmt::println("failed to initialize minhook");

    if (MH_CreateHook((void*)fn_ptr, &sub_180010B40_hk, (void**)&o_sub_180010B40) != MH_OK ||
        MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
        return fmt::println("failed to set hook");
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        AllocConsole();
        AttachConsole(ATTACH_PARENT_PROCESS);
        SetConsoleTitle(L"cs2 swapping");

        FILE* file = nullptr;
        freopen_s(&file, "CONOUT$", "w", stdout);

        _beginthread(&main_thread, NULL, nullptr);
    }

    return TRUE;
}

