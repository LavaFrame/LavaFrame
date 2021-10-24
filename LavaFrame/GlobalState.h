#pragma once
#include "Scene.h"
#include <string>

struct LavaFrameState {
	Scene* scene;
	Renderer* renderer;
	float mouseSensitivity = 0.005f;
	float previewScale = 0.5f;
	bool keyPressed = false;
	bool done = false;
	bool noUi = false;
	bool threadMode = false;
	bool noMove = false;
	bool noWindow = false;
	bool useDebug = false;
	bool displaySampleCounter = false;
	int maxSamples = -1;
	int sampleSceneIndex = 0;
	int selectedInstance = 0;
	int currentJpgQuality = 95;
	int displayX;
	int displayY;
	int nativeScreenWidth;
	int nativeScreenHeight;
	const char* output_path = "output\\";
	std::string exportName = "";
	std::string exportType = "png";
	std::string threadID = "0";
	std::string shadersDir = "./shaders/";
	std::string assetsDir = "./assets/";
	std::string releaseVersion = "Version 0.6.0";
	std::string versionString = "LavaFrame - " + releaseVersion;
};