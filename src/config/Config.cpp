#include "Config.hpp"

bool Config::Read() {
	return GetInstance().ReadImpl();
}

bool Config::Write() {
	return GetInstance().WriteImpl();
}

bool Config::ReadImpl() {
	std::ifstream f("danger_config.json");

	if (!f.good()) {
		LOGF(FATAL, "Configuration file does not exist, creating a new one");
		WriteImpl();
		return false;
	}

	json data;
	try {
		data = json::parse(f);
	}
	catch (const std::exception& e) {
		LOGF(FATAL, "Failed to parse configuration file");
		WriteImpl();
		return false;
	}

	if (data.empty())
		return false;

	try {
		// general
		cfg::enabled = data.value("enabled", true);

		// aimbot
		const auto& aim = data["aimbot"];
		cfg::aimbot::enabled = aim.value("enabled", true);
		cfg::aimbot::hotkey = aim.value("hotkey", VK_XBUTTON2);
		cfg::aimbot::always_on = aim.value("always_on", false);
		cfg::aimbot::fov = aim.value("fov", 5.0f);
		cfg::aimbot::smooth = aim.value("smooth", 3.0f);
		cfg::aimbot::visible_only = aim.value("visible_only", true);
		cfg::aimbot::velocity_comp = aim.value("velocity_comp", true);
		cfg::aimbot::velocity_comp_scale = aim.value("velocity_comp_scale", 0.02f);
		cfg::aimbot::rcs = aim.value("rcs", true);
		cfg::aimbot::aim_assist = aim.value("aim_assist", false);
		cfg::aimbot::multibone = aim.value("multibone", true);
		cfg::aimbot::multibone_closest = aim.value("multibone_closest", true);
		cfg::aimbot::multibone_interpolate = aim.value("multibone_interpolate", true);
		cfg::aimbot::multibone_interp_steps = aim.value("multibone_interp_steps", 2);
		cfg::aimbot::exposed_bones_only = aim.value("exposed_bones_only", false);
		cfg::aimbot::humanization = aim.value("humanization", true);
		cfg::aimbot::reaction_time_ms = aim.value("reaction_time_ms", 200.f);
		cfg::aimbot::aim_error_px = aim.value("aim_error_px", 3.5f);
		cfg::aimbot::tracking_jitter = aim.value("tracking_jitter", 0.8f);
		cfg::aimbot::miss_chance = aim.value("miss_chance", 0.08f);
		cfg::aimbot::flick_overshoot_px = aim.value("flick_overshoot_px", 4.0f);

		// triggerbot
		const auto& trig = data["triggerbot"];
		cfg::triggerbot::enabled = trig.value("enabled", false);
		cfg::triggerbot::team = trig.value("team", false);
		cfg::triggerbot::key = trig.value("key", VK_XBUTTON1);
		cfg::triggerbot::delay = trig.value("delay", 50);
		cfg::triggerbot::only_in_crosshair = trig.value("only_in_crosshair", true);

		// rcs
		const auto& rcs = data["rcs"];
		cfg::rcs::enabled = rcs.value("enabled", false);
		cfg::rcs::horizontal = rcs.value("horizontal", 1.0f);
		cfg::rcs::vertical = rcs.value("vertical", 1.0f);

		// antiflash
		const auto& flash = data["antiflash"];
		cfg::antiflash::enabled = flash.value("enabled", true);
		cfg::antiflash::opacity = flash.value("opacity", 0.0f);

		// settings
		cfg::settings::free_cpu = data["settings"].value("free_cpu", true);
		cfg::settings::update_rate = data["settings"].value("update_rate", 1);
	}
	catch (const std::exception& e) {
		LOGF(FATAL, "Failed to parse configuration");
		WriteImpl();
		return false;
	}

	LOGF(INFO, "Successfully parsed configuration");
	return true;
}

bool Config::WriteImpl() {
	std::ofstream f("danger_config.json");

	json data;

	data["enabled"] = cfg::enabled;

	// aimbot
	data["aimbot"]["enabled"] = cfg::aimbot::enabled;
	data["aimbot"]["hotkey"] = cfg::aimbot::hotkey;
	data["aimbot"]["always_on"] = cfg::aimbot::always_on;
	data["aimbot"]["fov"] = cfg::aimbot::fov;
	data["aimbot"]["smooth"] = cfg::aimbot::smooth;
	data["aimbot"]["visible_only"] = cfg::aimbot::visible_only;
	data["aimbot"]["velocity_comp"] = cfg::aimbot::velocity_comp;
	data["aimbot"]["velocity_comp_scale"] = cfg::aimbot::velocity_comp_scale;
	data["aimbot"]["rcs"] = cfg::aimbot::rcs;
	data["aimbot"]["aim_assist"] = cfg::aimbot::aim_assist;
	data["aimbot"]["multibone"] = cfg::aimbot::multibone;
	data["aimbot"]["multibone_closest"] = cfg::aimbot::multibone_closest;
	data["aimbot"]["multibone_interpolate"] = cfg::aimbot::multibone_interpolate;
	data["aimbot"]["multibone_interp_steps"] = cfg::aimbot::multibone_interp_steps;
	data["aimbot"]["exposed_bones_only"] = cfg::aimbot::exposed_bones_only;
	data["aimbot"]["humanization"] = cfg::aimbot::humanization;
	data["aimbot"]["reaction_time_ms"] = cfg::aimbot::reaction_time_ms;
	data["aimbot"]["aim_error_px"] = cfg::aimbot::aim_error_px;
	data["aimbot"]["tracking_jitter"] = cfg::aimbot::tracking_jitter;
	data["aimbot"]["miss_chance"] = cfg::aimbot::miss_chance;
	data["aimbot"]["flick_overshoot_px"] = cfg::aimbot::flick_overshoot_px;

	// triggerbot
	data["triggerbot"]["enabled"] = cfg::triggerbot::enabled;
	data["triggerbot"]["team"] = cfg::triggerbot::team;
	data["triggerbot"]["key"] = cfg::triggerbot::key;
	data["triggerbot"]["delay"] = cfg::triggerbot::delay;
	data["triggerbot"]["only_in_crosshair"] = cfg::triggerbot::only_in_crosshair;

	// rcs
	data["rcs"]["enabled"] = cfg::rcs::enabled;
	data["rcs"]["horizontal"] = cfg::rcs::horizontal;
	data["rcs"]["vertical"] = cfg::rcs::vertical;

	// antiflash
	data["antiflash"]["enabled"] = cfg::antiflash::enabled;
	data["antiflash"]["opacity"] = cfg::antiflash::opacity;

	// settings
	data["settings"]["free_cpu"] = cfg::settings::free_cpu;
	data["settings"]["update_rate"] = cfg::settings::update_rate;

	f << std::setw(4) << data << std::endl;
	f.close();

	LOGF(VERBOSE, "Writing configuration to file");

	return true;
}

color_t Config::JsonToColor(const json& parent, const std::string& key, const color_t& def) {
	if (!parent.contains(key) || !parent[key].is_array() || parent[key].size() != 4)
		return def;
	return color_t(
		parent[key][0].get<float>(),
		parent[key][1].get<float>(),
		parent[key][2].get<float>(),
		parent[key][3].get<float>()
	);
}

void Config::ColorToJson(json& parent, const std::string& key, const color_t& color) {
	parent[key] = { color.r, color.g, color.b, color.a };
}

Vec2_t Config::JsonToVec2(const json& parent, const std::string& key, const Vec2_t& def)
{
	if (!parent.contains(key) || !parent[key].is_array() || parent[key].size() != 2)
		return def;

	return Vec2_t{
		parent[key][0].get<float>(),
		parent[key][1].get<float>()
	};
}

void Config::Vec2ToJson(json& parent, const std::string& key, const Vec2_t& vec)
{
	parent[key] = { vec.x, vec.y };
}
