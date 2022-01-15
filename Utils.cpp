#include "Utils.h"
#include <cwctype>

std::wstring trim(const std::wstring& s)
{
	auto start = s.begin();
	while (start != s.end() && std::iswspace(*start)) {
		start++;
	}

	auto end = s.end();
	do {
		end--;
	} while (std::distance(start, end) > 0 && std::iswspace(*end));

	return std::wstring(start, end + 1);
}
