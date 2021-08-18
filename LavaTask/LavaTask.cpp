#pragma warning (disable : 4996)
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <chrono>
#include "stringmanager.h"

const char* script_filename = "default";
std::string apppath = "";
bool outputTime = false;

void console_log(std::string text) {
	printf((text + "\n").c_str());
}

std::wstring ExePath() {
	TCHAR buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
	return std::wstring(buffer).substr(0, pos);
}

void launch_renderer_wait(std::string arguments) {
	console_log(wstring_convert_string(ExePath()) + "\\LavaFrame.exe " + arguments);
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcessA(NULL, LPSTR(std::string(wstring_convert_string(ExePath()) + "\\LavaFrame.exe " + arguments).c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void launch_renderer_standalone(std::string arguments) {
	console_log(wstring_convert_string(ExePath()) + "\\LavaFrame.exe " + arguments);
	STARTUPINFOA si_a;
	PROCESS_INFORMATION pi_a;
	ZeroMemory(&si_a, sizeof(si_a));
	si_a.cb = sizeof(si_a);
	ZeroMemory(&pi_a, sizeof(pi_a));
	if (!CreateProcessA(NULL, LPSTR(std::string(wstring_convert_string(ExePath()) + "\\LavaFrame.exe " + arguments).c_str()), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si_a, &pi_a))
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return;
	}
	CloseHandle(pi_a.hProcess);
	CloseHandle(pi_a.hThread);
}

int main(int argc, char* argv[])
{
	if (argc < 2) //Erroring out when no script file is given
	{
		std::cerr << "Please specify the script file as the first CLI parameter." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	for (int i = 0; i < argc; ++i) {

		if (argv[i] != argv[0]) {
			script_filename = argv[i];
			console_log(script_filename);
		}
	}

	std::filesystem::path script_filepath = script_filename;
	std::ifstream script_file(script_filename);
	std::string filestr;
	std::string parameter = "";

	auto start_execution_timer = std::chrono::high_resolution_clock::now();

	//Script interpreter
	while (std::getline(script_file, filestr)) {
		parameter = filestr.substr(filestr.find(" ") + 1);
		switch (strint((filestr.substr(0, filestr.find(" "))).data())) {
		case strint("render_scene_simple"): //Runs simple scene render with 500 samples and no denoising
			console_log("Rendering scene " + parameter);
			launch_renderer_wait("--maxsamples 500 --noui --nomove --scene " + parameter);
			break;

		case strint("render"):
			console_log("Executed render command with parameters : " + parameter);
			launch_renderer_wait(parameter);
			break;

		case strint("render_create_thread_simple"): //Runs simple thread render with 500 samples and no denoising
			launch_renderer_standalone("--maxsamples 500 --nowindow --scene " + parameter);
			break;

		case strint("render_create_thread"):
			launch_renderer_standalone("--nowindow" + parameter);
			break;

		case strint("//"):
			break;

		case strint("#"):
			break;

		case strint("[timer]"):
			outputTime = true;
			break;

		case strint("log"):
			console_log(parameter);
			break;

		default: 
			console_log("\nUnknown command :");
			console_log("> " + filestr + "\n");
			break;
		}
	}

	if (outputTime) {
		console_log("\nOperation time : ");
		auto stop_execution_timer = std::chrono::high_resolution_clock::now();
		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop_execution_timer - start_execution_timer).count() << "ms";
	}

	return 0;
}
