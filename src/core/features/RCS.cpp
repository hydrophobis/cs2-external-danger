#include "RCS.hpp"
#include "core/engine/Engine.hpp"
#include "core/offsets/Offsets.hpp"
#include "Aimbot.hpp"
#include <Windows.h>
#include <thread>
#include <cmath>
#include <algorithm>
#include <chrono>

using pNtUserSendInput = LONG(__stdcall*)(UINT, LPINPUT, int);
static pNtUserSendInput NtUserSendInput = nullptr;
static HMODULE hWin32u = nullptr;

static void InitNtUserSendInput() {
    if (NtUserSendInput) return;
    hWin32u = GetModuleHandleA("win32u.dll");
    if (!hWin32u) hWin32u = LoadLibraryA("win32u.dll");
    if (!hWin32u) return;
    NtUserSendInput = (pNtUserSendInput)GetProcAddress(hWin32u, "NtUserSendInput");
}

void RcsController::Reset() {
    applied = { 0.f, 0.f };
    rem_x = 0.f;
    rem_y = 0.f;
    armed = false;
}

bool RcsController::Step(Vec2_t punch, float dt, LONG& out_x, LONG& out_y) {
    out_x = 0;
    out_y = 0;

    Vec2_t target{ punch.x * cfg::rcs::vertical, punch.y * cfg::rcs::horizontal };

    if (!armed) {
        applied = target;
        armed = true;
        return false;
    }

    float smoothing = std::clamp(cfg::rcs::smooth, 0.f, 5.f);
    float tau = smoothing * 0.05f;
    float a = (tau <= 0.0001f) ? 1.0f
             : std::clamp(1.0f - std::exp(-dt / tau), 0.f, 1.f);

    Vec2_t err  = { target.x - applied.x, target.y - applied.y };
    Vec2_t step = { err.x * a, err.y * a };
    applied.x += step.x;
    applied.y += step.y;

    constexpr float scale = 50.0f;
    float fmx = step.y * scale + rem_x;
    float fmy = -step.x * scale + rem_y;
    out_x = static_cast<LONG>(std::lroundf(fmx));
    out_y = static_cast<LONG>(std::lroundf(fmy));
    rem_x = fmx - static_cast<float>(out_x);
    rem_y = fmy - static_cast<float>(out_y);

    return out_x != 0 || out_y != 0;
}

void RCS::Init() {
    thread_ = std::thread(RCS::Thread);
}

void RCS::Shutdown() {
    if (thread_.joinable())
        thread_.join();
}

void RCS::Thread() {
    InitNtUserSendInput();

    if (cfg::rcs::enabled)
        LOGF(INFO, "RCS: Feature enabled");

    std::chrono::steady_clock::time_point lastFrame{};
    bool haveDt = false;

    RcsController rcs;

    auto reset_state = [&] {
        rcs.Reset();
        haveDt = false;
    };

    while (app::running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (!cfg::rcs::enabled) {
            reset_state();
            continue;
        }

        HWND gameWnd = FindWindowA(nullptr, "Counter-Strike 2");
        if (!gameWnd || GetForegroundWindow() != gameWnd) {
            reset_state();
            continue;
        }

        if (Aimbot::is_aiming) {
            reset_state();
            continue;
        }

        auto& cache = Cache::Get();
        if (!cache.local.alive) {
            reset_state();
            continue;
        }

        auto process = Engine::GetProcess();
        if (!process)
            continue;

        uintptr_t local_pawn = cache.local.pawn_addr;
        if (!local_pawn) {
            reset_state();
            continue;
        }

        int32_t shots_fired = process->read<int32_t>(local_pawn + offsets::pawn::m_iShotsFired);
        if (shots_fired < 1) {
            reset_state();
            continue;
        }

        uintptr_t aim_punch_service = process->read<uintptr_t>(local_pawn + offsets::pawn::m_pAimPunchServices);
        if (!aim_punch_service) {
            reset_state();
            continue;
        }

        auto now = std::chrono::steady_clock::now();
        float dt = 0.001f;
        if (haveDt) {
            dt = std::chrono::duration<float>(now - lastFrame).count();
            dt = std::clamp(dt, 0.0005f, 0.05f);
        }
        lastFrame = now;
        haveDt = true;

        Vec3_t punch3d = process->read<Vec3_t>(aim_punch_service + offsets::pawn::m_predictableBaseAngle);

        LONG moveX = 0, moveY = 0;
        if (rcs.Step({ punch3d.x, punch3d.y }, dt, moveX, moveY) && NtUserSendInput) {
            INPUT input = {};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = moveX;
            input.mi.dy = moveY;
            NtUserSendInput(1, &input, sizeof(INPUT));
        }
    }
}
