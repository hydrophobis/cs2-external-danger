#include "RCS.hpp"
#include "core/engine/Engine.hpp"
#include "core/offsets/Offsets.hpp"
#include "Aimbot.hpp"

Vec3_t RCS::last_punch_angle = { 0, 0, 0 };

void RCS::Run() {
    static bool first_run = true;
    if (first_run) {
        if (cfg::rcs::enabled) {
            LOGF(INFO, "RCS: Feature enabled");
        }
        first_run = false;
    }
    
    if (!cfg::rcs::enabled)
        return;
    
    if (Aimbot::is_aiming) {
        last_punch_angle = { 0, 0, 0 };
        return;
    }

    auto& cache = Cache::Get();
    if (!cache.local.alive) {
        last_punch_angle = { 0, 0, 0 };
        return;
    }

    auto process = Engine::GetProcess();
    if (!process) {
        return;
    }

    uintptr_t local_pawn = cache.local.pawn_addr;
    if (!local_pawn) {
        last_punch_angle = { 0, 0, 0 };
        return;
    }

    int32_t shots_fired = process->read<int32_t>(local_pawn + offsets::pawn::m_iShotsFired);
    
    if (shots_fired < 3) {
        last_punch_angle = { 0, 0, 0 };
        return;
    }
    
    if (shots_fired == 0) {
        last_punch_angle = { 0, 0, 0 };
        return;
    }

    uintptr_t aim_punch_service = process->read<uintptr_t>(local_pawn + offsets::pawn::m_pAimPunchServices);
    if (!aim_punch_service) {
        last_punch_angle = { 0, 0, 0 };
        return;
    }

    Vec3_t current_punch;
    current_punch.x = process->read<float>(aim_punch_service + offsets::pawn::m_predictableBaseAngle);
    current_punch.y = process->read<float>(aim_punch_service + offsets::pawn::m_predictableBaseAngle + 0x4);
    current_punch.z = 0;

    Vec3_t delta;
    delta.x = (current_punch.x - last_punch_angle.x) * cfg::rcs::vertical;
    delta.y = (current_punch.y - last_punch_angle.y) * cfg::rcs::horizontal;
    
    float current_magnitude = sqrtf(current_punch.x * current_punch.x + current_punch.y * current_punch.y);
    float last_magnitude = sqrtf(last_punch_angle.x * last_punch_angle.x + last_punch_angle.y * last_punch_angle.y);
    
    if (last_magnitude > 1.0f && current_magnitude < last_magnitude - 1.0f) {
        last_punch_angle = { 0, 0, 0 };
        return;
    }
    
    if (fabsf(delta.x) > 0.001f || fabsf(delta.y) > 0.001f) {
        float scale = 50.0f;
        int moveX = (int)(-delta.y * scale);
        int moveY = (int)(-delta.x * scale);
        
        if (moveX != 0 || moveY != 0) {
            mouse_event(MOUSEEVENTF_MOVE, (DWORD)moveX, (DWORD)moveY, 0, 0);
        }
    }

    last_punch_angle = current_punch;
}
