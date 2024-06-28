#include <iostream>
#include <Windows.h>
#include <cstdint>
#include <thread>
#include <chrono>
#include <array>
#include <process.h>
#include <filesystem>
#include <Psapi.h>

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

typedef __int64(__fastcall* open_fn)(__int64 a1, const char* a2, __int64 a3, int a4, const char* a5);
open_fn o_open = nullptr;

__int64 __fastcall open_hk(__int64 a1, const char* a2, __int64 a3, int a4, const char* a5)
{
	const std::string path = normalize_path(a2);

	for (const auto& dir : std::filesystem::recursive_directory_iterator("./swapping"))
	{
		std::string absolute = std::filesystem::absolute(dir.path()).string();
		std::string relative = normalize_path(dir.path().string());

		replace_str("./swapping/", "", relative);

		if (relative != path)
			continue;

		fmt::println("file {}", path.c_str());
		fmt::println("{:c}{:c} swapped to {}", 192, 196, absolute);
		return o_open(a1, absolute.c_str(), a3, a4, "");
	}

	return o_open(a1, a2, a3, a4, a5);
}

void main_thread(void*)
{
	std::filesystem::create_directory("./swapping");

	fmt::print("waiting filesystem_stdio.dll module... ");

	HMODULE module_ = nullptr;
	for (; !module_; module_ = GetModuleHandle(L"filesystem_stdio.dll"))
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);
	}

	fmt::println("loaded.");

	MODULEINFO module_info = {};
	if (!GetModuleInformation(GetCurrentProcess(), module_, &module_info, sizeof(module_info)))
		return fmt::println("failed to retrieve module information: {:08X}", GetLastError());

	constexpr auto signatures = std::to_array<std::pair<const char*, const char*>>({
		{ "\x48\x8B\xC4\x44\x89\x48\x20\x48\x89\x50\x10", "xxxxxxxxxxx" },
		{ "\x44\x89\x4C\x24\x00\x4C\x89\x44\x24\x00\x48\x89\x54\x24\x00\x55", "xxxx?xxxx?xxxx?x" }
	});

	std::uintptr_t fn_ptr = NULL;

	for (const auto& [pattern, mask] : signatures)
	{
		fn_ptr = find_pattern(reinterpret_cast<std::uintptr_t>(module_), module_info.SizeOfImage, pattern, mask);
		if (fn_ptr)
			break;
	}

	if (!fn_ptr)
		return fmt::println("failed to find function");

	if (MH_Initialize() != MH_OK)
		return fmt::println("failed to initialize minhook");

	if (MH_CreateHook((void*)fn_ptr, &open_hk, (void**)&o_open) != MH_OK || MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
		return fmt::println("failed to set hook");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
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

