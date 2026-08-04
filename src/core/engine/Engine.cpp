#include "Engine.hpp"

#include "core/offsets/Dumper.hpp"
#include "core/engine/cache/Cache.hpp"

bool Engine::Init() {
    return GetInstance().InitImpl();
}

ProcessModule Engine::GetClient() {
    return GetInstance().client;
}

ProcessModule Engine::GetEngine() {
    return GetInstance().engine;
}

std::shared_ptr<pProcess> Engine::GetProcess() {
    return GetInstance().process;
}

bool Engine::InitImpl() {
    process = std::make_shared<pProcess>();

    if (!this->AwaitProcess()) {
        LOGF(FATAL, "Could not find process, please make sure the game is open");
        return false;
    }

    if (!this->AwaitModules()) {
        LOGF(FATAL, "Game took too long to load, please open me again once its fully loaded");
        return false;
    }

    if (!Dumper::Init()) {
        LOGF(FATAL, "Failed to dump game offsets");
        return false;
    }

    this->CheckGameBuild();

    if (!Config::Read())
        LOGF(WARNING, "Failed to parse config, using default values");

#ifdef _DEBUG
    if (!cfg::dev::console)
        LogHelper::Free();
#endif

    thread_ = std::thread(&Engine::Thread, this);

    LOGF(INFO, "Successfully initialized engine...");
    return true;
}

void Engine::Shutdown() {
    GetInstance().ShutdownImpl();
}

void Engine::ShutdownImpl() {
    if (thread_.joinable())
        thread_.join();
}

void Engine::CheckGameBuild() {
    int build = process->read<int>(this->engine.base + offsets::buildNumber);

    if (build <= 0) {
        LOGF(WARNING, "Could not read the game build number, offsets may be out of date");
        return;
    }

    if (build != offsets::dumpedBuildNumber) {
        LOGF(WARNING,
            "Game build is {} but offsets were dumped for {}. Features may misbehave, "
            "run 'python tools/update_offsets.py' to refresh them, and maybe make a PR cuz im lazy",
            build, offsets::dumpedBuildNumber
        );
        return;
    }

    LOGF(VERBOSE, "Game build {} matches the dumped offsets", build);
}

void Engine::Thread() {
    while (app::running) {
        auto start = steady_clock::now();

        Cache::Refresh();

        if (cfg::settings::free_cpu)
            std::this_thread::sleep_until(start + 1ms);
    }
}

bool Engine::AwaitProcess() {
    if (!process || process->handle_) // Process not initialized, or already attached
        return false;

    do {
        if (process->AttachProcess("cs2.exe"))
            break;

        if (process->pid_ && !process->handle_) {
            LOGF(FATAL, "Insufficient permissions to open a handle to the process. Try running as Administrator.");
            return false;
        }

        static int attempts = 0;

        if (!attempts)
            LOGF(INFO, "Waiting 50s for the game to open...");

        if (attempts > 10)
            return false;
        attempts++;

        std::this_thread::sleep_for(5s);
    } while (true);

    return true;
}

bool Engine::AwaitModules() {
    if (!process || !process->handle_) // Process not initialized, or not attached
        return false;

    LOGF(INFO, "Waiting for the game to open...");

    do {
        this->client = process->GetModule("client.dll");
        this->engine = process->GetModule("engine2.dll");

        if (this->client.base && this->engine.base)
            break;

        static int attempts = 0;
        if (attempts > 10)
            return false;
        attempts++;

        std::this_thread::sleep_for(5s);
    } while (true);

    return true;
}
