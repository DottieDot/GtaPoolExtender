#include "main.hpp"

#include "pattern.hpp"
#include "rage.hpp"

#include <fstream>
#include <cstdint>
#include <MinHook.h>
#include <unordered_map>
#include <algorithm>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;

std::ofstream logFile;

bool CreateHook(void* target, void* detour, void** orig)
{
	return MH_CreateHook(target, detour, orig) == MH_OK && MH_EnableHook(target) == MH_OK;
}

std::unordered_map<rage::atHashValue, std::uint32_t> poolIncrements;
std::unordered_map<rage::atHashValue, std::uint32_t> requiredPoolSizes;

void LoadAdjustmentFile(const fs::path& file)
{
	nlohmann::json j;
	std::ifstream(file) >> j;

	logFile << fmt::format("Loading {}.json", file.filename().string()) << std::endl;

	for (const auto& [pool, _value] : j.items())
	{
		std::string value = _value.get<std::string>();
		if (!_value.is_string())
		{
			logFile << fmt::format("{}: value needs to be a string", pool) << std::endl;
			continue;
		}
		else if (value.size() < 2)
		{
			logFile << fmt::format("{}: invalid format", pool) << std::endl;
			continue;
		}

		rage::atHashValue poolHash = rage::atStringHash(pool.c_str());
		std::uint32_t num = 0;
		try
		{
			num = std::strtoul(value.c_str() + 1, nullptr, 0);
		}
		catch (...)
		{
			logFile << fmt::format("{}: invalid format", pool) << std::endl;
			continue;
		}

		if (value[0] == '+')
		{
			poolIncrements[poolHash] = poolIncrements[poolHash] + num;
		}
		else if (value[0] == '>')
		{
			requiredPoolSizes[poolHash] = std::max(requiredPoolSizes[poolHash], num);
		}
		else
		{
			logFile << fmt::format("{}: invalid format", pool) << std::endl;
			continue;
		}
	}
}

void LoadAdjustmentFiles(const fs::path& folder)
{
	for (auto it = fs::directory_iterator(folder); it != fs::directory_iterator(); ++it)
	{
		LoadAdjustmentFile(*it);
	}
}

void* GetSizeOfPool = nullptr;
typedef std::uint32_t(*GetSizeOfPool_t)(void* _this, rage::atHashValue pool, std::uint32_t minSize);
GetSizeOfPool_t OrigGetSizeOfPool = nullptr;

std::uint32_t MyGetSizeOfPool(void* _this, rage::atHashValue pool, std::uint32_t minSize)
{
	std::uint32_t size = OrigGetSizeOfPool(_this, pool, minSize);

	std::uint32_t min = 0;
	std::uint32_t add = 0;

	if (auto it = poolIncrements.find(pool); it != poolIncrements.end())
	{
		add = it->second;
	}
	if (auto it = requiredPoolSizes.find(pool); it != requiredPoolSizes.end())
	{
		min = it->second;
	}

	std::uint32_t newSize = std::max((size + add), min);
	if (newSize != size)
	{
		logFile << fmt::format("Pool {:08X} extended to {} (was {})", pool, newSize, size) << std::endl;
	}

	return newSize;
}

bool CreateHooks()
{
	return
		CreateHook(GetSizeOfPool, MyGetSizeOfPool, reinterpret_cast<void**>(&OrigGetSizeOfPool)) &&
		true;
}

void patch(HMODULE module)
{
	using namespace std::chrono;
	using namespace std::chrono_literals;

	logFile.open("PoolExtender.log");

	LoadAdjustmentFiles("PoolAdjustments/");

	logFile << "Waiting for window..." << std::endl;
	auto timeout = high_resolution_clock::now() + 20s;
	while (!FindWindowA("grcWindow", "Grand Theft Auto V") &&
		high_resolution_clock::now() < timeout)
	{
		std::this_thread::sleep_for(100ms);
	}

	if (high_resolution_clock::now() >= timeout)
	{
		logFile << "Failed to find game window" << std::endl;
		FreeLibraryAndExitThread(module, 0);
		return;
	}

	logFile << "Searching for rage::fwConfigManager::GetSizeOfPool." << std::endl;
	if (GetSizeOfPool = ScanPattern("\x45\x33\xDB\x44\x8B\xD2\x66\x44\x39\x59\x00\x74\x4B", "xxxxxxxxxx?xx"))
	{
		logFile << "rage::fwConfigManager::GetSizeOfPool found." << std::endl;
	}
	else
	{
		logFile << "Failed to find rage::fwConfigManager::GetSizeOfPool." << std::endl;
		return;
	}

	if (auto st = MH_Initialize(); st != MH_OK)
	{
		logFile << "Minhook initialization failed. error:" << st << std::endl;
		return;
	}

	logFile << "Creating hooks" << std::endl;
	if (!CreateHooks())
	{
		logFile << "Failed to create hooks" << std::endl;
	}
	logFile << "Hooks created" << std::endl;
}

void cleanup()
{
	logFile.close();
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}
