#include "main.hpp"
#include "include.h"
#include "esp.hpp"
#include "silentaim.hpp"
#include "memory.h"
#include "offsets.h"
#include "exploit.h"
#include "xor.hpp"
#include <Windows.h>
#include "auth.hpp"
#include <string>
#include "skStr.h"
#include <stdio.h>
#include <urlmon.h>
#include "color.hpp"
#include "colors.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "image.h"
#include "user_interface.h"
#include "imgui/imgui_impl_win32.h"
#include "trigger.hpp"
#include "elements.h"
#include <thread>
#include <chrono>
#include <ctime>
#include <random>
#include <vector>

// DEFINICJE GLOBALNYCH (zadeklarowane extern w main.hpp)
OverlayState g_overlay;
GameState g_game;
namespace fonts { Collection g_fonts; }

// GLOBALNE: tylko jedna definicja running i reallock_thread
std::atomic<bool> running{ true };
std::thread reallock_thread;

std::string tm_to_readable_time(std::tm ctx);
static std::time_t string_to_timet(std::string timestamp);
static std::tm timet_to_tm(std::time_t timestamp);
const std::string compilation_date = (std::string)skCrypt(__DATE__);
const std::string compilation_time = (std::string)skCrypt(__TIME__);

using namespace KeyAuth;

auto name = skCrypt("bobx.fun");
auto ownerid = skCrypt("dlTED3SFir");
auto secret = skCrypt("16e62034e0f19ff3743cc1667d926800d6d741b5551b1bcaf6d73f3903b8f937");
auto version = skCrypt("1.0");

auto url = skCrypt("https://keyauth.win/api/1.2/");

api KeyAuthApp(name.decrypt(), ownerid.decrypt(), secret.decrypt(), version.decrypt(), url.decrypt());

bool visual = true;
bool aimbot = true;

// Silent Aim configuration definitions
namespace silentaim_config {
    bool on = false;
    bool fov = false;
    int bone = 0;
    float fov_value = 45.0f;
    float max_distance = 200.0f;
    float angle_threshold = 5.0f;           // BEZPIECZNY threshold <= 5.0°
    bool enable_facing_check = true;
    float facing_threshold = 45.0f;
    bool rage_mode = false;                  // ZAWSZE false - AC protection
    int bind = 0;
}

enum heads {
	rage, triggerbot, antiaim, visuals, settings, friendlist
};

enum sub_heads {
	general, accuracy, exploits, _general, advanced
};

void panel();

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {

	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return true;
}

void FovSilentAim() {
    int screen_x = GetSystemMetrics(SM_CXSCREEN);
    int screen_y = GetSystemMetrics(SM_CYSCREEN);
    auto draw = ImGui::GetBackgroundDrawList();
    draw->AddCircle(ImVec2(screen_x / 2, screen_y / 2), silentaim_config::fov_value, IM_COL32(233, 232, 232, 255), fov_segments, 2.0f);
}

void GPlayerList() {
    if (!ReplayInterface_ptr) return;
    uintptr_t PedReplayInterface = read_mem<uintptr_t>(ReplayInterface_ptr + 0x18);
    if (!PedReplayInterface) return;
    uintptr_t PedList = read_mem<uintptr_t>(PedReplayInterface + 0x100);
    // CACHE local once to avoid race condition
    uintptr_t local = GetLocalPlayerSafe();
    for (int i = 0; i < 256; i++) {
        if (PedList) {
            uintptr_t Ped = read_mem<uintptr_t>(PedList + (i * 0x10));
            if (Ped) {
                if (local != 0 && Ped != local) {
                    player_contry[i] = Ped;
                }
            }
        }
    }
}

void PlayerList() {
	GPlayerList();
	for (int i = 0; i < 256; i++) {
		if (player_contry[i] != 0) {
			std::string layer = std::to_string(i);
			if (player_friend[i] == 0) {
				if (ImGui::Button((skCrypt("Add Friend ID: ").decrypt() + layer).c_str(), { 199, 22 })) {
					player_friend[i] = player_contry[i];
				}
			}
			else {
				const std::string label = skCrypt("Rem Friend ID: ").decrypt() + layer;
				if (ImGui::Button(label.c_str(), { 199, 22 })) {
					player_friend[i] = 0;
				}
			}
		}
	}
}

void loginpanel();

bool god;
bool nspeed;
bool nrec;
bool nreload;
bool npread;
bool VehSp;
bool VehBrk;
bool VehGod;
int tabb = 0;

const char* menu_bind[] = { ("F4"), ("F8"), ("Delete"), ("Insert") };
const char* aimbot_bind[] = { ("Right Button") ,("Left Button") , ("Page Backward Button"), ("Page Forward Button"),("Shift"),("Menu"),("E") };
const char* trigger_bind[] = { ("Right Button") ,("Left Button"), ("Page Backward Button"), ("Page Forward Button"),("Shift"),("Menu"),("E") };
const char* silentaim_bind[] = { "Right Button", "Left Button", "Page Backward Button", "Page Forward Button", "Shift", "Menu", "E" };
static int selected = 0;
static int sub_selected = 0;

static heads tab{ rage };
static sub_heads subtab{ general };

void Render() {

	glfwPollEvents();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (bind_menu == 0) { bind_menuVK = VK_F4; }
	else if (bind_menu == 1) { bind_menuVK = VK_F8; }
	else if (bind_menu == 2) { bind_menuVK = VK_DELETE; }
	else if (bind_menu == 3) { bind_menuVK = VK_INSERT; }

	SAFE_EXECUTE({
		if (silentaim_config.fov) { FovSilentAim(); }

		if (esp_on) { DrawEsp(); }

		if (silentaim_config.on) { 
			SetSilentAimBind(silentaim_config.bind); 
			SilentAim(world_ptr, GetLocalPlayerSafe()); 
		}

		if (trigger_on) {
			bindtrigger();
			if (trigger_type == 0 || trigger_type == 1) {
				RageTriggerbot();
			}
			else if (trigger_type == 2) {
				LegitTriggerbot();
			}
		}

		// Usunięto duplikat wywołania SpeedWalk (została jedna sekcja)
		if (ex_runspeed_on) { Player::SpeedWalk(true, ex_runspeed_value); nspeed = true; }
		else { if (nspeed) { Player::SpeedWalk(false, 0); } nspeed = false; }

		if (ex_norecoil) { Weapons::NoRecoil(true); nrec = true; }
		else { if (nrec) { Weapons::NoRecoil(false); } nrec = false; }

		if (ex_noreaload) { Weapons::NoReaload(true); nreload = true; }
		else { if (nreload) { Weapons::NoReaload(false); } nreload = false; }

		if (ex_nospread) { Weapons::NoSpread(true); npread = true; }
		else { if (npread) { Weapons::NoSpread(false); } npread = false; }

		if (ex_godmode) { Player::Godmode(true); god = true; }
		else { if (god) { Player::Godmode(false); } god = false; }

		if (ex_semigodmode) {
			if (Player::GetHealth() <= 200) {
				Player::SetHealth(Player::GetMaxHealth());
			}
		}

		if (ex_vehicleaceleration) { Vehicle::VehicleAceleration(true, ex_vehicleaceleration_value); VehSp = true; }
		else { if (VehSp) { Vehicle::VehicleAceleration(false, ex_vehicleaceleration_value); } VehSp = false; }

		if (ex_vehiclebreak) { Vehicle::VehicleBrakeForce(true, ex_vehiclebreak_value); VehBrk = true; }
		else { if (VehBrk) { Vehicle::VehicleBrakeForce(false, ex_vehiclebreak_value); } VehBrk = false; }

		if (ex_vehiclegodmode) { Vehicle::GodModeVehicle(true); VehGod = true; }
		else { if (VehGod) { Vehicle::GodModeVehicle(false); } VehGod = false; }
	});

	if (menu_visible)
	{

		ImGui::SetNextWindowBgAlpha(0.98f);
		ImGui::SetNextWindowSize(ImVec2(860, 540));
		ImGui::Begin(skCrypt("BOBX.FUN"), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

		auto draw = ImGui::GetWindowDrawList();
		auto win_pos = ImGui::GetWindowPos();
		auto win_size = ImGui::GetWindowSize();

		draw->AddRectFilledMultiColor(
			win_pos,
			ImVec2(win_pos.x + win_size.x, win_pos.y + win_size.y),
			ImGui::GetColorU32(ImVec4(0.07f, 0.07f, 0.16f, 0.98f)),
			ImGui::GetColorU32(ImVec4(0.11f, 0.07f, 0.20f, 0.98f)),
			ImGui::GetColorU32(ImVec4(0.09f, 0.09f, 0.18f, 0.98f)),
			ImGui::GetColorU32(ImVec4(0.05f, 0.05f, 0.13f, 0.98f))
		);

		ImGui::BeginChild("##Sidebar", ImVec2(240, 0), true, ImGuiWindowFlags_NoScrollbar);

		ImGui::Dummy(ImVec2(0, 20));

		ImGui::PushFont(fonts::semibold);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.80f, 1.00f, 1.00f));
		ImGui::SetCursorPosX((240 - ImGui::CalcTextSize("BOBX.FUN").x) * 0.5f);
		ImGui::Text("BOBX.FUN");
		ImGui::PopStyleColor();
		ImGui::PopFont();

		ImGui::Dummy(ImVec2(0, 30));

		static const char* tab_labels[] = { "Rage", "Visuals", "Trigger", "Exploits", "Safelist", "Config" };
		static const char* tab_icons[] = { "B", "D", "F", "C", "G", "E" };

		for (int i = 0; i < IM_ARRAYSIZE(tab_labels); i++)
		{
			bool is_selected = ((int)tab == i);

			ImGui::PushStyleColor(ImGuiCol_Text, is_selected ? ImVec4(0.95f, 0.95f, 1.0f, 1.0f) : ImVec4(0.65f, 0.65f, 0.85f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.20f, 0.36f, is_selected ? 0.75f : 0.00f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.28f, 0.28f, 0.48f, 0.80f));

			std::string label = std::string(tab_icons[i]) + "   " + tab_labels[i];

			if (ImGui::Selectable(label.c_str(), is_selected, 0, ImVec2(0, 44)))
				tab = (heads)i;

			ImGui::PopStyleColor(3);
		}

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("##MainContent", ImVec2(0, 0), true);

		ImGui::PushFont(fonts::semibold);
		ImGui::TextColored(ImVec4(0.85f, 0.88f, 1.00f, 1.00f), "%s", tab_labels[(int)tab]);
		ImGui::PopFont();

		ImGui::Separator();
		ImGui::Dummy(ImVec2(0, 12));

		switch (tab)
		{
		case rage:
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Silent Aim Features");
			ImGui::Separator();
			ImGui::Checkbox("Enable Silent Aim", &silentaim_config::on);
			ImGui::Checkbox("Draw FOV Circle", &silentaim_config::fov);
			ImGui::Checkbox("Enable Facing Check", &silentaim_config::enable_facing_check);
			// WYŁĄCZONE - rage mode jest zawsze wykrywany przez anticheat
			// ImGui::Checkbox("Rage Mode", &silentaim_config::rage_mode);
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Rage Mode: DISABLED (AC Protection)");

			ImGui::Dummy(ImVec2(0, 16));
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Silent Aim Tuning");
			ImGui::Separator();
			ImGui::Combo("Target Bone", &silentaim_config::bone, hitbox_items, IM_ARRAYSIZE(hitbox_items));
			ImGui::SliderFloat("Max Distance", &silentaim_config::max_distance, 20.f, 500.f, "%.2f");
			ImGui::SliderFloat("FOV Size", &silentaim_config::fov_value, 20.f, 400.f, "%.2f");
			ImGui::SliderFloat("Angle Threshold", &silentaim_config::angle_threshold, 1.f, 5.f, "%.2f");
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "WARNING: >5.0 = AC Detection!");
			ImGui::SliderFloat("Facing Threshold", &silentaim_config::facing_threshold, 10.f, 90.f, "%.2f");
			ImGui::SliderInt("FOV Segments", &fov_segments, 1, 50);
			break;

		case visuals:
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "ESP Options");
			ImGui::Separator();
			ImGui::Checkbox("Box", &esp_box);
			ImGui::Checkbox("Corner Box", &esp_corner);
			ImGui::Checkbox("Skeleton", &esp_skeletons);
			ImGui::Checkbox("Distance", &esp_showdistance);
			ImGui::Checkbox("Health Text", &esp_hptext);
			ImGui::Checkbox("Snaplines", &esp_lines);
			ImGui::Checkbox("Show NPCs", &esp_drawnpcs);
			ImGui::Checkbox("Filled Box", &filled_box);
			ImGui::Checkbox("Health Bar", &esp_hpbar);
			ImGui::Checkbox("Head Dot", &esp_head);
			ImGui::Checkbox("Player ID", &esp_showid);
			ImGui::SliderFloat("ESP Render Distance", &esp_max_distance, 20, 500, "%.2f");

			ImGui::Dummy(ImVec2(0, 16));
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Colors");
			ImGui::Separator();
			ImGui::ColorEdit4("Box Color", Temp::Box, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			Colors::ESPBox.Value = ImVec4(Temp::Box[0], Temp::Box[1], Temp::Box[2], Temp::Box[3]);

			ImGui::ColorEdit4("Corner Color", Temp::Corner, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			Colors::ESPCorner.Value = ImVec4(Temp::Corner[0], Temp::Corner[1], Temp::Corner[2], Temp::Corner[3]);

			ImGui::ColorEdit4("Skeleton Color", Temp::Skeleton, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			Colors::ESPSkeleton.Value = ImVec4(Temp::Skeleton[0], Temp::Skeleton[1], Temp::Skeleton[2], Temp::Skeleton[3]);

			ImGui::ColorEdit4("Distance Text Color", Temp::Distance, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			Colors::ESPDistance.Value = ImVec4(Temp::Distance[0], Temp::Distance[1], Temp::Distance[2], Temp::Distance[3]);

			ImGui::ColorEdit4("Health Text Color", Temp::HpText, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			Colors::ESPHpText.Value = ImVec4(Temp::HpText[0], Temp::HpText[1], Temp::HpText[2], Temp::HpText[3]);

			ImGui::ColorEdit4("Snapline Color", Temp::Lines, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
			Colors::ESPLines.Value = ImVec4(Temp::Lines[0], Temp::Lines[1], Temp::Lines[2], Temp::Lines[3]);
			break;

		case triggerbot:
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Trigger Features");
			ImGui::Separator();
			ImGui::Checkbox("Enable TriggerBot", &trigger_on);
			ImGui::Checkbox("Trigger FOV Check", &trigger_fov);

			ImGui::Dummy(ImVec2(0, 16));
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Trigger Settings");
			ImGui::Separator();
			ImGui::SliderFloat("Trigger FOV", &trigger_fov_value, 0.f, 50.f, "%.2f");
			ImGui::SliderFloat("Max Distance", &aimbot_max_distance, 20.f, 500.f, "%.2f");
			break;

		case antiaim:
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Cheat Features");
			ImGui::Separator();
			ImGui::Checkbox("Godmode", &ex_godmode);
			ImGui::Checkbox("Semi-Godmode", &ex_semigodmode);
			ImGui::Checkbox("Speed Hack", &ex_runspeed_on);
			ImGui::Checkbox("Vehicle Acceleration", &ex_vehicleaceleration);
			ImGui::Checkbox("Vehicle Godmode", &ex_vehiclegodmode);
			ImGui::Checkbox("No Recoil", &ex_norecoil);
			ImGui::Checkbox("No Reload", &ex_noreaload);
			ImGui::Checkbox("No Spread", &ex_nospread);

			ImGui::Dummy(ImVec2(0, 16));
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Adjustments");
			ImGui::Separator();
			ImGui::SliderFloat("Movement Speed", &ex_runspeed_value, 0.f, 1000000.f, "%.1f");
			ImGui::SliderFloat("Vehicle Boost", &ex_vehicleaceleration_value, 0.f, 20000.f, "%.2f");
			if (ImGui::Button("Repair Engine", ImVec2(180, 30))) {
				Vehicle::FixEngine(1000.f);
			}
			break;

		case settings:
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Configuration");
			ImGui::Separator();
			ImGui::Combo("Menu Key", &bind_menu, menu_bind, IM_ARRAYSIZE(menu_bind));
			ImGui::Combo("Aimbot Key", &bind_aimbot, aimbot_bind, IM_ARRAYSIZE(aimbot_bind));
			ImGui::Combo("Trigger Key", &bind_trigger, trigger_bind, IM_ARRAYSIZE(trigger_bind));
			ImGui::Combo("Silent Aim Key", &silentaim_config::bind, silentaim_bind, IM_ARRAYSIZE(silentaim_bind));
			break;

		case friendlist:
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Safe Players");
			ImGui::Separator();
			PlayerList();
			break;
		}

		ImGui::EndChild();
		ImGui::End();
	} // <-- zamyka if (menu_visible)
} // <-- zamyka funkcję Render()

void ReallockLocalPlayer() {
    util::Log(util::LogLevel::DEBUG, "ReallockLocalPlayer thread started");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> jitter(0, 20);

    while (running.load()) {
        try {
            if (world_ptr) {
                uintptr_t player = read_mem<uintptr_t>(world_ptr + 0x8);
                if (player != 0) {
                    uintptr_t current = GetLocalPlayerSafe();
                    if (player != current) {
                        SetLocalPlayerSafe(player);
                        util::Log(util::LogLevel::DEBUG, "LocalPlayer updated to 0x%p", (void*)player);
                    }
                }
            }
        }
        catch (const std::exception& e) {
            util::Log(util::LogLevel::ERROR, "ReallockLocalPlayer exception: %s", e.what());
        }
        catch (...) {
            util::Log(util::LogLevel::ERROR, "ReallockLocalPlayer unknown exception");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30 + jitter(gen)));
    }

    util::Log(util::LogLevel::DEBUG, "ReallockLocalPlayer thread exiting");
}

int initialize() {
    util::Log(util::LogLevel::INFO, "Initializing...");

    attach_window = FindWindowA("grcWindow", 0);
    if (!attach_window) {
        util::Log(util::LogLevel::ERROR, "FiveM window not found");
        std::string msg = skCrypt("FiveM application not found.").decrypt();
        MessageBoxA(NULL, msg.c_str(), "BOBX.FUN - Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // mapowanie procesu -> offsety (nazwy przechowywane w std::string by uniknąć tymczasowych c_str())
    struct ProcessMapping {
        std::string name;
        offset::VersionOffsets* offsets;
        bool* detect_flag;
    };

    std::vector<ProcessMapping> process_map = {
        { skCrypt("FiveM_b3095_GTAProcess.exe").decrypt(), &offset::b3095, &offset::detectv3095 },
        { skCrypt("FiveM_b2944_GTAProcess.exe").decrypt(), &offset::b2944, &offset::detectv2944 },
        { skCrypt("FiveM_b2802_GTAProcess.exe").decrypt(), &offset::b2802, &offset::detectv2802 },
        { skCrypt("FiveM_b2699_GTAProcess.exe").decrypt(), &offset::b2699, &offset::detectv2699 },
        { skCrypt("FiveM_b2612_GTAProcess.exe").decrypt(), &offset::b2612, &offset::detectv2612 },
        { skCrypt("FiveM_b2545_GTAProcess.exe").decrypt(), &offset::b2545, &offset::detectv2545 },
        { skCrypt("FiveM_b2372_GTAProcess.exe").decrypt(), &offset::b2372, &offset::detectv2372 },
        { skCrypt("FiveM_b2189_GTAProcess.exe").decrypt(), &offset::b2189, &offset::detectv2189 },
        { skCrypt("FiveM_b2060_GTAProcess.exe").decrypt(), &offset::b2060, &offset::detectv2060 },
        { skCrypt("FiveM_GTAProcess.exe").decrypt(),       &offset::b1604, &offset::detectv1604 },
    };

    offset::VersionOffsets* selected_offsets = nullptr;

    for (auto& mapping : process_map) {
        if (isProcRunning(mapping.name.c_str())) {
            *mapping.detect_flag = true;
            selected_offsets = mapping.offsets;
            procname = mapping.name;
            util::Log(util::LogLevel::INFO, "Detected process: %s", procname.c_str());
            break;
        }
    }

    if (!selected_offsets) {
        util::Log(util::LogLevel::ERROR, "No supported FiveM process found");
        std::string msg = skCrypt("No supported FiveM version running.").decrypt();
        MessageBoxA(NULL, msg.c_str(), "BOBX.FUN - Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(attach_window, &pid);

    DWORD desiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_OPERATION;
    process_handle = OpenProcess(desiredAccess, FALSE, pid);
    if (!process_handle) {
        util::Log(util::LogLevel::ERROR, "OpenProcess failed for pid %u (err=%u)", pid, GetLastError());
        std::string msg = skCrypt("Failed to open process.").decrypt();
        MessageBoxA(NULL, msg.c_str(), "BOBX.FUN - Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    base_address = get_module_base64(pid, std::wstring(procname.begin(), procname.end()).c_str());
    if (!base_address.modBaseAddr) {
        util::Log(util::LogLevel::ERROR, "Failed to get module base address for %s", procname.c_str());
        CloseHandle(process_handle);
        process_handle = nullptr;
        return 1;
    }

    // Odczytywanie wskaźników z wybranych offsetów
    if (selected_offsets->ReplayInterface_ptr) ReplayInterface_ptr = read_mem<uintptr_t>(base_address.modBaseAddr + selected_offsets->ReplayInterface_ptr);
    if (selected_offsets->world_ptr) world_ptr = read_mem<uintptr_t>(base_address.modBaseAddr + selected_offsets->world_ptr);
    if (selected_offsets->viewport_ptr) viewport_ptr = read_mem<uintptr_t>(base_address.modBaseAddr + selected_offsets->viewport_ptr);

    if (world_ptr) {
        uintptr_t player = read_mem<uintptr_t>(world_ptr + 0x8);
        if (player != 0) {
            SetLocalPlayerSafe(player);
            util::Log(util::LogLevel::INFO, "LocalPlayer initialized: 0x%p", (void*)player);
        }
    }

    if (!setupWindow()) {
        util::Log(util::LogLevel::ERROR, "setupWindow failed");
        if (process_handle) { CloseHandle(process_handle); process_handle = nullptr; }
        return 1;
    }

    running.store(true);
    reallock_thread = std::thread(ReallockLocalPlayer);

    if (!InitializeSilentAim()) {
        util::Log(util::LogLevel::WARNING, "SilentAim initialization failed");
    }

    tema();
    while (!glfwWindowShouldClose(menu_window)) {
        handleKeyPresses();
        Render();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    util::Log(util::LogLevel::INFO, "Shutting down...");
    running.store(false);
    if (reallock_thread.joinable()) reallock_thread.join();

    if (process_handle) { CloseHandle(process_handle); process_handle = nullptr; }

    cleanupWindow();
    return 0;
}

// Wrappery zgodne z main.hpp
namespace overlay {
    bool LoadTexture(const char* path, GLuint* tex, int* w, int* h) {
        return LoadTextureFromFile(path, tex, w, h);
    }
    void ApplyTheme() { tema(); }
    bool Setup() { return setupWindow(); }
    void Cleanup() { cleanupWindow(); }
}

namespace input {
    void HandleKeyPresses() {
        ::handleKeyPresses();
    }
}

namespace game {
    void ReallockLocalPlayer() {
        ::ReallockLocalPlayer();
    }
}

int main() {
    if (IsDebuggerPresent()) {
        ExitProcess(0);
    }

    util::InitLogger("bobx.log");
    util::Log(util::LogLevel::INFO, "=== BOBX.FUN Started ===");

    // Ustawienia konsoli (oryginalne)
#define color SetConsoleTextAttribute
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    static CONSOLE_FONT_INFOEX  fontex;
    fontex.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    GetCurrentConsoleFontEx(hOut, 0, &fontex);
    fontex.FontWeight = 800;
    SetCurrentConsoleFontEx(hOut, NULL, &fontex);
    color(hConsole, 4);

    // Wywołaj initialize() — zwróci kod rezultatu
    int result = initialize();

    // Zapewnij deterministyczne zakończenie wątków i loggera
    running.store(false);
    if (reallock_thread.joinable()) reallock_thread.join();

    util::Log(util::LogLevel::INFO, "=== BOBX.FUN Exiting ===");
    util::CloseLogger();

    return result;
}