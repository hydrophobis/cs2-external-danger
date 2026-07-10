#include "AntiFlash.hpp"
#include "core/engine/Engine.hpp"
#include "core/offsets/Offsets.hpp"

void AntiFlash::Run() {
    if (!cfg::antiflash::enabled)
        return;

    auto& cache = Cache::Get();
    if (!cache.local.alive)
        return;

    auto process = Engine::GetProcess();
    if (!process)
        return;

    // Get local player pawn address (actual pawn, not controller)
    uintptr_t local_pawn = cache.local.pawn_addr;
    if (!local_pawn) {
        static bool warned = false;
        if (!warned) {
            LOGF(WARNING, "AntiFlash: pawn_addr is NULL!");
            warned = true;
        }
        return;
    }

    // Read current flash alpha
    float current_flash = process->read<float>(local_pawn + offsets::pawn::m_flFlashOverlayAlpha);
    
    // If player is flashed, reduce it
    if (current_flash > 0.0f) {
        static bool first_flash = true;
        if (first_flash) {
            LOGF(INFO, "AntiFlash: Detected flash! current_flash=%.2f, setting to %.2f", current_flash, current_flash * cfg::antiflash::opacity);
            first_flash = false;
        }
        
        // Calculate new flash value based on config
        float new_flash = current_flash * cfg::antiflash::opacity;
        
        // Write reduced flash value
        process->write<float>(local_pawn + offsets::pawn::m_flFlashOverlayAlpha, new_flash);
        
        // Also reduce duration if needed
        if (cfg::antiflash::opacity < 0.5f) {
            float duration = process->read<float>(local_pawn + offsets::pawn::m_flFlashDuration);
            if (duration > 0.1f) {
                process->write<float>(local_pawn + offsets::pawn::m_flFlashDuration, 0.1f);
            }
        }
    }
}
