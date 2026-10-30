#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <chrono>
#include <vector>

using namespace geode::prelude;
using Clock = std::chrono::steady_clock;

struct PendingClick {
    Clock::time_point dueTime;
    uint64_t dueFrame;
    bool useFrames;
    bool down;
    int button;
    bool player1;
};

static std::vector<PendingClick> g_queue;
static bool g_replaying = false;
static uint64_t g_frame = 0;

class $modify(CTBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool player1) {
        auto mod = Mod::get();
        if (g_replaying
            || !mod->getSettingValue<bool>("enabled")
            || (mod->getSettingValue<bool>("only-in-level") && !PlayLayer::get())) {
            return GJBaseGameLayer::handleButton(down, button, player1);
        }

        bool useFrames = mod->getSettingValue<bool>("use-frames");
        int64_t ms = mod->getSettingValue<int64_t>("delay-ms");
        int64_t frames = mod->getSettingValue<int64_t>("delay-frames");

        if ((useFrames && frames <= 0) || (!useFrames && ms <= 0)) {
            return GJBaseGameLayer::handleButton(down, button, player1);
        }

        g_queue.push_back({
            Clock::now() + std::chrono::milliseconds(useFrames ? 0 : ms),
            g_frame + (useFrames ? (uint64_t)frames : 0),
            useFrames, down, button, player1
        });
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        g_frame++;
        auto now = Clock::now();
        size_t i = 0;
        while (i < g_queue.size()) {
            auto& c = g_queue[i];
            bool due = c.useFrames ? (g_frame >= c.dueFrame) : (c.dueTime <= now);
            if (!due) break;
            auto copy = c;
            g_replaying = true;
            GJBaseGameLayer::handleButton(copy.down, copy.button, copy.player1);
            g_replaying = false;
            i++;
        }
        g_queue.erase(g_queue.begin(), g_queue.begin() + i);
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

class $modify(CTPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto label = CCLabelBMFont::create("CT", "bigFont.fnt");
        auto sprite = CircleButtonSprite::create(label);
        auto btn = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(CTPauseLayer::onCTSettings)
        );
        btn->setID("click-timing-button"_spr);

        auto winSize = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setID("click-timing-menu"_spr);
        menu->setPosition({winSize.width - 35.f, 35.f});
        menu->addChild(btn);
        this->addChild(menu);
    }

    void onCTSettings(CCObject*) {
        geode::openSettingsPopup(Mod::get(), false);
    }
};
