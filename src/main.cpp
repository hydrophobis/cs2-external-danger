/*
    CS2 External Danger - P2C (Paste-to-Cheat) Version with GUI
    
    Combat-focused features with configuration menu.
    Features: Aimbot, Triggerbot, Anti-Flash, RCS
    
    Based on cs2-external-esp by IMXNOOBX
*/

#include <iostream>

#include "updater/Updater.hpp"
#include "core/engine/Engine.hpp"
#include "gui/renderer/Renderer.hpp"
#include "core/features/Features.hpp"

#include <external/exception.hpp>

int main()
{
    c_exception_handler::setup();

    LogHelper::Init();

    LOGF(INFO, "Compiled {}, Welcome to cs2-external-danger (P2C with GUI)!", __TIMESTAMP__);
    LOGF(INFO, "Features: Aimbot, Triggerbot, Anti-Flash, RCS");

    // Needs to be ran as ADMINISTRATOR
    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
        LOGF(WARNING, "Could not set application process priority to HIGH");

    if (!Updater::Init() || !Updater::Process()) {
        LOGF(WARNING, "Updater failed to run, continuing anyway...");
    }

    if (!Engine::Init()) {
        LOGF(FATAL, "Engine failed to initialize, cannot continue execution");
        goto exit;
    }

    if (!Renderer::Init()) {
        LOGF(FATAL, "Renderer failed to initialize, cannot continue execution");
        goto exit;
    }

    LOGF(INFO, "Everything setup and ready!");
    LOGF(INFO, "Use the menu (INSERT key) to configure P2C features");

    // Main rendering loop (also runs features)
    Renderer::Thread();

exit:
    LOGF(INFO, "Shutting down...");

    app::running = false;
    Features::Shutdown();
    Engine::Shutdown();

    LogHelper::Destroy();
    std::cin.get();
}
