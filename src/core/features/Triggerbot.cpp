#include "Triggerbot.hpp"

void Triggerbot::Run() {
    if (!cfg::triggerbot::enabled)
        return;

    // Check if trigger key is pressed
    if (!(GetAsyncKeyState(cfg::triggerbot::key) & 0x8000))
        return;

    auto& cache = Cache::Get();
    if (!cache.local.alive)
        return;

    if (IsCrosshairOnEnemy()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cfg::triggerbot::delay));
        Shoot();
    }
}

bool Triggerbot::IsCrosshairOnEnemy() {
    auto& cache = Cache::Get();
    
    // Simple implementation: check if any enemy is close to screen center
    Vec2_t screen_center = { 960.0f, 540.0f }; // 1920x1080 center
    
    for (const auto& player : cache.players) {
        if (!player.alive || player.localplayer)
            continue;
            
        if (!cfg::triggerbot::team && player.team == cache.local.team)
            continue;

        // Check if player head is near crosshair
        if (player.bone_list.size() > 6) {
            Vec3_t head_pos = player.bone_list[6].pos;
            
            // World to screen
            const auto& vm = cache.game.view_matrix;
            float w = vm.matrix[3][0] * head_pos.x + vm.matrix[3][1] * head_pos.y + 
                      vm.matrix[3][2] * head_pos.z + vm.matrix[3][3];

            if (w < 0.001f)
                continue;

            float x = vm.matrix[0][0] * head_pos.x + vm.matrix[0][1] * head_pos.y + 
                      vm.matrix[0][2] * head_pos.z + vm.matrix[0][3];
            float y = vm.matrix[1][0] * head_pos.x + vm.matrix[1][1] * head_pos.y + 
                      vm.matrix[1][2] * head_pos.z + vm.matrix[1][3];

            Vec2_t screen_pos;
            screen_pos.x = 960.0f * (1.0f + x / w);
            screen_pos.y = 540.0f * (1.0f - y / w);

            // Check distance from center
            float dx = screen_pos.x - screen_center.x;
            float dy = screen_pos.y - screen_center.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < 50.0f) // Within 50 pixels of crosshair
                return true;
        }
    }
    
    return false;
}

void Triggerbot::Shoot() {
    // Simulate mouse click
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}
