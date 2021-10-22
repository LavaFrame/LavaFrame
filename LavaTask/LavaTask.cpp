#pragma warning (disable : 4996)
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <chrono>
#include "stringmanager.h"

#define SIMPLE_RENDER_MODE_ARGS "--maxsamples 500 --noui --nomove --scene "
#define RENDERER_PATH "\\LavaFrame.exe "

const char* script_filename = "default";
std::string apppath = "";
bool outputTime = false;
int threadID = 0;

void console_log(std::string text) {
	printf((text + "\n").c_str());
}

std::wstring ExePath() {
	TCHAR buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
	return std::wstring(buffer).substr(0, pos);
}

void file_copy(std::string from, std::string to, bool copyRecursive) {
	std::error_code errorcode;
	std::filesystem::copy_options copyOptions = std::filesystem::copy_options::update_existing;
	if (copyRecursive) {
		copyOptions = std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive;
	}
	console_log("Copying from " + from + " to " + to + " | Recursive : " + bool_convert_string(copyRecursive));
	std::filesystem::copy(from, to, copyOptions, errorcode);
	console_log("Copy operation failed. Error : " + errorcode.message());
	if (errorcode.value() == 0) console_log("Copy action complete.");
	return;
}

void launch_renderer_wait(std::string arguments) {
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcessA(NULL, LPSTR(std::string(wstring_convert_string(ExePath()) + RENDERER_PATH + arguments).c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

PROCESS_INFORMATION launch_renderer_thread(std::string arguments, std::string threadid) {
	STARTUPINFOA si_a;
	PROCESS_INFORMATION pi_a;
	ZeroMemory(&si_a, sizeof(si_a));
	si_a.cb = sizeof(si_a);
	ZeroMemory(&pi_a, sizeof(pi_a));
	if (!CreateProcessA(NULL, LPSTR(std::string(wstring_convert_string(ExePath()) + RENDERER_PATH + "-w .lt-threadmode " + threadid + " " + arguments).c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si_a, &pi_a))
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
	}
	CloseHandle(pi_a.hProcess);
	CloseHandle(pi_a.hThread);
	return pi_a;
}

int main(int argc, char* argv[])
{
	if (argc < 2) // Erroring out when no script file is given
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
	std::string full_parameter = "";
	std::string parameter = "";
	std::string parameter_2 = "";
	PROCESS_INFORMATION local_pi{};
	bool waitForThreads = false;

	auto start_execution_timer = std::chrono::high_resolution_clock::now();

	//Script interpreter
	while (std::getline(script_file, filestr)) {
		full_parameter = filestr.substr(filestr.find(" ") + 1);
		parameter = filestr.substr(find_nth(filestr, " ", 1) + 1, (find_nth(filestr, " ", 2) - find_nth(filestr, " ", 1)) - 1);
		parameter_2 = filestr.substr(find_nth(filestr, " ", 2) + 1);
		switch (strint((filestr.substr(0, filestr.find(" "))).data())) {

		case strint("render_scene_simple"): // Runs simple scene render with 500 samples and no denoising
			console_log("Rendering scene " + parameter);
			launch_renderer_wait(SIMPLE_RENDER_MODE_ARGS + full_parameter);
			console_log("Render complete.");
			break;

		case strint("render_scene_simple_timed"): // Runs simple scene render with 500 samples and no denoising with the timer enabled
		{
			console_log("Rendering scene " + parameter + " timed");
			auto start_function_execution_timer = std::chrono::high_resolution_clock::now();
			launch_renderer_wait(SIMPLE_RENDER_MODE_ARGS + full_parameter);
			auto stop_function_execution_timer = std::chrono::high_resolution_clock::now();
			std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop_function_execution_timer - start_function_execution_timer).count() << "ms \n";
			console_log("Render complete.");
			break;
		}

		case strint("render_scene"): // Runs scene render
			console_log("Rendering scene " + parameter);
			launch_renderer_wait("--noui --nomove " + full_parameter);
			console_log("Render complete.");
			break;

		case strint("render_scene_timed"): // Runs scene render with timer
		{
			console_log("Rendering scene " + parameter);
			auto start_function_execution_timer = std::chrono::high_resolution_clock::now();
			launch_renderer_wait("--noui --nomove " + full_parameter);
			auto stop_function_execution_timer = std::chrono::high_resolution_clock::now();
			std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop_function_execution_timer - start_function_execution_timer).count() << "ms \n";
			console_log("Render complete.");
			break;
		}

		case strint("render"): // Runs scene render command
			console_log("Executed render command with parameters : " + full_parameter);
			launch_renderer_wait(full_parameter);
			console_log("Render complete.");
			break;

		case strint("render_timed"): // Runs scene render command
		{
			console_log("Executed render command with parameters : " + full_parameter);
			auto start_function_execution_timer = std::chrono::high_resolution_clock::now();
			launch_renderer_wait(full_parameter);
			auto stop_function_execution_timer = std::chrono::high_resolution_clock::now();
			std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop_function_execution_timer - start_function_execution_timer).count() << "ms \n";
			console_log("Render complete.");
			break;
		}

		case strint("render_create_thread_simple"): // Runs simple thread render with 500 samples and no denoising
			console_log("Created simple render thread for file : " + parameter);
			threadID++;
			waitForThreads = true;
			local_pi = launch_renderer_thread(SIMPLE_RENDER_MODE_ARGS + full_parameter, std::to_string(threadID));
			break;

		case strint("render_create_thread"): // Runs thread render
			console_log("Created render thread with parameters : " + full_parameter);
			threadID++;
			waitForThreads = true;
			local_pi = launch_renderer_thread("--nowindow" + full_parameter, std::to_string(threadID));
			break;

		case strint("//"):
			break;

		case strint("#"):
			break;

		case strint("[timer]"):
			outputTime = true;
			break;

		case strint("log"):
			console_log(full_parameter);
			break;

		case strint(""):
			break;

		case strint("file_copy"):
			file_copy(parameter, parameter_2, false);
			break;

		case strint("file_copy_recursive"):
			file_copy(parameter, parameter_2, true);
			break;

		default: 
			console_log("\nUnknown command :");
			console_log("> " + filestr + "\n");
			break;
		}
	}

	if (waitForThreads) {
		WaitForSingleObject(local_pi.hProcess, INFINITE);
	}

	if (outputTime) {
		console_log("\nOperation time : ");
		auto stop_execution_timer = std::chrono::high_resolution_clock::now();
		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop_execution_timer - start_execution_timer).count();
		console_log("ms \n");
	}

	return 0;
}
