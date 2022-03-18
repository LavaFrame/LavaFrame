#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_THREAD 1
#include <SDL2/SDL.h>
#include "stb_image.h"
#include "stb_image_write.h"
#include <tinyexr.h>
#include "GlobalState.h"

extern LavaFrameState GlobalState;

void SaveFrame(const std::string filename, SDL_Window* window) // Saves current frame as a png
{
	unsigned char* data = nullptr;
	int w, h;
	GlobalState.renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_png(std::string(GlobalState.output_path + filename).c_str(), w, h, 3, data, w * 3);
	delete data;
    SDL_SetWindowTitle(window, GlobalState.versionString.c_str());
}

void SaveFrameTGA(const std::string filename, SDL_Window* window) // Saves current frame as a png
{
	unsigned char* data = nullptr;
	int w, h;
	GlobalState.renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_tga(std::string(GlobalState.output_path + filename).c_str(), w, h, 3, data);
	delete data;
    SDL_SetWindowTitle(window, GlobalState.versionString.c_str());
}

void SaveFrameJPG(const std::string filename, int jpgQuality, SDL_Window* window) // Saves current frame as a bitmap-JPG
{
	unsigned char* data = nullptr;
	int w, h;
	GlobalState.renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_jpg(std::string(GlobalState.output_path + filename).c_str(), w, h, 3, data, jpgQuality);
	delete data;
    SDL_SetWindowTitle(window, GlobalState.versionString.c_str());
}
void SaveFrameBMP(const std::string filename, SDL_Window* window) // Saves current frame as a bitmap-JPG
{
	unsigned char* data = nullptr;
	int w, h;
	GlobalState.renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_bmp(std::string(GlobalState.output_path + filename).c_str(), w, h, 3, data);
	delete data;
    SDL_SetWindowTitle(window, GlobalState.versionString.c_str());
}

void SaveFrameEXR(const std::string outfilename, SDL_Window* window) {
    int width;
    int height;
    float* rgb;


    GlobalState.renderer->GetOutputBufferHDR(&rgb, width, height);

    EXRHeader header;
    InitEXRHeader(&header);

    switch (GlobalState.exrCompressionIndex) {
    case (0):
        header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;
        break;

    case (1):
        header.compression_type = TINYEXR_COMPRESSIONTYPE_RLE;
        break;

    case (2):
        header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIPS;
        break;

    case (3):
        header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;
        break;

    case (4):
        header.compression_type = TINYEXR_COMPRESSIONTYPE_PIZ;
        break;

    case (5):
        header.compression_type = TINYEXR_COMPRESSIONTYPE_ZFP;
        break;

    default:
        std::cout << "EXR export error : Invalid compression type." << std::endl;
        break;
    }

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    std::vector<float> images[3];
    images[0].resize(width * height);
    images[1].resize(width * height);
    images[2].resize(width * height);

    // Split RGBRGBRGB... into R, G and B layer
    for (int i = 0; i < width * height; i++) {
        images[0][i] = rgb[3 * i + 0];
        images[1][i] = rgb[3 * i + 1];
        images[2][i] = rgb[3 * i + 2];
    }

    float* image_ptr[3];
    image_ptr[0] = &(images[2].at(0)); // B
    image_ptr[1] = &(images[1].at(0)); // G
    image_ptr[2] = &(images[0].at(0)); // R

    image.images = (unsigned char**)image_ptr;
    image.width = width;
    image.height = height;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo*)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Must be (A)BGR order, since most of EXR viewers expect this channel order.
    strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of output image to be stored in .EXR
    }

    const char* err = NULL; // or nullptr in C++11 or later.
    int ret = SaveEXRImageToFile(&image, &header, std::string(GlobalState.output_path + outfilename).c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        fprintf(stderr, "Error saving EXR : %s\n", err);
        FreeEXRErrorMessage(err); // free's buffer for an error message
    }

    delete rgb;

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    SDL_SetWindowTitle(window, GlobalState.versionString.c_str());
}