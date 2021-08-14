/*
* Hi, and welcome to the LavaFrame source code !
* The goal of this project was to create a (somewhat) production
* ready renderer. I used it as a
* project to build my C++ and GLSL skills further by forking and
* extending another project and building it into something I can
* use for my own projects.
*
* THIS SOURCE CODE CAN BE A BIT MESSY AT TIMES !!! PLEASE BEWARE AND FEEL FREE TO IMPROVE ON IT !
*
* - Nolram
*
* MIT License
* Check license.txt for more information on licensing
* Based on the original software by Alif Ali (knightcrawler25)
*/

#define _USE_MATH_DEFINES

#include <SDL2/SDL.h>
#include <GL/gl3w.h>
#include <iostream>
#include <io.h>

#include <time.h>
#include <math.h>
#include <string>

#include "Scene.h"
#include "TiledRenderer.h"
#include "Camera.h"
#include "Strint.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include "Loader.h"
#include "ImGuizmo.h"
#include "tinydir.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"


using namespace std;
using namespace LavaFrame;

Scene* scene = nullptr;
Renderer* renderer = nullptr;

std::vector<string> sceneFiles;

float mouseSensitivity = 0.005f;
bool keyPressed = false;
int sampleSceneIndex = 0;
int selectedInstance = 0;
double lastTime = SDL_GetTicks();
bool done = false;
bool noUi = false;
bool noWindow = false;
bool useDebug = false;
std::string releaseVersion = "Version 0.5.2";
std::string versionString = "LavaFrame - " + releaseVersion;
int maxSamples = -1;
float previewScale = 0.5f;
bool useNeutralTonemap = false;
std::string exportname = "0";
int currentJpgQuality = 95;

std::string shadersDir = "./shaders/";
std::string assetsDir = "./assets/";

RenderOptions renderOptions;

struct LoopData
{
	SDL_Window* mWindow = nullptr;
	SDL_GLContext mGLContext = nullptr;
};

void GetSceneFiles() //Load and index all scene files in a directory.
{
	tinydir_dir dir;
	int i;
	tinydir_open_sorted(&dir, assetsDir.c_str());

	for (i = 0; i < dir.n_files; i++)
	{
		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		if (std::string(file.extension) == "ignition") //Support for old ignition files
		{
			sceneFiles.push_back(assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "scene") //Support for old scene files
		{
			sceneFiles.push_back(assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "lfs") //LavaFrame Scene
		{
			sceneFiles.push_back(assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "lf") //LavaFrame Scene
		{
			sceneFiles.push_back(assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "lavaframe") //Full name file
		{
			sceneFiles.push_back(assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "lavaframescene") //Completely typed out filename
		{
			sceneFiles.push_back(assetsDir + std::string(file.name));
		}
	}

	tinydir_close(&dir);
}

void LoadScene(std::string sceneName) //Load scene - this is also called on startup.
{
	delete scene;
	scene = new Scene();
	LoadSceneFromFile(sceneName, scene, renderOptions);
	selectedInstance = 0;
	scene->renderOptions = renderOptions;
}

bool InitRenderer() //Create the tiled renderer and inform the user that the proccess has started.
{
	delete renderer;
	renderer = new TiledRenderer(scene, shadersDir);
	renderer->Init();
	printf("LavaFrame renderer started !\n");
	return true;
}

void SaveFrame(const std::string filename) //Saves current frame as a png
{
	unsigned char* data = nullptr;
	int w, h;
	renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_png(filename.c_str(), w, h, 3, data, w * 3);
	delete data;
}
void SaveFrameTGA(const std::string filename) //Saves current frame as a png
{
	unsigned char* data = nullptr;
	int w, h;
	renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_tga(filename.c_str(), w, h, 3, data);
	delete data;
}
void SaveFrameJPG(const std::string filename, int jpgQuality) //Saves current frame as a bitmap-JPG
{
	unsigned char* data = nullptr;
	int w, h;
	renderer->GetOutputBuffer(&data, w, h);
	stbi_flip_vertically_on_write(true);
	stbi_write_jpg(filename.c_str(), w, h, 3, data, jpgQuality);
	delete data;
}

void Render() //Main Render function for ImGUI and the renderer.
{
	auto io = ImGui::GetIO();
	renderer->Render();
	//const glm::ivec2 screenSize = renderer->GetScreenSize();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	renderer->Present();

	// Rendering of window
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Update(float secondsElapsed)
{
	keyPressed = false;
	if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) && ImGui::IsAnyMouseDown() && !ImGuizmo::IsOver()) //Mouse control
	{
		if (ImGui::IsMouseDown(0))
		{
			ImVec2 mouseDelta = ImGui::GetMouseDragDelta(0, 0);
			scene->camera->OffsetOrientation(mouseDelta.x, mouseDelta.y); //Move camera in 3D
			ImGui::ResetMouseDragDelta(0);
		}
		else if (ImGui::IsMouseDown(1))
		{
			ImVec2 mouseDelta = ImGui::GetMouseDragDelta(1, 0);
			scene->camera->SetRadius(mouseSensitivity * mouseDelta.y); //"Scroll" camera in 3D
			ImGui::ResetMouseDragDelta(1);
		}
		else if (ImGui::IsMouseDown(2))
		{
			ImVec2 mouseDelta = ImGui::GetMouseDragDelta(2, 0);
			scene->camera->Strafe(mouseSensitivity * mouseDelta.x, mouseSensitivity * mouseDelta.y); //Move the camera sidewards
			ImGui::ResetMouseDragDelta(2);
		}
		scene->camera->isMoving = true; //Render preview frames
	}


	// Maximum sample auto export
	if (maxSamples == renderer->GetSampleCount()) {
		if (exportname == "0") {
			SaveFrame("./render_" + to_string(renderer->GetSampleCount()) + ".png");
		}
		else {
			SaveFrame("./" + exportname + ".png");
		}
		printf("Render finished !");
		exit(0);
	}
	renderer->Update(secondsElapsed);
}

void EditTransform(const float* view, const float* projection, float* matrix)
{
	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

	if (ImGui::IsKeyPressed(90))  //Hotkeys for object translation
	{
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	}

	if (ImGui::IsKeyPressed(69))
	{
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	}

	if (ImGui::IsKeyPressed(82))
	{
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	}

	if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
	{
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	}

	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
	{
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	}

	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
	{
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	}

	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
	ImGui::InputFloat3("Tr", matrixTranslation);
	ImGui::InputFloat3("Rt", matrixRotation);
	ImGui::InputFloat3("Sc", matrixScale);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

	if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
		{
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		}

		ImGui::SameLine();
		if (ImGui::RadioButton("Global", mCurrentGizmoMode == ImGuizmo::WORLD))
		{
			mCurrentGizmoMode = ImGuizmo::WORLD;
		}
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(view, projection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, NULL);
}

void MainLoop(void* arg) //Its the main loop !
{
	LoopData& loopdata = *(LoopData*)arg;

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT)
		{
			done = true;
		}
		if (event.type == SDL_WINDOWEVENT) // Render resolution gets set to window size
		{
			if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				scene->renderOptions.resolution = iVec2(event.window.data1, event.window.data2);
				InitRenderer(); //Restart renderer on window resize. 
			}

			if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(loopdata.mWindow))
			{
				done = true;
			}
		}
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(loopdata.mWindow);
	ImGui::NewFrame();
	ImGuizmo::SetOrthographic(false);

	ImGuizmo::BeginFrame();
	{
		//io.Fonts->GetTexDataAsAlpha8(); Old font handling

		//Window flags
		static bool no_titlebar = false;
		static bool no_menu = false;
		static bool no_move = true;
		static bool no_resize = false;
		static bool window_override_size = true;

		bool optionsChanged = false;

		ImGuiWindowFlags window_flags = 0;
		if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
		if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
		if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
		if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;

		if (noUi == false) {
			ImGui::SetNextWindowPos({ 0, 1 });
			if (window_override_size) {
				ImGui::SetNextWindowSize({ 340, 512 });
				window_override_size = false;
			}
			ImGui::Begin(versionString.c_str(), nullptr, window_flags); //Main panel


			if (ImGui::BeginMenuBar()) // Export menu
			{
				if (ImGui::BeginMenu("Export"))
				{
					if (ImGui::MenuItem("Export as JPG", "")) {
						if (exportname == "0") {
							SaveFrameJPG("./render_" + to_string(renderer->GetSampleCount()) + ".jpg", currentJpgQuality);
						}
						else {
							SaveFrameJPG("./" + exportname + ".jpg", 80);
						}
					}
					if (ImGui::MenuItem("Export as PNG", "")) {
						if (exportname == "0") {
							SaveFrame("./render_" + to_string(renderer->GetSampleCount()) + ".png");
						}
						else {
							SaveFrame("./" + exportname + ".png");
						}
					}
					if (ImGui::MenuItem("Export as TGA", "")) {
						if (exportname == "0") {
							SaveFrameTGA("./render_" + to_string(renderer->GetSampleCount()) + ".tga");
						}
						else {
							SaveFrameTGA("./" + exportname + ".tga");
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			if (useDebug) { //Some debug stats, --debug / -db
				ImGui::Text("- Debug Mode -");
				ImGui::Text("Debug enabled : %d", useDebug);
				ImGui::Text("Render size : %d x %d", renderer->GetScreenSize().x, renderer->GetScreenSize().y);
				if (ImGui::Button("Reload scenes")) //Button for working on shaders or tonemaps to restart the renderer without a complete application restart.
				{
					sceneFiles.clear();
					GetSceneFiles();
				}
			}

			ImGui::Text("Rendered samples: %d ", renderer->GetSampleCount()); //Sample counter
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Currently rendered samples per pixel.");

			if (useDebug) {
				if (ImGui::Button("Recompile shaders")) //Button for working on shaders or tonemaps to restart the renderer without a complete application restart.
				{
					InitRenderer(); //Recompile shaders and restart the renderer.
				}
			}

			//Controls instruction - these will go into the documentation since they are universal.
			//ImGui::BulletText("LMB + drag to rotate");
			//ImGui::BulletText("MMB + drag to pan");
			//ImGui::BulletText("RMB + drag to zoom in/out");

			ImGui::Separator();
			ImGui::Text("\n");

			if (ImGui::CollapsingHeader("System"))
			{
				std::vector<const char*> scenes;
				for (int i = 0; i < sceneFiles.size(); ++i)
				{
					scenes.push_back(sceneFiles[i].c_str());
				}

				if (ImGui::Combo("Active scene", &sampleSceneIndex, scenes.data(), scenes.size(), scenes.size()))
				{
					LoadScene(sceneFiles[sampleSceneIndex]);
					SDL_RestoreWindow(loopdata.mWindow);
					SDL_SetWindowSize(loopdata.mWindow, renderOptions.resolution.x, renderOptions.resolution.y);
					InitRenderer();
				}
				ImGui::SliderInt("JPG export quality", &currentJpgQuality, 1, 200);
				optionsChanged |= ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.001f, 1.0f);

				ImGui::Text("\n");
			}

			ImGui::Text("\n");

			if (ImGui::CollapsingHeader("Render Settings"))
			{
				bool requiresReload = false;
				Vec3* bgCol = &renderOptions.bgColor;

				optionsChanged |= ImGui::SliderInt("Max ray-depth", &renderOptions.maxDepth, 1, 20);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Limits the amount of times a ray is allowed to bounce. Causes longer render times for better quality.");
				//requiresReload |= ImGui::Checkbox("Enable RR", &renderOptions.enableRR); Seems to currently do nothing, left it disabled for UI clarity. Can be set from scene file.
				//requiresReload |= ImGui::SliderInt("RR depth", &renderOptions.RRDepth, 1, 10);
				ImGui::Separator();
				requiresReload |= ImGui::Checkbox("Enable HDRI", &renderOptions.useEnvMap);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Enable lighting from a 360 degree HDR image for lighting.");
				optionsChanged |= ImGui::SliderFloat("HDRI multiplier", &renderOptions.hdrMultiplier, 0.1f, 10.0f);
				ImGui::Separator();
				requiresReload |= ImGui::Checkbox("Enable constant lighting", &renderOptions.useConstantBg);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Creates a constant sourrounding color lighting the scene.");
				optionsChanged |= ImGui::ColorEdit3("Constant color", (float*)bgCol, 0);
				ImGui::Separator();
				ImGui::Checkbox("Enable denoiser", &renderOptions.enableDenoiser);
				ImGui::SliderInt("Denoise on x sample", &renderOptions.denoiserFrameCnt, 1, 250);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Run the denoiser and update the view every x samples.");

				if (requiresReload)
				{
					scene->renderOptions = renderOptions;
					InitRenderer(); //When the options change, restart the render proccess. 
				}

				scene->renderOptions.enableDenoiser = renderOptions.enableDenoiser;
				scene->renderOptions.denoiserFrameCnt = renderOptions.denoiserFrameCnt;
			}

			ImGui::Text("\n");

			if (ImGui::CollapsingHeader("Camera"))
			{
				float fov = Math::Degrees(scene->camera->fov);
				float aperture = scene->camera->aperture * 1000.0f;
				optionsChanged |= ImGui::SliderFloat("Field of vision", &fov, 10, 90);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Field of vision of the camera.");
				scene->camera->SetFov(fov);
				optionsChanged |= ImGui::SliderFloat("Aperture", &aperture, 0.0f, 10.8f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Aperture of the camera. Rule of thumb : The larger this is, the more depth of field you will get. Set to 0 to disable.");
				scene->camera->aperture = aperture / 1000.0f;
				optionsChanged |= ImGui::SliderFloat("Focal Distance", &scene->camera->focalDist, 0.01f, 50.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Focus distance of the camera.");
				ImGui::Text("Pos: %.2f, %.2f, %.2f", scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
			}

		}


		scene->camera->isMoving = false;

		if (optionsChanged)
		{
			scene->renderOptions = renderOptions;
			scene->camera->isMoving = true;
		}

		if (noUi == false) {
			ImGui::Text("\n");

			if (ImGui::CollapsingHeader("Objects"))
			{
				bool objectPropChanged = false;

				std::vector<std::string> listboxItems;
				for (int i = 0; i < scene->meshInstances.size(); i++)
				{
					listboxItems.push_back(scene->meshInstances[i].name);
				}

				// Object Selection
				ImGui::ListBoxHeader("Instances");
				for (int i = 0; i < scene->meshInstances.size(); i++)
				{
					bool is_selected = selectedInstance == i;
					if (ImGui::Selectable(listboxItems[i].c_str(), is_selected))
					{
						selectedInstance = i;
					}
				}
				ImGui::ListBoxFooter();

				ImGui::Separator();
				ImGui::Text("Materials");

				// Material properties
				Vec3* albedo = &scene->materials[scene->meshInstances[selectedInstance].materialID].albedo;
				Vec3* emission = &scene->materials[scene->meshInstances[selectedInstance].materialID].emission;
				Vec3* extinction = &scene->materials[scene->meshInstances[selectedInstance].materialID].extinction;

				objectPropChanged |= ImGui::ColorEdit3("Albedo", (float*)albedo, 0);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Color of the object.");
				objectPropChanged |= ImGui::SliderFloat("Metallic", &scene->materials[scene->meshInstances[selectedInstance].materialID].metallic, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("How metalness of the object.");
				objectPropChanged |= ImGui::SliderFloat("Roughness", &scene->materials[scene->meshInstances[selectedInstance].materialID].roughness, 0.001f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("How smoothness of the object. Setting this to 0 and metallic to max will result in a mirror.");
				objectPropChanged |= ImGui::SliderFloat("Specular", &scene->materials[scene->meshInstances[selectedInstance].materialID].specular, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Intensity of specular highlights.");
				objectPropChanged |= ImGui::SliderFloat("SpecularTint", &scene->materials[scene->meshInstances[selectedInstance].materialID].specularTint, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Tint of the specular highlights.");
				objectPropChanged |= ImGui::SliderFloat("Subsurface", &scene->materials[scene->meshInstances[selectedInstance].materialID].subsurface, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Amount of subsurface scattering on the object.");
				objectPropChanged |= ImGui::SliderFloat("Anisotropic", &scene->materials[scene->meshInstances[selectedInstance].materialID].anisotropic, 0.0f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("Sheen", &scene->materials[scene->meshInstances[selectedInstance].materialID].sheen, 0.0f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("SheenTint", &scene->materials[scene->meshInstances[selectedInstance].materialID].sheenTint, 0.0f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("Clearcoat", &scene->materials[scene->meshInstances[selectedInstance].materialID].clearcoat, 0.0f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("clearcoatRoughness", &scene->materials[scene->meshInstances[selectedInstance].materialID].clearcoatRoughness, 0.001f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("Transmission", &scene->materials[scene->meshInstances[selectedInstance].materialID].transmission, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Amount of transmission/glass-ness. ");
				objectPropChanged |= ImGui::SliderFloat("IOR", &scene->materials[scene->meshInstances[selectedInstance].materialID].ior, 1.001f, 2.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Index of refraction of the object. Only used with transmission.");
				objectPropChanged |= ImGui::ColorEdit3("Extinction", (float*)extinction, 0);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Light extinction of the transmission.");

				// Transform properties
				ImGui::Separator();
				ImGui::Text("Transforms");
				{
					float viewMatrix[16];
					float projMatrix[16];

					auto io = ImGui::GetIO();
					scene->camera->ComputeViewProjectionMatrix(viewMatrix, projMatrix, io.DisplaySize.x / io.DisplaySize.y);
					Mat4 xform = scene->meshInstances[selectedInstance].transform;

					EditTransform(viewMatrix, projMatrix, (float*)&xform);

					if (memcmp(&xform, &scene->meshInstances[selectedInstance].transform, sizeof(float) * 16))
					{
						scene->meshInstances[selectedInstance].transform = xform;
						objectPropChanged = true;
					}
				}

				if (objectPropChanged)
				{
					scene->RebuildInstances();
				}
			}
		}
		ImGui::End();
	}
	double presentTime = SDL_GetTicks();
	Update((float)(presentTime - lastTime));
	lastTime = presentTime;
	glClearColor(0., 0., 0., 0.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //OpenGL clearing.
	glDisable(GL_DEPTH_TEST);
	Render();
	SDL_GL_SwapWindow(loopdata.mWindow);
}

int main(int argc, char** argv)
{
	Log(("--- " + versionString + " ---\n").c_str());

	srand((unsigned int)time(0));

	std::string sceneFile;

	for (int i = 1; i < argc; ++i) 
	{
		const std::string arg(argv[i]);
		switch (strint(arg.c_str())) {
		case strint("--scene"):
			sceneFile = argv[++i];
			break;

		case strint("-s"):
			sceneFile = argv[++i];
			break;

		case strint("-u"):
			noUi = true;
			break;

		case strint("--noui"):
			noUi = true;
			break;

		case strint("-w"):
			noWindow = true;
			break;

		case strint("--nowindow"):
			noWindow = true;
			break;

		case strint("-n"):
			useNeutralTonemap = true;
			break;

		case strint("--neutral"):
			useNeutralTonemap = true;
			break;

		case strint("-ms"):
			std::string::size_type sz0;
			maxSamples = std::stoi(argv[++i], &sz0);
			break;

		case strint("--maxsamples"):
			std::string::size_type sz1;
			maxSamples = std::stoi(argv[++i], &sz1);
			break;

		case strint("-ps"):
			std::string::size_type sz2;
			previewScale = std::stoi(argv[++i], &sz2);
			break;

		case strint("--previewscale"):
			std::string::size_type sz3;
			previewScale = std::stoi(argv[++i], &sz3);
			break;

		case strint("-ep"):
			exportname = argv[++i];
			break;

		case strint("-exportname"):
			exportname = argv[++i];
			break;

		case strint("-dn"):
			renderOptions.enableDenoiser = true;
			break;

		case strint("--denoise"):
			renderOptions.enableDenoiser = true;
			break;

		case strint("-db"):
			useDebug = true;
			break;

		case strint("--debug"):
			useDebug = true;
			break;

		case strint("-df"):
			std::string::size_type sz4;
			renderOptions.denoiserFrameCnt = std::stoi(argv[++i], &sz4);
			break;

		case strint("--denoiseframe"):
			std::string::size_type sz5;
			renderOptions.denoiserFrameCnt = std::stoi(argv[++i], &sz5);
			break;


		case strint("--jpgquality"):
			std::string::size_type sz6;
			currentJpgQuality = std::stoi(argv[++i], &sz6);
			break;

		case strint("-jpgq"):
			std::string::size_type sz7;
			currentJpgQuality = std::stoi(argv[++i], &sz7);
			break;

		default:
			printf("Unknown argument '%s' \n", arg.c_str());
			exit(0);
			break;
		}
	}

	if (!sceneFile.empty())
	{
		scene = new Scene();

		if (!LoadSceneFromFile(sceneFile, scene, renderOptions))
			exit(0);

		scene->renderOptions = renderOptions;
		std::cout << "Scene loaded\n\n";
	}
	else
	{
		GetSceneFiles();
		LoadScene(sceneFiles[sampleSceneIndex]);
	}

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) //Init. SDL2
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	LoopData loopdata;

#ifdef __APPLE__ //Broken apple support
	// GL 3.2 Core + GLSL 150
	const char* glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (noWindow == true) {
		window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
	}
	loopdata.mWindow = SDL_CreateWindow("LavaFrame", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, renderOptions.resolution.x, renderOptions.resolution.y, window_flags);



	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

	loopdata.mGLContext = SDL_GL_CreateContext(loopdata.mWindow);
	if (!loopdata.mGLContext)
	{
		fprintf(stderr, "Failed to initialize OpenGL context!\n");
		return 1;
	}
	SDL_GL_SetSwapInterval(0); // Disable vsync

	// Initialize OpenGL loader
#if GL_VERSION_3_2
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
	bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
	bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
	bool err = gladLoadGL() == 0;
#endif
	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}
#endif

	// Setup Dear ImGui context and style
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontFromFileTTF("fonts/Roboto-Regular.ttf", 16); //Make the font not-eyesore 
	if (useDebug) {
		io.IniFilename = "guiconfig.ini";
	}
	else {
		io.IniFilename = NULL;
	}
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = 0.9f;
	style.WindowRounding = 0;
	style.GrabRounding = 1;
	style.GrabMinSize = 20;
	style.FrameRounding = 3;
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.74f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.01f, 0.01f, 0.01f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 0.45f, 0.00f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.64f, 0.30f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.63f, 0.27f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.57f, 0.28f, 0.00f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.58f, 0.18f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.45f, 0.45f, 0.45f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.17f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 0.61f, 0.24f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.75f, 0.51f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.85f, 0.42f, 0.00f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 0.69f, 0.36f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.31f, 0.15f, 0.00f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.64f, 0.31f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.49f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	
	//End of UI style code.

	ImGui_ImplSDL2_InitForOpenGL(loopdata.mWindow, loopdata.mGLContext);

	ImGui_ImplOpenGL3_Init(glsl_version);

	//ImGui::StyleColorsDark();

	if (!InitRenderer())
		return 1;

	while (!done)
	{
		MainLoop(&loopdata);
	}

	delete renderer;
	delete scene;

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(loopdata.mGLContext);
	SDL_DestroyWindow(loopdata.mWindow);
	SDL_Quit(); //End of application.
	return 0;
}