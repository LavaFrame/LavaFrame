#pragma once
#include <string>

#ifdef FLAREON_EXPORTS
#define FLAREON_API __declspec(dllexport)
#else
#define FLAREON_API __declspec(dllimport)
#endif

extern "C" FLAREON_API std::string getPreviewShader();