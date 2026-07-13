#include "Triggerbot.hpp"
#include "core/engine/Engine.hpp"
#include "core/offsets/Offsets.hpp"
#include "Aimbot.hpp"
#include <Windows.h>
#include <random>

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

void Triggerbot::Run() {
    static bool first_run = true;
    if (first_run) {
        InitNtUserSendInput();
        first_run = false;
    }

    if (!cfg::triggerbot::enabled)
        return;

    if (!cfg::aimbot::enabled || !Aimbot::is_aiming)
        return;

    auto& cache = Cache::Get();
    if (!cache.local.alive || !cache.local.pawn_addr)
        return;

    if (!(GetAsyncKeyState(cfg::triggerbot::key) & 0x8000))
        return;

    HWND gameWnd = FindWindowA(nullptr, "Counter-Strike 2");
    if (!gameWnd || GetForegroundWindow() != gameWnd)
        return;

    int delay = cfg::triggerbot::delay;
    if (cfg::triggerbot::randomization) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(-30, 30);
        delay = std::max(0, delay + dist(gen));
    }

    if (delay > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    if (!cfg::triggerbot::enabled) return;
    if (!Aimbot::is_aiming) return;
    if (!cache.local.alive) return;

    bool under_crosshair = false;
    if (cache.local.index >= 0) {
        under_crosshair = cache.local.index >= 0;
    }

    if (!under_crosshair)
        return;

    if (NtUserSendInput) {
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        NtUserSendInput(1, &input, sizeof(INPUT));

        input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        NtUserSendInput(1, &input, sizeof(INPUT));
    }
}
