#pragma once
constexpr unsigned int strint(const char* str, int h = 0) //Introducing the strint - a datatype for using strings in switchs. Converts them to an int for comparison.
{
	return !str[h] ? 5381 : (strint(str, h + 1) * 33) ^ str[h];
}