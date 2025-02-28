#pragma once

#include <ll/api/i18n/I18n.h>
#include <ll/api/mod/NativeMod.h>
#include <mc/deps/core/math/Vec3.h>

namespace lk {

class MyMod {

public:
    static MyMod& getInstance();

    MyMod() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    // TODO: Implement this method if you need to unload the mod.
    // /// @return True if the mod is unloaded successfully.
    // bool unload();

private:
    ll::mod::NativeMod& mSelf;
};

namespace compass_teleport {
struct Record {
    int  dimid;
    Vec3 pos;
    int  status;
};
void hookBroadcastUpdateToClients();
void listenEvents();
void removeEvents();
} // namespace compass_teleport
} // namespace lk
