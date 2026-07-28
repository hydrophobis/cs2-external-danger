#include "Cache.hpp"

#include "core/engine/Engine.hpp" // Circular dep
#include "core/offsets/Dumper.hpp"
#include "core/vischeck/VisCheckManager.h"

bool Cache::Refresh() {
    return Get().RefreshImpl();
}

Snapshot Cache::CopySnapshot() {
    std::lock_guard<std::mutex> lock(Get().mtx);
    return {
        Get().game,
        Get().bomb,
        Get().local,
        Get().globals,
        Get().players
    };
}

bool Cache::RefreshImpl() {
    auto p = Engine::GetProcess();
    auto client = Engine::GetClient();

    if (!p)
        return false;

    auto now = steady_clock::now();

    // Without this, we are pointless :c
    // Its just calling game.UpdateMatrix() which has to bee updated as fast as possible
    if (!game.Update())
        return false;

#ifdef _DEBUG
    // Testing performance
    if (now - last < (cfg::dev::cache_refresh_rate * 1ms)) 
        return true;
#else
    // Just refresh every 5ms good for most people
    if (now - last < 5ms) 
        return true; // All good
#endif

    game.UpdateEntityList();
    globals.Update();
    bomb.Update();

    // Handle map change for LOS vischeck
    {
        static std::string lastMap;
        std::string currentMap(globals.map_name);
        if (currentMap != lastMap && !currentMap.empty()) {
            VisCheckManager::OnMapChanged(globals.map_name);
            lastMap = currentMap;
        }
    }

    std::vector<Player> scan;
    scan.reserve(globals.max_clients);
    for (int i = 0; i < globals.max_clients; i++) {
        auto player = Player(i, game.entity_list, game.list_entry);

        if (!player.Update())
            continue;

        if (player.localplayer)
            this->local = player;

        player.has_c4 = (bomb.carrier != 0 && (uintptr_t)player.pawn_controller_addr == bomb.carrier);

        // TODO: Handle or at least alert, in case of multiple lp
        //if (player.localplayer && (this->local.index == -1 || this->local.index == player.index))
        //    this->local = player;
        //else if (player.localplayer)
        //    LOGF(FATAL, "Offset missmatch, initial({}) current({}) there are more than one local players, update needed", this->local.index, player.index);
    
        scan.push_back(player);
    }

    // Update visibility using real ray-traced LOS (eye -> head/chest bones)
    {
        const bool visReady = VisCheckManager::IsReady();
        const Vec3_t eye_pos = this->local.bone_list.size() > bone_index::head
            ? this->local.bone_list[bone_index::head].pos
            : this->local.pos + Vec3_t(0, 0, 64.f);

        static uint8_t vis_hold[64]{};

        for (auto& player : scan) {
            if (player.localplayer || !player.alive) {
                player.visible = false;
                continue;
            }

            if (!visReady || eye_pos.zero()) {
                player.visible = player.spotted;
                continue;
            }

            if (player.bone_list.size() > bone_index::pelvis) {
                const auto& bones = player.bone_list;
                bool has_los = VisCheckManager::IsVisible(eye_pos, bones[bone_index::head].pos)
                    || VisCheckManager::IsVisible(eye_pos, bones[bone_index::chest].pos)
                    || VisCheckManager::IsVisible(eye_pos, bones[bone_index::shoulder_L].pos)
                    || VisCheckManager::IsVisible(eye_pos, bones[bone_index::shoulder_R].pos)
                    || VisCheckManager::IsVisible(eye_pos, bones[bone_index::pelvis].pos);

                auto& h = vis_hold[player.index & 63];
                if (has_los)
                    h = 6;
                else if (h)
                    --h;

                player.visible = h > 0;
            } else {
                player.visible = player.spotted;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        players = std::move(scan);

        duration = duration_cast<std::chrono::milliseconds>(last - now);
        last = now;
    }

    return true;
}
