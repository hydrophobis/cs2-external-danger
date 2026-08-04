#pragma once
#include "config/Config.hpp"
#include "core/memory/Memory.hpp"

class Engine {
public:
    ~Engine()                           = default;
    Engine(const Engine&)            = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&)      = delete;

   static bool Init();
   static void Shutdown();
   static ProcessModule GetClient();
   static ProcessModule GetEngine();
   static std::shared_ptr<pProcess> GetProcess(); // Refactor this so its easier to access
private:
    Engine() {};

    static Engine& GetInstance()
    {
        static Engine i{};
        return i;
    }

    bool InitImpl();
    void ShutdownImpl();

    bool AwaitProcess();
    bool AwaitModules();
    void CheckGameBuild();

    void Thread();

private:
    std::shared_ptr<pProcess> process;
    ProcessModule client;
    ProcessModule engine;
    std::thread thread_;
};