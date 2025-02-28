#include "mod/MyMod.h"

#include <ll/api/io/LogLevel.h>
#include <ll/api/mod/RegisterHelper.h>

namespace lk {

MyMod& MyMod::getInstance() {
    static MyMod instance;
    return instance;
}

bool MyMod::load() {
    getSelf().getLogger().setLevel(ll::io::LogLevel::Debug);
    getSelf().getLogger().debug("Loading...");
    auto res = ll::i18n::getInstance().load(getSelf().getLangDir());
    // Code for loading the mod goes here.
    return true;
}

bool MyMod::enable() {
    getSelf().getLogger().debug("Enabling...");
    compass_teleport::hookBroadcastUpdateToClients();
    compass_teleport::listenEvents();
    // Code for enabling the mod goes here.
    return true;
}

bool MyMod::disable() {
    getSelf().getLogger().debug("Disabling...");
    compass_teleport::removeEvents();
    // Code for disabling the mod goes here.
    return true;
}

} // namespace lk

LL_REGISTER_MOD(lk::MyMod, lk::MyMod::getInstance());
