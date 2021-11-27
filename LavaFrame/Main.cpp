/*
* Hi, and welcome to the LavaFrame source code !
* The goal of this project was to create a (somewhat) production
* ready renderer. I used it as a
* project to build my C++ and GLSL skills further by forking and
* extending another project and building it into something I can
* use for my own projects.
*
* THIS SOURCE CODE CAN BE A BIT MESSY AT TIMES ! PLEASE BEWARE AND FEEL FREE TO IMPROVE ON IT !
*
* - Nolram
*
* MIT License
* Check license.txt for more information on licensing
* Based on the original software by Alif Ali (knightcrawler25)
*/
#define _USE_MATH_DEFINES
#define IMGUI_DEFINE_MATH_OPERATORS

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
#include "GlobalState.h"
#include "Export.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#include "Loader.h"
#include "ImGuizmo.h"
#include "tinydir.h"

using namespace LavaFrame;

double lastTime = SDL_GetTicks();
std::vector<std::string> sceneFiles;

LavaFrameState GlobalState;
RenderOptions renderOptions;

ImVec2 viewportPanelSize;
bool viewportHovered = false;
bool viewportClicked = false;

bool viewport_window_override_size = true;
bool viewport_window_override_pos = true;

bool window_override_size = true;
bool window_override_pos = true;

float viewport_scaler;
bool showDenoise = false;

struct LoopData
{
	SDL_Window* mWindow = nullptr;
	SDL_GLContext mGLContext = nullptr;
};

void GetSceneFiles() // Load and index all scene files in the assets directory.
{
	tinydir_dir dir;
	int i;
	tinydir_open_sorted(&dir, GlobalState.assetsDir.c_str());

	for (i = 0; i < dir.n_files; i++)
	{
		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		if (std::string(file.extension) == "ignition") // Support for old ignition files
		{
			sceneFiles.push_back(GlobalState.assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "scene") // Support for old scene files
		{
			sceneFiles.push_back(GlobalState.assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "lfs") // LavaFrame Scene
		{
			sceneFiles.push_back(GlobalState.assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "lf") // LavaFrame Scene
		{
			sceneFiles.push_back(GlobalState.assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "lavaframe") // Full name file
		{
			sceneFiles.push_back(GlobalState.assetsDir + std::string(file.name));
		}
		if (std::string(file.extension) == "lavaframescene") // Completely typed out filename
		{
			sceneFiles.push_back(GlobalState.assetsDir + std::string(file.name));
		}
	}

	tinydir_close(&dir);
}

void LoadScene(std::string sceneName) // Load scene - this is also called on startup.
{
	delete GlobalState.scene;
	GlobalState.scene = new Scene();
	LoadSceneFromFile(sceneName, GlobalState.scene, renderOptions);
	GlobalState.selectedInstance = 0;
	GlobalState.scene->renderOptions = renderOptions;
}

bool InitRenderer() // Create the tiled renderer and inform the user that the proccess has started.
{
	delete GlobalState.renderer;
	GlobalState.renderer = new TiledRenderer(GlobalState.scene, GlobalState.shadersDir);
	GlobalState.renderer->Init();
	viewportPanelSize.x = GlobalState.scene->renderOptions.resolution.x;
	viewportPanelSize.y = GlobalState.scene->renderOptions.resolution.y;
	if (!GlobalState.threadMode) {
		if (GlobalState.useDebug) printf("Renderer started.\n");
	}
	else {
		printf(("\nLavaFrame thread " + GlobalState.threadID + " started, " + GlobalState.releaseVersion).c_str());
	}

	return true;
}

void Render()
{
	GlobalState.renderer->Render();
	//Viewport Setup

	ImGuiStyle& style = ImGui::GetStyle();

	bool no_titlebar = true;
	bool no_menu = true;
	bool no_move = true;
	bool no_resize = true;
	bool no_collapse = true;

	style.WindowPadding = { 0, 0 };

	if (viewport_window_override_pos and !GlobalState.noUi) {
		ImGui::SetNextWindowPos({ static_cast<float>(GlobalState.displayX / 5), 0 });
	}

	if (viewport_window_override_size and !GlobalState.noUi) {
		ImGui::SetNextWindowSize({ static_cast<float>((GlobalState.displayX / 5) * 3),  static_cast<float>(GlobalState.displayY) });
	}

	if (GlobalState.noUi) {
		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ static_cast<float>(GlobalState.displayX), static_cast<float>(GlobalState.displayY) });
	}

	ImGuiWindowFlags window_flags = 0;
	if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
	if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
	if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
	if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("Viewport", NULL, window_flags);
	{
		viewportPanelSize = ImGui::GetContentRegionAvail();

		viewportPanelSize.x = GlobalState.scene->renderOptions.resolution.x;
		viewportPanelSize.y = GlobalState.scene->renderOptions.resolution.y;

		if (GlobalState.scene->camera->isMoving)
		{
			GlobalState.renderer->Present();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, viewportPanelSize.x, viewportPanelSize.y);
		GlobalState.traceTexture = GlobalState.renderer->SetViewport(viewportPanelSize.x, viewportPanelSize.y);
		if (showDenoise) GlobalState.viewportTexture = GlobalState.denoiseTexture;
		else GlobalState.viewportTexture = GlobalState.traceTexture;

		viewport_scaler = std::min(ImGui::GetContentRegionAvail().x / GlobalState.scene->renderOptions.resolution.x, ImGui::GetContentRegionAvail().y / GlobalState.scene->renderOptions.resolution.y);

		ImGui::SetCursorPos((ImGui::GetContentRegionAvail() * 0.5f) - ((viewportPanelSize * viewport_scaler) * 0.5));
		if (!GlobalState.noUi) ImGui::Image((void*)GlobalState.viewportTexture, { viewportPanelSize.x * viewport_scaler, viewportPanelSize.y * viewport_scaler }, ImVec2(0, 1), ImVec2(1, 0));
		else ImGui::Image((void*)GlobalState.viewportTexture, { viewportPanelSize.x, viewportPanelSize.y}, ImVec2(0, 1), ImVec2(1, 0));
		viewportHovered = ImGui::IsItemHovered();
		viewportClicked = ImGui::IsItemClicked();

		ImGui::End();

		if (!GlobalState.noUi) style.WindowPadding = { static_cast<float>(GlobalState.nativeScreenWidth / 200), static_cast<float>(GlobalState.nativeScreenWidth / 200) };
		else style.WindowPadding = { 0, 0 };
	}

	ImGui::Render();
	ImGui::UpdatePlatformWindows();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Update(float secondsElapsed)
{
	GlobalState.keyPressed = false;
	if (ImGui::IsAnyMouseDown() and !GlobalState.noMove)
	{
		ImVec2 mousePos = ImGui::GetMousePos();
		if (viewportHovered) // Chech if mouse is on viewport TODO(PIXEL): object mouse picking
		{
			if (ImGui::IsMouseDown(0))
			{
				//Rotate Mouse around center
				ImVec2 mouseDelta = ImGui::GetMouseDragDelta(0, 0);
				GlobalState.scene->camera->OffsetOrientation(mouseDelta.x, mouseDelta.y);
				ImGui::ResetMouseDragDelta(0);
			}
			else if (ImGui::IsMouseDown(1))
			{
				//Move mouse the in Z axis
				ImVec2 mouseDelta = ImGui::GetMouseDragDelta(1, 0);
				GlobalState.scene->camera->SetRadius(GlobalState.mouseSensitivity * mouseDelta.y);
				ImGui::ResetMouseDragDelta(1);
			}
			else if (ImGui::IsMouseDown(2))
			{
				//Move mouse the in the XY Plane
				ImVec2 mouseDelta = ImGui::GetMouseDragDelta(2, 0);
				GlobalState.scene->camera->Strafe(GlobalState.mouseSensitivity * mouseDelta.x, GlobalState.mouseSensitivity * mouseDelta.y);
				ImGui::ResetMouseDragDelta(2);
			}
			GlobalState.scene->camera->isMoving = true;
		}
	}

	// Maximum sample auto export
	if (GlobalState.maxSamples + 1 == GlobalState.renderer->GetSampleCount()) {
		if (GlobalState.exportType == "png") {
			if (GlobalState.exportName == "") {
				SaveFrame("./render_" + std::to_string(GlobalState.renderer->GetSampleCount()) + ".png");
			}
			else {
				SaveFrame("./" + GlobalState.exportName + ".png");
			}
		}
		else if (GlobalState.exportType == "jpg") {
			if (GlobalState.exportName == "") {
				SaveFrameJPG("./render_" + std::to_string(GlobalState.renderer->GetSampleCount()) + ".jpg", GlobalState.currentJpgQuality);
			}
			else {
				SaveFrameJPG("./" + GlobalState.exportName + ".jpg", GlobalState.currentJpgQuality);
			}
		}
		else if (GlobalState.exportType == "tga") {
			if (GlobalState.exportName == "") {
				SaveFrameTGA("./render_" + std::to_string(GlobalState.renderer->GetSampleCount()) + ".tga");
			}
			else {
				SaveFrameTGA("./" + GlobalState.exportName + ".tga");
			}
		}
		else if (GlobalState.exportType == "bmp") {
			if (GlobalState.exportName == "") {
				SaveFrameBMP("./render_" + std::to_string(GlobalState.renderer->GetSampleCount()) + ".bmp");
			}
			else {
				SaveFrameBMP("./" + GlobalState.exportName + ".bmp");
			}
		}

		if (!GlobalState.threadMode) { printf("Render finished\n"); }
		else { printf(("\nThread " + GlobalState.threadID + " render finished").c_str()); }
		exit(0);
	}
	GlobalState.renderer->Update(secondsElapsed);
}

void EditTransform(const float* view, const float* projection, float* matrix)
{
	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

	if (ImGui::IsKeyPressed(90))  // Hotkeys for object translation
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

static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void MainLoop(void* arg) // Its the main loop !
{
	LoopData& loopdata = *(LoopData*)arg;
	SDL_GetWindowSize(loopdata.mWindow, &GlobalState.displayX, &GlobalState.displayY);

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT)
		{
			GlobalState.done = true;
		}
		if (event.type == SDL_WINDOWEVENT) // Render resolution gets set to window size
		{
			if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				//GlobalState.scene->renderOptions.resolution = iVec2(event.window.data1, event.window.data2);
				//InitRenderer(); // Restart renderer on window resize.
			}

			if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(loopdata.mWindow))
			{
				GlobalState.done = true;
			}
		}
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(loopdata.mWindow);
	ImGui::NewFrame();
	ImGuizmo::SetOrthographic(false);
	//ImGui::DockSpaceOverViewport();
	ImGuizmo::BeginFrame();
	{
		// io.Fonts->GetTexDataAsAlpha8(); Old font handling
		bool optionsChanged = false;

		if (GlobalState.noUi == false) {
			// Window flags

			bool no_titlebar = true;
			bool no_menu = false;
			bool no_move = true;
			bool no_resize = true;

			ImGuiWindowFlags window_flags = 0;
			if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
			if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
			if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
			if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;

			GlobalState.scene->camera->isMoving = false;

			if (window_override_pos) {
				ImGui::SetNextWindowPos({ 0, 0 });
			}

			if (window_override_size) {
				ImGui::SetNextWindowSize({ static_cast<float>(GlobalState.displayX / 5), static_cast<float>(GlobalState.displayY) });
			}

			ImGui::Begin("Panel1", nullptr, window_flags); //Main panel

			if (ImGui::BeginMenuBar()) // Export menu
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::BeginMenu("Export")) {
						if (ImGui::MenuItem("Export as JPG", "")) {
							if (GlobalState.exportName == "") {
								SaveFrameJPG("./render_" + std::to_string(GlobalState.renderer->GetSampleCount()) + ".jpg", GlobalState.currentJpgQuality);
							}
							else {
								SaveFrameJPG("./" + GlobalState.exportName + ".jpg", 80);
							}
						}
						if (ImGui::MenuItem("Export as PNG", "")) {
							if (GlobalState.exportName == "") {
								SaveFrame("./render_" + std::to_string(GlobalState.renderer->GetSampleCount()) + ".png");
							}
							else {
								SaveFrame("./" + GlobalState.exportName + ".png");
							}
						}
						if (ImGui::MenuItem("Export as TGA", "")) {
							if (GlobalState.exportName == "") {
								SaveFrameTGA("./render_" + std::to_string(GlobalState.renderer->GetSampleCount()) + ".tga");
							}
							else {
								SaveFrameTGA("./" + GlobalState.exportName + ".tga");
							}
						}

						if (ImGui::MenuItem("Export as BMP", "")) {
							if (GlobalState.exportName == "") {
								SaveFrameBMP("./render_" + std::to_string(GlobalState.renderer->GetSampleCount()) + ".bmp");
							}
							else {
								SaveFrameBMP("./" + GlobalState.exportName + ".bmp");
							}
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndMenuBar();

			if (GlobalState.useDebug) { // Some debug stats, --debug / -db
				ImGui::Text("- Debug Mode -");
				ImGui::Text("Debug enabled : %d", GlobalState.useDebug);
				ImGui::Text("Render size : %d x %d", GlobalState.renderer->GetScreenSize().x, GlobalState.renderer->GetScreenSize().y);
				if (ImGui::Button("Reload scenes")) // Button for working on shaders or tonemaps to restart the renderer without a complete application restart.
				{
					sceneFiles.clear();
					GetSceneFiles();
				}
			}

			ImGui::Text("Rendered samples: %d ", GlobalState.renderer->GetSampleCount()); // Sample counter
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Currently rendered samples per pixel.");

			if (GlobalState.useDebug) {
				if (ImGui::Button("Recompile shaders")) // Button for working on shaders to restart the renderer without a complete application restart.
				{
					InitRenderer(); // Recompile shaders and restart the renderer.
				}
				if (ImGui::Button("Relay exportname"))
				{
					if (GlobalState.exportName == "") {
						printf("Empty exportname.\n");
					}
					else {
						printf((GlobalState.exportName + "\n").c_str());
					};
				}
			}

			ImGui::Separator();
			ImGui::Text("\n");

			if (ImGui::CollapsingHeader("System"))
			{
				std::vector<const char*> scenes;
				for (int i = 0; i < sceneFiles.size(); ++i)
				{
					scenes.push_back(sceneFiles[i].c_str());
				}

				if (ImGui::Combo("Active scene", &GlobalState.sampleSceneIndex, scenes.data(), scenes.size(), scenes.size()))
				{
					LoadScene(sceneFiles[GlobalState.sampleSceneIndex]);
					InitRenderer();
				}
				ImGui::InputText("Export filename", &GlobalState.exportName);
				ImGui::SliderInt("JPG export quality", &GlobalState.currentJpgQuality, 1, 200);
				optionsChanged |= ImGui::SliderFloat("Mouse Sensitivity", &GlobalState.mouseSensitivity, 0.001f, 1.0f);
				ImGui::Text("\n");
			}

			bool requiresReload = false;

			if (ImGui::CollapsingHeader("Render Settings"))
			{
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
					ImGui::SetTooltip("Creates a constant surrounding color lighting the scene.");
				optionsChanged |= ImGui::ColorEdit3("Constant color", (float*)bgCol, 0);
				ImGui::Separator();
				ImGui::Checkbox("Use ACES tonemapping", &renderOptions.useAces);

				GlobalState.scene->renderOptions.denoiserFrameCnt = renderOptions.denoiserFrameCnt;
				GlobalState.scene->renderOptions.useAces = renderOptions.useAces;
			}

			if (ImGui::CollapsingHeader("Preview Settings")) {
				// For integrating more preview engines in the future
				ImGui::Combo("Preview Engine", &GlobalState.previewEngineIndex, "Flareon\0", 2);
				ImGui::SameLine(); HelpMarker(
					"Selects the preview engine to use.\nFlareon is an interactive viewport engine based on the same\npath-tracing backend as the LavaFrame renderer.");

				if (GlobalState.previewEngineIndex == 0) requiresReload |= ImGui::InputFloat("Preview Scale", &GlobalState.previewScale, 0.1, 0.25);
				if (GlobalState.previewScale > 2.0) GlobalState.previewScale = 2.0;
				if (GlobalState.previewScale < 0.1) GlobalState.previewScale = 0.1;

				if (GlobalState.previewEngineIndex == 0) requiresReload |= ImGui::Checkbox("Use Aperture in Preview", &GlobalState.useDofInPreview);
			}

			if (ImGui::CollapsingHeader("Denoiser")) {
				if (ImGui::Button("Run denoise")) GlobalState.denoiseTexture = GlobalState.renderer->Denoise();
				ImGui::Checkbox("Show denoise", &showDenoise);
				ImGui::Separator();
				ImGui::Checkbox("Enable automatic denoise", &GlobalState.scene->renderOptions.enableAutomaticDenoise);
				ImGui::InputInt("Denoise on x sample", &renderOptions.denoiserFrameCnt, 10, 50);
				if (renderOptions.denoiserFrameCnt < 1) renderOptions.denoiserFrameCnt = 1;
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Run the denoiser and update the view every x samples.");
			}

			if (ImGui::CollapsingHeader("Effects")) {
				ImGui::Checkbox("Chromatic Abberation", &renderOptions.useCA);
				if (renderOptions.useCA) {
					ImGui::Checkbox("Use CA distortion", &renderOptions.useCADistortion);
					ImGui::SliderFloat("CA Distance", &renderOptions.caDistance, 0, 5);
					if (renderOptions.useCADistortion) ImGui::SliderFloat("CA Angularity", &renderOptions.caP1, 0, 10);
					if (renderOptions.useCADistortion) ImGui::SliderFloat("CA Center", &renderOptions.caP3, -1, 1);
					ImGui::SliderFloat("CA Directionality", &renderOptions.caP2, -1, 1);
				}
			}
			GlobalState.scene->renderOptions.useCA = renderOptions.useCA;
			GlobalState.scene->renderOptions.useCADistortion = renderOptions.useCADistortion;
			GlobalState.scene->renderOptions.caDistance = renderOptions.caDistance;
			GlobalState.scene->renderOptions.caP1 = renderOptions.caP1;
			GlobalState.scene->renderOptions.caP2 = renderOptions.caP2;
			GlobalState.scene->renderOptions.caP3 = renderOptions.caP3;

			if (requiresReload)
			{
				GlobalState.scene->renderOptions = renderOptions;
				InitRenderer(); // When the options change, restart the render proccess.
			}

			ImGui::End();

			// PANEL 2
			if (window_override_pos) {
				ImGui::SetNextWindowPos({ (static_cast<float>(GlobalState.displayX) / 5) * 4, 0 });
			};

			if (window_override_size) {
				ImGui::SetNextWindowSize({ static_cast<float>(GlobalState.displayX / 5), static_cast<float>(GlobalState.displayY) });
			}

			ImGui::Begin("Panel2", nullptr, window_flags);

			if (ImGui::CollapsingHeader("Camera"))
			{
				float fov = Math::Degrees(GlobalState.scene->camera->fov);
				float aperture = GlobalState.scene->camera->aperture * 1000.0f;
				optionsChanged |= ImGui::SliderFloat("Field of vision", &fov, 10, 90);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Field of vision of the camera.");
				GlobalState.scene->camera->SetFov(fov);
				optionsChanged |= ImGui::SliderFloat("Aperture", &aperture, 0.0f, 10.8f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Aperture of the camera.");
				GlobalState.scene->camera->aperture = aperture / 1000.0f;
				optionsChanged |= ImGui::SliderFloat("Focal Distance", &GlobalState.scene->camera->focalDist, 0.1f, 50.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Focus distance of the camera.");
				ImGui::Text("Pos: %.2f, %.2f, %.2f", GlobalState.scene->camera->position.x, GlobalState.scene->camera->position.y, GlobalState.scene->camera->position.z);
			}

			if (optionsChanged)
			{
				GlobalState.scene->renderOptions = renderOptions;
				GlobalState.scene->camera->isMoving = true;
			}

			if (ImGui::CollapsingHeader("Objects"))
			{
				bool objectPropChanged = false;

				std::vector<std::string> listboxItems;
				for (int i = 0; i < GlobalState.scene->meshInstances.size(); i++)
				{
					listboxItems.push_back(GlobalState.scene->meshInstances[i].name);
				}

				// Object Selection
				ImGui::ListBoxHeader("Instances");
				for (int i = 0; i < GlobalState.scene->meshInstances.size(); i++)
				{
					bool is_selected = GlobalState.selectedInstance == i;
					if (ImGui::Selectable(listboxItems[i].c_str(), is_selected))
					{
						GlobalState.selectedInstance = i;
					}
				}
				ImGui::ListBoxFooter();

				ImGui::Separator();
				ImGui::Text("Materials");

				// Material properties
				Vec3* albedo = &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].albedo;
				Vec3* emission = &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].emission;
				Vec3* extinction = &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].extinction;

				objectPropChanged |= ImGui::ColorEdit3("Albedo", (float*)albedo, 0);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Color of the object.");
				objectPropChanged |= ImGui::SliderFloat("Metallic", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].metallic, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("How metalness of the object.");
				objectPropChanged |= ImGui::SliderFloat("Roughness", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].roughness, 0.001f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("How smoothness of the object. Setting this to 0 and metallic to max will result in a mirror.");
				objectPropChanged |= ImGui::SliderFloat("Specular", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].specular, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Intensity of specular highlights.");
				objectPropChanged |= ImGui::SliderFloat("SpecularTint", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].specularTint, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Tint of the specular highlights.");
				objectPropChanged |= ImGui::SliderFloat("Subsurface", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].subsurface, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Amount of subsurface scattering on the object.");
				objectPropChanged |= ImGui::SliderFloat("Anisotropic", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].anisotropic, 0.0f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("Sheen", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].sheen, 0.0f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("SheenTint", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].sheenTint, 0.0f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("Clearcoat", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].clearcoat, 0.0f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("clearcoatRoughness", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].clearcoatRoughness, 0.001f, 1.0f);
				objectPropChanged |= ImGui::SliderFloat("Transmission", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].transmission, 0.0f, 1.0f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Amount of transmission/glass-ness. ");
				objectPropChanged |= ImGui::SliderFloat("IOR", &GlobalState.scene->materials[GlobalState.scene->meshInstances[GlobalState.selectedInstance].materialID].ior, 1.001f, 2.0f);
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
					GlobalState.scene->camera->ComputeViewProjectionMatrix(viewMatrix, projMatrix, io.DisplaySize.x / io.DisplaySize.y);
					Mat4 xform = GlobalState.scene->meshInstances[GlobalState.selectedInstance].transform;

					EditTransform(viewMatrix, projMatrix, (float*)&xform);

					if (memcmp(&xform, &GlobalState.scene->meshInstances[GlobalState.selectedInstance].transform, sizeof(float) * 16))
					{
						GlobalState.scene->meshInstances[GlobalState.selectedInstance].transform = xform;
						objectPropChanged = true;
					}
				}

				if (objectPropChanged)
				{
					GlobalState.scene->RebuildInstances();
				}
			}
			ImGui::End();
		}
		if (GlobalState.noUi == true) {
			GlobalState.scene->camera->isMoving = false;
		}
		if (GlobalState.noUi == true and GlobalState.displaySampleCounter == true) {
			// Window flags
			static bool no_titlebar = true;
			static bool no_menu = true;
			static bool no_move = false;
			static bool no_resize = true;
			static bool window_override_size = false;

			ImVec4* colors = ImGui::GetStyle().Colors;
			colors[ImGuiCol_Border] = ImVec4(0.01f, 0.01f, 0.01f, 0.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.1f);

			ImGuiWindowFlags window_flags_samplecounter = 0;
			if (no_titlebar)        window_flags_samplecounter |= ImGuiWindowFlags_NoTitleBar;
			if (!no_menu)           window_flags_samplecounter |= ImGuiWindowFlags_MenuBar;
			if (no_move)            window_flags_samplecounter |= ImGuiWindowFlags_NoMove;
			if (no_resize)          window_flags_samplecounter |= ImGuiWindowFlags_NoResize;

			GlobalState.scene->camera->isMoving = false;

			ImGui::SetNextWindowSize({ 340, 64 });
			ImGui::SetNextWindowPos({ 0, 1 });
			ImGui::Begin("samplecount", nullptr, window_flags_samplecounter);
			ImGui::Text("Rendered samples: %d ", GlobalState.renderer->GetSampleCount());
			ImGui::End();
		}
	}

	double presentTime = SDL_GetTicks();
	Update((float)(presentTime - lastTime));
	lastTime = presentTime;
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // OpenGL clearing.
	glDisable(GL_DEPTH_TEST);
	Render();
	SDL_GL_SwapWindow(loopdata.mWindow);
}

int main(int argc, char** argv)
{
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
			GlobalState.noUi = true;
			break;

		case strint("--noui"):
			GlobalState.noUi = true;
			break;

		case strint("-nm"):
			GlobalState.noMove = true;
			break;

		case strint("--nomove"):
			GlobalState.noMove = true;
			break;

		case strint("-w"):
			GlobalState.noWindow = true;
			break;

		case strint("--nowindow"):
			GlobalState.noWindow = true;
			break;

		case strint("-n"):
			renderOptions.useAces = false;
			break;

		case strint("--neutral"):
			renderOptions.useAces = false;
			break;

		case strint("-ms"):
			std::string::size_type sz;
			GlobalState.maxSamples = std::stoi(argv[++i], &sz);
			break;

		case strint("--maxsamples"):
		{
			std::string::size_type sz;
			GlobalState.maxSamples = std::stoi(argv[++i], &sz);
			break;
		}

		case strint("-ps"):
		{
			std::string::size_type sz;
			GlobalState.previewScale = std::stoi(argv[++i], &sz);
			break;
		}

		case strint("--previewscale"):
		{
			std::string::size_type sz;
			GlobalState.previewScale = std::stoi(argv[++i], &sz);
			break;
		}

		case strint("-ou"):
			GlobalState.output_path = argv[++i];
			break;

		case strint("--outputpath"):
			GlobalState.output_path = argv[++i];
			break;

		case strint("-ep"):
			GlobalState.exportName = argv[++i];
			break;

		case strint("-exportname"):
			GlobalState.exportName = argv[++i];
			break;

		case strint("-et"):
			GlobalState.exportType = argv[++i];
			break;

		case strint("--exporttype"):
			GlobalState.exportType = argv[++i];
			break;

		case strint("-dn"):
			renderOptions.enableAutomaticDenoise = true;
			break;

		case strint("--denoise"):
			renderOptions.enableAutomaticDenoise = true;
			break;

		case strint("-db"):
			GlobalState.useDebug = true;
			break;

		case strint(".lt-threadmode"):
		{
			GlobalState.threadMode = true;
			GlobalState.threadID = argv[++i];
			break;
		}

		case strint("-sc"):
			GlobalState.displaySampleCounter = true;
			break;

		case strint("--samplecounter"):
			GlobalState.displaySampleCounter = true;
			break;

		case strint("--debug"):
			GlobalState.useDebug = true;
			break;

		case strint("-df"):
		{
			std::string::size_type sz;
			renderOptions.denoiserFrameCnt = std::stoi(argv[++i], &sz);
			break;
		}

		case strint("--denoiseframe"):
		{
			std::string::size_type sz;
			renderOptions.denoiserFrameCnt = std::stoi(argv[++i], &sz);
			break;
		}

		case strint("--jpgquality"):
		{
			std::string::size_type sz;
			GlobalState.currentJpgQuality = std::stoi(argv[++i], &sz);
			break;
		}

		case strint("-jpgq"):
		{
			std::string::size_type sz;
			GlobalState.currentJpgQuality = std::stoi(argv[++i], &sz);
			break;
		}

		default:
			printf("Unknown argument '%s' \n", arg.c_str());
			exit(0);
			break;
		}
	}

	if (!GlobalState.threadMode) { Log(("--- " + GlobalState.versionString + " ---\n").c_str()); }

	if (!sceneFile.empty())
	{
		GlobalState.scene = new Scene();

		if (!LoadSceneFromFile(sceneFile, GlobalState.scene, renderOptions))
			exit(0);

		GlobalState.scene->renderOptions = renderOptions;
		if (!GlobalState.threadMode) { std::cout << "Scene loaded\n"; }
	}
	else
	{
		GetSceneFiles();
		LoadScene(sceneFiles[GlobalState.sampleSceneIndex]);
	}

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) // Init. SDL2
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	LoopData loopdata;

#ifdef __APPLE__ // Broken apple support
	// GL 3.2 Core + GLSL 150
	const char* glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 330
	const char* glsl_version = "#version 330";
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
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
	if (GlobalState.noUi == true) {
		window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
	}
	if (GlobalState.noWindow == true) {
		window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
	}

	SDL_DisplayMode sdldisplaymode;
	SDL_GetDesktopDisplayMode(0, &sdldisplaymode);
	GlobalState.nativeScreenWidth = sdldisplaymode.w;
	GlobalState.nativeScreenHeight = sdldisplaymode.h;

	SDL_Rect displayBounds;
	SDL_GetDisplayUsableBounds(0, &displayBounds);
	int screenWidth = displayBounds.w;
	int screenHeight = displayBounds.h;

	if (!GlobalState.noUi) loopdata.mWindow = SDL_CreateWindow(GlobalState.versionString.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, window_flags);
	else loopdata.mWindow = SDL_CreateWindow(GlobalState.versionString.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GlobalState.scene->renderOptions.resolution.x, GlobalState.scene->renderOptions.resolution.y, window_flags);

	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

	loopdata.mGLContext = SDL_GL_CreateContext(loopdata.mWindow);
	if (!loopdata.mGLContext)
	{
		fprintf(stderr, "Failed to initialize OpenGL context!\n");
		return 1;
	}
	SDL_GL_SetSwapInterval(0); // Disable vsync

	// Initialize OpenGL loader
	bool err = gl3wInit() != 0;
	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	// Setup Dear ImGui context and style
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontFromFileTTF("fonts/Roboto-Regular.ttf", GlobalState.nativeScreenWidth / 121); //Make the font not-eyesore
	if (GlobalState.useDebug) {
		io.IniFilename = "guiconfig.ini";
	}
	else {
		io.IniFilename = NULL;
	}
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		  // Enable DOCKING

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = ImGui::GetStyle().Colors;

	style.Alpha = 1.0f;
	style.WindowRounding = 0;
	style.GrabRounding = 1;
	style.GrabMinSize = 20;
	style.FrameRounding = 2.5;
	style.FramePadding = { 4, 4 };

#define THEME_NONE ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
#define THEME_COLOR_WHITE ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
#define THEME_COLOR_GRAY ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
#define THEME_BACKGROUND_DARK ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
#define THEME_BACKGROUND_DARKER ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
#define THEME_BACKGROUND_DARKER_TRANSPARENT ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
#define THEME_BACKGROUND_DARK_TRANSPARENT ImVec4(0.0f, 0.0f, 0.0f, 0.25f);
#define THEME_BACKGROUND_LIGHT ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
#define THEME_COLOR_ORANGE ImVec4(1.00f, 0.42f, 0.00f, 1.00f);
#define THEME_SUBDUED_TRANSPARENT ImVec4(0.00f, 0.00f, 0.00f, 0.5f);
#define THEME_ENLIGHTEN_TRANSPARENT ImVec4(0.43f, 0.43f, 0.43f, 0.50f);

	colors[ImGuiCol_Text] = THEME_COLOR_WHITE;
	colors[ImGuiCol_TextDisabled] = THEME_COLOR_GRAY;
	colors[ImGuiCol_WindowBg] = THEME_BACKGROUND_DARK;
	colors[ImGuiCol_ChildBg] = THEME_BACKGROUND_DARK_TRANSPARENT;
	colors[ImGuiCol_PopupBg] = THEME_BACKGROUND_DARKER_TRANSPARENT;
	colors[ImGuiCol_Border] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_BorderShadow] = THEME_NONE;
	colors[ImGuiCol_FrameBg] = THEME_BACKGROUND_LIGHT;
	colors[ImGuiCol_FrameBgHovered] = THEME_BACKGROUND_LIGHT;
	colors[ImGuiCol_FrameBgActive] = THEME_BACKGROUND_LIGHT;
	colors[ImGuiCol_TitleBg] = THEME_NONE;
	colors[ImGuiCol_TitleBgActive] = THEME_NONE;
	colors[ImGuiCol_TitleBgCollapsed] = THEME_NONE;
	colors[ImGuiCol_MenuBarBg] = THEME_BACKGROUND_DARKER;
	colors[ImGuiCol_ScrollbarBg] = THEME_BACKGROUND_DARKER_TRANSPARENT;
	colors[ImGuiCol_ScrollbarGrab] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_ScrollbarGrabHovered] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_ScrollbarGrabActive] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_CheckMark] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_SliderGrab] = THEME_BACKGROUND_DARKER_TRANSPARENT;
	colors[ImGuiCol_SliderGrabActive] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_Button] = THEME_SUBDUED_TRANSPARENT;
	colors[ImGuiCol_ButtonHovered] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_ButtonActive] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_Header] = THEME_SUBDUED_TRANSPARENT;
	colors[ImGuiCol_HeaderHovered] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_HeaderActive] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_Separator] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_SeparatorHovered] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_SeparatorActive] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_ResizeGrip] = THEME_NONE;
	colors[ImGuiCol_ResizeGripHovered] = THEME_NONE;
	colors[ImGuiCol_ResizeGripActive] = THEME_NONE;
	colors[ImGuiCol_Tab] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_TabHovered] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_TabActive] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_TabUnfocused] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_TabUnfocusedActive] = THEME_ENLIGHTEN_TRANSPARENT;
	colors[ImGuiCol_TableHeaderBg] = THEME_BACKGROUND_DARKER;
	colors[ImGuiCol_TableBorderStrong] = THEME_BACKGROUND_DARKER;
	colors[ImGuiCol_TableBorderLight] = THEME_BACKGROUND_LIGHT;
	colors[ImGuiCol_TableRowBg] = THEME_BACKGROUND_DARK;
	colors[ImGuiCol_TableRowBgAlt] = THEME_BACKGROUND_DARK;
	colors[ImGuiCol_TextSelectedBg] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_DragDropTarget] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_NavHighlight] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_NavWindowingHighlight] = THEME_COLOR_ORANGE;
	colors[ImGuiCol_NavWindowingDimBg] = THEME_BACKGROUND_DARK_TRANSPARENT;
	colors[ImGuiCol_ModalWindowDimBg] = THEME_BACKGROUND_DARK_TRANSPARENT;

	ImGui_ImplSDL2_InitForOpenGL(loopdata.mWindow, loopdata.mGLContext);
	ImGui_ImplOpenGL3_Init(glsl_version);

	if (!InitRenderer())
		return 1;

	while (!GlobalState.done)
	{
		MainLoop(&loopdata);
	}

	delete GlobalState.renderer;
	delete GlobalState.scene;

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(loopdata.mGLContext);
	SDL_DestroyWindow(loopdata.mWindow);
	SDL_Quit(); //End of application.
	return 0;
}