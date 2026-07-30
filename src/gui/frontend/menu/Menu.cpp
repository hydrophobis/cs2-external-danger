#include "Menu.hpp"
#include <cstring>
#include "core/scripting/Scripting.hpp"
#include "core/scripting/Scripting.hpp"
#include "core/engine/cache/Cache.hpp"
#include "gui/renderer/Renderer.hpp" // Circular dependency
#include "gui/renderer/window/Window.hpp" // Circular dependency
#include "assets/fonts/Icons.h";


bool Menu::Init() {
	return GetInstance().InitImpl();
}

void Menu::Render() {
	return GetInstance().RenderImpl();
}

void Menu::RenderStartupHelp() {
	return GetInstance().RenderStartupHelpImpl();
}

ImVec2 Menu::GetPos() {
	return GetInstance().pos;
}

ImVec2 Menu::GetSize() {
	return GetInstance().size;
}

bool Menu::InitImpl() {
	SetupStyles();
	theme::ThemeManager::Get().Init();

	LOGF(INFO, "Successfully initialized menu...");
	return true;
}

void Menu::RenderImpl() {
	if (!isSetup)
		return;

	static auto io = ImGui::GetIO();
	static auto screen = io.DisplaySize;
	static auto color_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None;

#ifdef _DEBUG
	static auto title = "cs2-external-danger - P2C Config [DEV]";
#else
	static auto title = "cs2-external-danger | P2C Config";
#endif

	ImGui::SetNextWindowSize(ImVec2(640, 520), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(screen.x / 2 - 320, screen.y / 2 - 260), ImGuiCond_FirstUseEver);

	ImGui::GetWindowPos();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
		this->pos = ImGui::GetWindowPos();
		this->size = ImGui::GetWindowSize();

		static int active_tab = 0;

		if (ImGui::BeginChild("##main_split"))
		{
			auto size = ImGui::GetContentRegionAvail();

			ImGui::BeginChild("##tab_buttons", ImVec2(120, size.y), true);
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.1f, 0.5f));
				for (const auto& tab : tabs)
				{
					bool is_active = (active_tab == tab.id);

					if (is_active)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
					}

					if (ImGui::Button((tab.icon + " " + tab.label).c_str(), ImVec2(-1, 28)))
						active_tab = tab.id;

					if (is_active) 
						ImGui::PopStyleColor(3);
				}
				ImGui::PopStyleVar(1);

				auto space = ImGui::GetContentRegionAvail().y;
				ImGui::SetCursorPosY(ImGui::GetCursorPos().y + space - 16.f * 3);

				ImGui::Dummy(ImVec2(11, 0)); ImGui::SameLine();
				ImGui::TextLinkOpenURL(Icons::DISCORD, "https://discord.gg/pRew8ZDkyp");
				ImGui::SameLine();
				ImGui::Dummy(ImVec2(11, 0)); ImGui::SameLine();
				ImGui::TextLinkOpenURL(Icons::GITHUB, "https://github.com/IMXNOOBX/cs2-external-esp");
				ImGui::Separator();

				ImGui::Checkbox("Enable", &cfg::enabled);
			}
			ImGui::EndChild();

			ImGui::SameLine();


			ImGui::BeginDisabled(!cfg::enabled);

			ImGui::BeginChild("##tab_content", ImVec2(0, size.y), true);
			{
				if (active_tab == Tab::PLAYER)
				{
					ImGui::Text("Aimbot");
					ImGui::Separator();

					ImGui::Checkbox("Enable Aimbot", &cfg::aimbot::enabled);
					ImGui::BeginDisabled(!cfg::aimbot::enabled);
					{
						ImGui::Checkbox("Always On (Inverted)", &cfg::aimbot::always_on);
						ImGui::SetItemTooltip("If checked, aimbot is ON by default and OFF when key is held");
						
						ImGui::SliderFloat("FOV", &cfg::aimbot::fov, 1.0f, 40.0f, "%.1f");
						ImGui::SetItemTooltip("Field of view radius (multiplied by 10 in screen space)");
						
						ImGui::SliderFloat("Smooth", &cfg::aimbot::smooth, 1.0f, 15.0f, "%.1f");
						ImGui::SetItemTooltip("Higher = slower, more human-like correction");
						
						ImGui::Checkbox("Visible Only", &cfg::aimbot::visible_only);
						ImGui::SetItemTooltip("Only target enemies that are visible/spotted");
						
						ImGui::Checkbox("Multibone Targeting", &cfg::aimbot::multibone);
						ImGui::SetItemTooltip("Try multiple bones instead of just head");
						
						ImGui::BeginDisabled(!cfg::aimbot::multibone);
						ImGui::Checkbox("  Closest Bone Mode", &cfg::aimbot::multibone_closest);
						ImGui::SetItemTooltip("If ON: aim at whichever bone is closest to crosshair.\nIf OFF: use priority (head -> neck -> chest)");
						
						ImGui::Checkbox("  Interpolate Segments", &cfg::aimbot::multibone_interpolate);
						ImGui::SetItemTooltip("Also aim at points between connected bones (neck→head, shoulder→elbow, etc.)\nMakes aimbot looser, less HSP but good accuracy");
						
						ImGui::BeginDisabled(!cfg::aimbot::multibone_interpolate);
						ImGui::SliderInt("  Interp Steps", &cfg::aimbot::multibone_interp_steps, 1, 5);
						ImGui::SetItemTooltip("Number of sample points between each bone pair");
						ImGui::EndDisabled();
						
						ImGui::Checkbox("  Exposed Bones Only", &cfg::aimbot::exposed_bones_only);
						ImGui::SetItemTooltip("Only aim at bones/skin that are visible via raytrace (not behind cover)");
						ImGui::EndDisabled();
						
						ImGui::Checkbox("Velocity Compensation", &cfg::aimbot::velocity_comp);
						ImGui::BeginDisabled(!cfg::aimbot::velocity_comp);
						ImGui::SliderFloat("Velocity Scale", &cfg::aimbot::velocity_comp_scale, 0.0f, 0.1f, "%.3f");
						ImGui::SetItemTooltip("How much to compensate for target movement");
						ImGui::EndDisabled();
						
						ImGui::Checkbox("RCS Integration", &cfg::aimbot::rcs);
						ImGui::SetItemTooltip("Use standalone RCS feature for recoil control");

						ImGui::Text("Aimbot Key:");
						ImGui::SameLine();
						const char* aimbot_keys[] = { "Mouse 4 (VK_XBUTTON1)", "Mouse 5 (VK_XBUTTON2)", "Left Alt", "Left Shift" };
						int aimbot_key_index = (cfg::aimbot::hotkey == VK_XBUTTON1) ? 0 : (cfg::aimbot::hotkey == VK_XBUTTON2) ? 1 : (cfg::aimbot::hotkey == VK_MENU) ? 2 : 3;
						if (ImGui::Combo("##aimbotkey", &aimbot_key_index, aimbot_keys, IM_ARRAYSIZE(aimbot_keys))) {
							cfg::aimbot::hotkey = (aimbot_key_index == 0) ? VK_XBUTTON1 : (aimbot_key_index == 1) ? VK_XBUTTON2 : (aimbot_key_index == 2) ? VK_MENU : VK_LSHIFT;
						}

						ImGui::Spacing();
						ImGui::Text("Humanization");
						ImGui::Separator();

						ImGui::Checkbox("Humanization", &cfg::aimbot::humanization);
						ImGui::SetItemTooltip("Adds natural imperfections (reaction delay, aim error, jitter, occasional miss)");
						ImGui::BeginDisabled(!cfg::aimbot::humanization);
						{
							ImGui::Checkbox("Aim Assist", &cfg::aimbot::aim_assist);
							ImGui::SetItemTooltip("Lighter assist: skip the humanized flick curvature so the initial snap onto a target is more direct");

							ImGui::SliderFloat("Reaction (ms)", &cfg::aimbot::reaction_time_ms, 50.f, 500.f, "%.0f");
							ImGui::SetItemTooltip("Delay before the aimbot starts correcting onto a new target");

							ImGui::SliderFloat("Aim Error (px)", &cfg::aimbot::aim_error_px, 0.f, 15.f, "%.1f");
							ImGui::SetItemTooltip("Random offset so crosshair doesn't land dead-centre every time");

							ImGui::SliderFloat("Tracking Jitter", &cfg::aimbot::tracking_jitter, 0.f, 3.f, "%.1f");
							ImGui::SetItemTooltip("Micro-wobble during tracking to feel like a real hand");

							ImGui::SliderFloat("Miss Chance", &cfg::aimbot::miss_chance, 0.f, 0.5f, "%.2f");
							ImGui::SetItemTooltip("Probability of deliberately pulling the shot off target (0 = never, 0.5 = 50%)");

							ImGui::SliderFloat("Flick Overshoot (px)", &cfg::aimbot::flick_overshoot_px, 0.f, 20.f, "%.1f");
							ImGui::SetItemTooltip("How many pixels to overshoot during the initial snap before correcting back");
						}
						ImGui::EndDisabled();
					}
					ImGui::EndDisabled();

					ImGui::Spacing();
					ImGui::Text("Triggerbot");
					ImGui::Separator();

					ImGui::Checkbox("Enable Triggerbot", &cfg::triggerbot::enabled);
					ImGui::BeginDisabled(!cfg::triggerbot::enabled);
					{
						ImGui::Checkbox("Target Team##trig", &cfg::triggerbot::team);
						ImGui::SliderInt("Delay (ms)", &cfg::triggerbot::delay, 0, 200);
						ImGui::Checkbox("Only in Crosshair", &cfg::triggerbot::only_in_crosshair);
						
						ImGui::Text("Triggerbot Key:");
						ImGui::SameLine();
						const char* trigger_keys[] = { "Mouse 4 (VK_XBUTTON1)", "Mouse 5 (VK_XBUTTON2)", "Left Alt", "Left Shift" };
						int trigger_key_index = (cfg::triggerbot::key == VK_XBUTTON1) ? 0 : (cfg::triggerbot::key == VK_XBUTTON2) ? 1 : (cfg::triggerbot::key == VK_MENU) ? 2 : 3;
						if (ImGui::Combo("##triggerbotkey", &trigger_key_index, trigger_keys, IM_ARRAYSIZE(trigger_keys))) {
							cfg::triggerbot::key = (trigger_key_index == 0) ? VK_XBUTTON1 : (trigger_key_index == 1) ? VK_XBUTTON2 : (trigger_key_index == 2) ? VK_MENU : VK_LSHIFT;
						}
					}
					ImGui::EndDisabled();

					ImGui::Spacing();
					ImGui::Text("Anti-Flash");
					ImGui::Separator();

					ImGui::Checkbox("Enable Anti-Flash", &cfg::antiflash::enabled);
					ImGui::BeginDisabled(!cfg::antiflash::enabled);
					{
						ImGui::SliderFloat("Flash Opacity", &cfg::antiflash::opacity, 0.0f, 1.0f, "%.2f");
						ImGui::SetItemTooltip("0.0 = no flash, 1.0 = full flash");
						ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Note: Requires game offset to work");
					}
					ImGui::EndDisabled();

					ImGui::Spacing();
					ImGui::Text("Recoil Control (RCS)");
					ImGui::Separator();

					ImGui::Checkbox("Enable RCS", &cfg::rcs::enabled);
					ImGui::BeginDisabled(!cfg::rcs::enabled);
					{
						ImGui::SliderFloat("Horizontal", &cfg::rcs::horizontal, 0.0f, 2.0f, "%.2f");
						ImGui::SliderFloat("Vertical", &cfg::rcs::vertical, 0.0f, 2.0f, "%.2f");
						ImGui::SliderFloat("RCS Smooth", &cfg::rcs::smooth, 0.0f, 5.0f, "%.2f");
						ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Note: Requires game offset to work");
					}
					ImGui::EndDisabled();
				}
				else if (active_tab == Tab::WORLD)
				{
					ImGui::Text("P2C Features Status");
					ImGui::Separator();
					
					ImGui::Text("Aimbot: %s", cfg::aimbot::enabled ? "ENABLED" : "DISABLED");
					ImGui::Text("Triggerbot: %s", cfg::triggerbot::enabled ? "ENABLED" : "DISABLED");
					ImGui::Text("Anti-Flash: %s", cfg::antiflash::enabled ? "ENABLED" : "DISABLED");
					ImGui::Text("RCS: %s", cfg::rcs::enabled ? "ENABLED" : "DISABLED");
					
					ImGui::Spacing();
					ImGui::Text("Info");
					ImGui::Separator();
					
					ImGui::TextWrapped("This is the P2C (Paste-to-Cheat) version with no ESP rendering.");
					ImGui::TextWrapped("Configure all features in the first tab.");
					ImGui::TextWrapped("Hold your configured keys to activate aimbot/triggerbot.");
					
					ImGui::Spacing();
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Note:");
					ImGui::TextWrapped("Anti-Flash and RCS require game memory offsets to function properly.");
				}
				else if (active_tab == Tab::SETTINGS)
				{
					ImGui::Text("UI Theme");
					ImGui::Separator();

					auto& theme_mgr = theme::ThemeManager::Get();
					static std::vector<std::string> theme_names = theme_mgr.GetThemeNames();
					static int selected_theme = 0;
					
					// Create combo items
					std::string combo_preview = theme_names.empty() ? "No themes" : theme_mgr.GetCurrentThemeName();
					
					if (ImGui::BeginCombo("Theme", combo_preview.c_str())) {
						for (size_t i = 0; i < theme_names.size(); i++) {
							bool is_selected = (theme_mgr.GetCurrentThemeName() == theme_names[i]);
							if (ImGui::Selectable(theme_names[i].c_str(), is_selected)) {
								theme_mgr.ApplyTheme(theme_names[i]);
							}
							if (is_selected) {
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					if (ImGui::Button("Reload Themes")) {
						theme_mgr.LoadThemesFromDirectory();
						theme_names = theme_mgr.GetThemeNames();
						LOGF(INFO, "Reloaded themes - found {} themes", theme_names.size());
					}
					ImGui::SameLine();
					if (ImGui::Button("Export Current")) {
						static char filename[64] = "my_theme";
						ImGui::OpenPopup("Export Theme");
						
						if (ImGui::BeginPopup("Export Theme")) {
							ImGui::Text("Enter theme name:");
							ImGui::InputText("##filename", filename, sizeof(filename));
							if (ImGui::Button("Save")) {
								theme_mgr.SaveCurrentTheme(filename);
								ImGui::CloseCurrentPopup();
							}
							ImGui::SameLine();
							if (ImGui::Button("Cancel")) {
								ImGui::CloseCurrentPopup();
							}
							ImGui::EndPopup();
						}
					}

					// Show current theme info
					const theme::ThemeInfo* info = theme_mgr.GetThemeInfo(theme_mgr.GetCurrentThemeName());
					if (info) {
						ImGui::Text("Author: %s", info->author.empty() ? "Unknown" : info->author.c_str());
						if (!info->description.empty()) {
							ImGui::TextWrapped("%s", info->description.c_str());
						}
					}

					ImGui::Spacing();
					ImGui::Text("Misc");
					ImGui::Separator();

					if (ImGui::Checkbox("Streamproof", &cfg::settings::streamproof))
					{
						Window::SetAffinity(
							Window::hwnd,
							cfg::settings::streamproof ? WindowAffinity::Invisible : WindowAffinity::Disabled
						);
					}

					ImGui::Checkbox("Watermark", &cfg::settings::watermark);

					if (ImGui::Checkbox("VSync", &cfg::settings::vsync))
						Window::vsync = cfg::settings::vsync;

					ImGui::Checkbox("Free CPU", &cfg::settings::free_cpu);
					ImGui::SetItemTooltip("Let the CPU sleep to Free Resources\nNOTE: might cause performance issues in lower end computers!");

					ImGui::Text("Notes");
					ImGui::Separator();
					ImGui::TextWrapped(
						"If you experience bad performance/lag try the following:\n"
						"\t- Disable ESP VSync: Look up > VSync: Un-Check\n"
						"\t- Disable VSync in game: ...Advanced Video > V-Sync: Disabled\n"
						"\t- Last Resort: Disable \"Free CPU\" option, it will inpact on your overall performace, but improve latency\n"
					);

#ifdef _DEBUG
					ImGui::Text("Dev");
					ImGui::Separator();

					if (ImGui::Checkbox("Console", &cfg::dev::console))
						if (!cfg::dev::console) LogHelper::Free();

					static int key_out;
					if (ImGui::Button("Open Menu Key"))
					{
						for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i++)
						{
							if (ImGui::IsKeyPressed((ImGuiKey)i))
							{
								key_out = i;
								LOGF(VERBOSE, "Changed the open menu key to {}", key_out);
								break;
							}
						}
					}

					ImGui::SliderInt("Cache Refresh Rate", &cfg::dev::cache_refresh_rate, 0, 100, "%dms");
					ImGui::Checkbox("Force Show Flags", &cfg::dev::force_show_flags);
#endif
				}
				else if (active_tab == Tab::MACROS)
				{
					ImGui::Text("Script Macros");
					ImGui::Separator();

					ImGui::TextWrapped("Execute macros defined in scripts.txt. Only macros prefixed with @ in the definition are shown here.");
					ImGui::Spacing();

					if (ImGui::Button("Reload Scripts Now"))
					{
						scripting::Scripting::Get().LoadScripts();
					}
					ImGui::SetItemTooltip("Manually reload scripts.txt\n(Auto-reloads when file is modified)");

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Text("Available Macros:");
					ImGui::Separator();

					const auto& macros = scripting::Scripting::Get().GetMacros();
					
					int gui_macro_count = 0;
					for (const auto& [name, macro] : macros) {
						if (macro.gui_accessible) gui_macro_count++;
					}
					
					if (gui_macro_count == 0)
					{
						ImGui::TextWrapped("No GUI-accessible macros found. Create macros with @ prefix in scripts.txt.");
						ImGui::Spacing();
						ImGui::TextWrapped("Example:\nmacro @my_macro {\n    set esp.box true\n    set esp.skeleton true\n}");
					}
					else
					{
						ImGui::Text("Click a macro button to execute it:");
						ImGui::Spacing();

						float available_width = ImGui::GetContentRegionAvail().x;
						float button_width = 150.0f;
						float spacing = ImGui::GetStyle().ItemSpacing.x;
						int buttons_per_row = (int)((available_width + spacing) / (button_width + spacing));
						if (buttons_per_row < 1) buttons_per_row = 1;

						int col = 0;
						for (const auto& [name, macro] : macros)
						{
							if (!macro.gui_accessible) continue;
							
							if (col > 0 && col < buttons_per_row)
								ImGui::SameLine();

							std::string button_label = "@" + name;
							if (ImGui::Button(button_label.c_str(), ImVec2(button_width, 30)))
							{
								scripting::Scripting::Get().ExecuteCommand(name);
								LOGF(INFO, "Executed macro: @{}", name);
							}

							// tooltip with macro commands
							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								ImGui::Text("Macro: @%s", name.c_str());
								ImGui::Separator();
								ImGui::Text("Commands:");
								for (const auto& cmd : macro.commands)
								{
									ImGui::Text("  %s", cmd.c_str());
								}
								ImGui::EndTooltip();
							}

							col++;
							if (col >= buttons_per_row) col = 0;
						}
					}

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Text("Manual Execution:");
					ImGui::Separator();

					static char macro_input[256] = "@";
					float input_width = ImGui::GetContentRegionAvail().x - 100.0f; // Leave space for button
					ImGui::SetNextItemWidth(input_width);
					ImGui::InputText("##macro_input", macro_input, IM_ARRAYSIZE(macro_input));
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Enter macro name with @ prefix (e.g., @my_macro)");

					ImGui::SameLine();
					if (ImGui::Button("Execute", ImVec2(90, 0)))
					{
						if (macro_input[0] == '@' && strlen(macro_input) > 1)
						{
							std::string macro_name = std::string(macro_input + 1); // Skip @ prefix
							
							// Check if this macro exists and is GUI accessible
							const auto& all_macros = scripting::Scripting::Get().GetMacros();
							auto it = all_macros.find(macro_name);
							if (it != all_macros.end())
							{
								if (it->second.gui_accessible)
								{
									scripting::Scripting::Get().ExecuteCommand(macro_name);
									LOGF(INFO, "Executed macro: @{}", macro_name);
								}
								else
								{
									LOGF(WARNING, "Macro '{}' is not GUI-accessible (must be defined with @ prefix)", macro_name);
								}
							}
							else
							{
								LOGF(WARNING, "Macro '{}' not found", macro_name);
							}
						}
						else
						{
							LOGF(WARNING, "Invalid macro name. Must start with @");
						}
					}
				}
			}
			ImGui::EndChild();

			ImGui::EndDisabled();


			ImGui::EndChild();
		}

	}

	ImGui::End();
}

void Menu::SetupStyles() {
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

	style.Colors[ImGuiCol_WindowBg] = ImColor(10, 10, 10);
	style.Colors[ImGuiCol_ChildBg] = ImColor(10, 10, 10);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImColor(50, 50, 50);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	style.Colors[ImGuiCol_FrameBg] = ImColor(75, 75, 75);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);

	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);

	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);

	style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.29f, 0.30f, 0.31f, 0.95f);

	style.Colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	style.FrameBorderSize = 1.0f;

	// Window & Frame
	style.WindowRounding = 12.f;
	style.ChildRounding = 10.f;

	style.FrameRounding = 5.f;
	style.PopupRounding = 5.f;

	style.GrabRounding = 3.f;

	auto& io = ImGui::GetIO();

	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 16.0f);

	ImFontConfig merge_icon_cfg{};
	merge_icon_cfg.FontDataOwnedByAtlas = false;
	merge_icon_cfg.MergeMode = true;
	merge_icon_cfg.GlyphOffset = Vec2_t(0, 3.5f);

	// the icons will use the size specified when getting added so it ignores the base size
	static const ImWchar icon_ranges[] = { 0xE100, 0xE108, 0 };
	io.Fonts->AddFontFromMemoryTTF(icons_font, icons_font_len, 20.f, &merge_icon_cfg, icon_ranges);
}

void Menu::RenderStartupHelpImpl() {
	static bool has_opened_menu = false;

	if (has_opened_menu)
		return;

	auto& io = ImGui::GetIO();
	auto screen = io.DisplaySize;
	auto d = ImGui::GetBackgroundDrawList();

	if (Renderer::IsOpen())
		has_opened_menu = true;

	auto help = "To OPEN the menu, Use Insert or Right Shift keys"
		"\n\t\t\t\tTo CLOSE, press the End key";
	auto size = ImGui::CalcTextSize(help);

	d->AddText(
		ImVec2(screen.x / 2 - size.x / 2, 80),
		IM_COL32(255, 255, 255, 255),
		help
	);
}
