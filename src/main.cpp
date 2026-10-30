#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <vector>

using namespace geode::prelude;
using Clock = std::chrono::steady_clock;

struct PendingClick {
    Clock::time_point due;
    bool down;
    int button;
    bool player1;
};

static std::vector<PendingClick> g_queue;
static bool g_replaying = false;

class $modify(CTBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool player1) {
        auto mod = Mod::get();
        if (g_replaying
            || !mod->getSettingValue<bool>("enabled")
            || (mod->getSettingValue<bool>("only-in-level") && !PlayLayer::get())) {
            return GJBaseGameLayer::handleButton(down, button, player1);
        }
        int64_t ms = mod->getSettingValue<int64_t>("delay-ms");
        if (ms <= 0) return GJBaseGameLayer::handleButton(down, button, player1);

        g_queue.push_back({Clock::now() + std::chrono::milliseconds(ms), down, button, player1});
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        if (!g_queue.empty()) {
            auto now = Clock::now();
            size_t i = 0;
            while (i < g_queue.size() && g_queue[i].due <= now) {
                auto c = g_queue[i];
                g_replaying = true;
                GJBaseGameLayer::handleButton(c.down, c.button, c.player1);
                g_replaying = false;
                i++;
            }
            g_queue.erase(g_queue.begin(), g_queue.begin() + i);
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};

class $modify(CTPlayLayer, PlayLayer) {
    void resetLevel() {
        g_queue.clear();
        PlayLayer::resetLevel();
    }
    void onQuit() {
        g_queue.clear();
        PlayLayer::onQuit();
    }
};
