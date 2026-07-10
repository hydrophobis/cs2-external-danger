#include "Features.hpp"

void Features::Run() {
    static bool first_run = true;
    if (first_run) {
        LOGF(INFO, "P2C Features are now running!");
        Aimbot::Init(); // Initialize mouse-based aimbot thread
        first_run = false;
    }
    
    Triggerbot::Run();
    AntiFlash::Run();
    RCS::Run();
}

void Features::DrawOverlays() {
    // Mouse-based aimbot doesn't need FOV drawing (no screen-space FOV)
}
