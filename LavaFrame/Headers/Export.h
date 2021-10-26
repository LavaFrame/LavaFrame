#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "GlobalState.h"

extern LavaFrameState GlobalState;

void SaveFrame(const std::string filename) // Saves current frame as a png
{
	unsigned char* data = nullptr;
	int w, h;
	GlobalState.renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_png(std::string(GlobalState.output_path + filename).c_str(), w, h, 3, data, w * 3);
	delete data;
}
void SaveFrameTGA(const std::string filename) // Saves current frame as a png
{
	unsigned char* data = nullptr;
	int w, h;
	GlobalState.renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_tga(std::string(GlobalState.output_path + filename).c_str(), w, h, 3, data);
	delete data;
}
void SaveFrameJPG(const std::string filename, int jpgQuality) // Saves current frame as a bitmap-JPG
{
	unsigned char* data = nullptr;
	int w, h;
	GlobalState.renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_jpg(std::string(GlobalState.output_path + filename).c_str(), w, h, 3, data, jpgQuality);
	delete data;
}
void SaveFrameBMP(const std::string filename) // Saves current frame as a bitmap-JPG
{
	unsigned char* data = nullptr;
	int w, h;
	GlobalState.renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_bmp(std::string(GlobalState.output_path + filename).c_str(), w, h, 3, data);
	delete data;
}