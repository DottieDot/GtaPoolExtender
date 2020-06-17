#pragma once

#include <cstdint>

// Not 100% what the game does but close enough
constexpr char NormalizeChar(const char c)
{
	if (c >= 'A' && c <= 'Z')
	{
		return c + ('a' - 'A');
	}
	else if (c == '\\')
	{
		return '/';
	}
	else
	{
		return c;
	}
}

namespace rage
{
	using atHashValue = std::uint32_t;

	constexpr atHashValue atPartialStringHash(const char* key, const atHashValue initialHash = 0)
	{
		atHashValue hash = initialHash;
		while (*key)
		{
			hash += NormalizeChar(*key++);
			hash += hash << 10;
			hash ^= hash >> 6;
		}
		return hash;
	}

	constexpr atHashValue atStringHash(const char* key, const atHashValue initialHash = 0)
	{
		atHashValue hash = atPartialStringHash(key, initialHash);
		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}
}
