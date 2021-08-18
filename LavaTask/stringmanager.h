#pragma once
#pragma warning (disable : 4996)
#include <string>
#include <codecvt>

constexpr unsigned int strint(const char* str, int h = 0) //Introducing the strint - a datatype for using strings in switchs. Converts them to an int for comparison.
{
	return !str[h] ? 5381 : (strint(str, h + 1) * 33) ^ str[h];
}

std::wstring string_convert_wstring(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string wstring_convert_string(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}