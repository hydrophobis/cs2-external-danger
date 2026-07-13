#pragma once
#include <Windows.h>

namespace cfg {
	inline bool enabled = true;

	namespace aimbot {
		inline bool enabled = true;
		inline int hotkey = VK_XBUTTON2;
		inline bool always_on = false;
		inline float fov = 5.0f;
		inline float smooth = 3.0f;
		inline bool visible_only = true;
		inline bool velocity_comp = true;
		inline float velocity_comp_scale = 0.02f;
		inline bool rcs = true;

		inline bool aim_assist = false;

		inline bool multibone = true;
		inline bool multibone_closest = true;
		inline int bone_priority[5] = { 7, 6, 5, 4, 2 };

		inline bool humanization = true;
		inline float reaction_time_ms = 200.f;
		inline float aim_error_px = 3.5f;
		inline float tracking_jitter = 0.8f;
		inline float miss_chance = 0.08f;
		inline float flick_overshoot_px = 4.0f;
		inline float dead_zone = 0.f;
		inline bool dead_zone_enabled = true;
		inline float stop_threshold = 1.5f;
	}

	namespace triggerbot {
		inline bool enabled = false;
		inline bool team = false;
		inline int key = VK_XBUTTON1;
		inline int delay = 50;
		inline bool only_in_crosshair = true;
		inline bool randomization = true;
	}

	namespace rcs {
		inline bool enabled = true;
		inline float horizontal = 1.0f;
		inline float vertical = 1.0f;
		inline float smooth = 1.0f;
	}

	namespace antiflash {
		inline bool enabled = true;
		inline float opacity = 0.0f;
	}

	namespace esp {
		inline bool team = true;

		inline bool box = true;
		inline bool armor = true;
		inline bool health = true;
		inline bool skeleton = true;
		inline bool head_tracker = true;
		inline bool health_number = false;

		inline bool spotted = false;
		inline bool bomb = true;
		inline color_t bomb_color{ 1.f, 0.84f, 0.f, 1.f };

		inline bool tracers = false;

		namespace flags {
			inline bool name = true;
			inline bool ping = true;
			inline bool weapon = false;
			inline bool ammo = false;
			inline bool reloading = false;
			inline bool defusing = false;
			inline bool money = false;
			inline bool flashed = false;
			inline bool scoped = false;
			inline bool has_c4 = false;
		}

		namespace colors {
			inline color_t box_team{ 0.f, 1.f, 0.29f, 0.5f };
			inline color_t box_enemy{ 1.f, 0.f, 0.f, 0.5f };

			inline color_t skeleton_team{ 0.f, 1.f, 0.f, 0.5f };
			inline color_t skeleton_enemy{ 1.f, 0.f, 0.f, 0.5f };

			inline color_t tracker_team{ 1.f, 1.f, 1.f, 0.3f };
			inline color_t tracker_enemy{ 1.f, 1.f, 1.f, 0.3f };

			inline color_t tracer_team{ 0.f, 1.f, 0.f, 0.5f };
			inline color_t tracer_enemy{ 1.f, 0.f, 0.f, 0.5f };

			namespace flags {
				inline color_t flashed_team{ 1.f, 1.f, 1.f, 0.5f };
				inline color_t flashed_enemy{ 1.f, 1.f, 1.f, 0.8f };

				inline color_t reloading_team{ 1.f, 1.f, 1.f, 0.5f };
				inline color_t reloading_enemy{ 1.f, 1.f, 1.f, 0.8f };

				inline color_t defusing_team{ 1.f, 1.f, 1.f, 0.5f };
				inline color_t defusing_enemy{ 1.f, 1.f, 1.f, 0.8f };

				inline color_t scoped_team{ 1.f, 1.f, 1.f, 0.5f };
				inline color_t scoped_enemy{ 1.f, 1.f, 1.f, 0.8f };

				inline color_t c4_team{ 1.f, 0.84f, 0.f, 1.f };
				inline color_t c4_enemy{ 1.f, 0.84f, 0.f, 1.f };
			}
		}
	}

	namespace world {
		namespace spectators {
			inline bool enabled = false;
			inline bool detailed = false;
			inline bool self_only = true;
			inline Vec2_t pos{ 10.f, 100.f };
		}

		namespace bomb {
			inline bool location = true;
			inline bool timer = true;
			inline bool hud = false;
			inline Vec2_t pos{ 10.f, 300.f };
		}

		namespace crosshair {
			inline bool enabled = false;
		}

		namespace radar {
			inline bool enabled = true;
			inline bool no_rotate = false;
			inline float range = 2000.f;
			inline Vec2_t pos{ 10.f, 10.f };
			inline Vec2_t size{ 200.f, 200.f };
		}

		namespace velocity {
			inline bool enabled = false;
			inline int sample_rate = 35;
			inline float sample_length = 5.f;
			inline Vec2_t size{ 400.f, 100.f };
			inline Vec2_t pos{ 10.f, 400.f };
		}
	}

	namespace settings {
		inline bool watermark = true;
		inline bool streamproof = false;
		inline bool vsync = false;
		inline bool free_cpu = true;
		inline int update_rate = 1;
	}

	namespace dev {
		inline bool console = true;
		inline int open_menu_key = false;
		inline int cache_refresh_rate = 5;
		inline bool force_show_flags = false;
	}
}