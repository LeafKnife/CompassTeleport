#include "mod/MyMod.h"

#include <ll/api/event/EventBus.h>
#include <ll/api/event/ListenerBase.h>
#include <ll/api/event/player/PlayerUseItemEvent.h>
#include <ll/api/form/ModalForm.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/memory/Memory.h>
#include <ll/api/service/Bedrock.h>
#include <mc/_HeaderOutputPredefine.h>
#include <mc/deps/core/math/Vec3.h>
#include <mc/deps/core/string/HashedString.h>
#include <mc/nbt/ByteTag.h>
#include <mc/nbt/CompoundTag.h>
#include <mc/nbt/EndTag.h>
#include <mc/nbt/IntTag.h>
#include <mc/nbt/ListTag.h>
#include <mc/nbt/StringTag.h>
#include <mc/world/actor/player/Inventory.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/actor/player/PlayerInventory.h>
#include <mc/world/item/Item.h>
#include <mc/world/item/ItemStack.h>
#include <mc/world/item/LodestoneCompassItem.h>
#include <mc/world/item/SaveContextFactory.h>
#include <mc/world/item/VanillaItemNames.h>
#include <mc/world/item/components/ComponentItem.h>
#include <mc/world/level/BlockPos.h>
#include <mc/world/level/DimensionConversionData.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/PositionTrackingId.h>
#include <mc/world/level/position_trackingdb/PositionTrackingDBClient.h>
#include <mc/world/level/position_trackingdb/PositionTrackingDBServer.h>
#include <mc/world/level/position_trackingdb/TrackingRecord.h>
#include <optional>
#include <unordered_map>

using namespace ll::i18n_literals;

namespace lk::compass_teleport {
namespace {
ll::event::ListenerPtr               playerUseItemOnListener;
std::unordered_map<int, CompoundTag> cache;
} // namespace

LL_TYPE_INSTANCE_HOOK(
    broadcastUpdateToClientsHook,
    HookPriority::Normal,
    ::PositionTrackingDB::PositionTrackingDBServer,
    &PositionTrackingDB::PositionTrackingDBServer::_broadcastUpdateToClients,
    void,
    ::PositionTrackingDB::TrackingRecord const* record
) {
    auto nbt = record->serialize();
    origin(record);
    auto id   = nbt.at("id").get<StringTag>().toString();
    id        = id.substr(2);
    int index = std::stoi(id, nullptr, 16);
    if (auto it = cache.find(index); it != cache.end()) {
        it->second = nbt;
    } else {
        cache.emplace(index, nbt);
    }
}
void hookBroadcastUpdateToClients() { broadcastUpdateToClientsHook::hook(); }

void sendConfirmForm(Player& player, std::string itemType, Vec3 const& pos, DimensionType const& dimId) {
    ll::form::ModalForm fm = ll::form::ModalForm();
    fm.setTitle("gui.title.text"_tr(ll::i18n::getInstance().get(itemType, {})));
    fm.setContent("gui.content.text"_tr(dimId.id, pos.x, pos.y, pos.z));
    fm.setUpperButton("gui.button.confirm.text"_tr());
    fm.setLowerButton("gui.button.cancel.text"_tr());
    fm.sendTo(
        player,
        [pos, dimId](Player& player, ll::form::ModalFormResult result, ll::form::FormCancelReason cancel) {
            if (cancel.has_value()) return player.sendMessage("message.form.canceled.reason"_tr());
            if (result.has_value() && (bool)result.value()) {
                auto slot = player.getSelectedItemSlot();
                player.mInventory->mInventory->removeItem(slot, 1);
                player.refreshInventory();
                player.teleport(pos, dimId);
            } else {
                return player.sendMessage("message.form.canceled.reason"_tr());
            }
        }
    );
}

std::optional<Record> parseTrackingRecord(CompoundTag& nbt) {
    try {
        auto  dim    = nbt.at("dim").get<IntTag>().data;
        auto& posNbt = nbt.at("pos").get<ListTag>();

        Vec3 pos{
            posNbt.get(0)->as_ptr<IntTag>()->data,
            posNbt.get(1)->as_ptr<IntTag>()->data,
            posNbt.get(2)->as_ptr<IntTag>()->data
        };
        auto& status = nbt.at("status").get<ByteTag>().data;
        return Record{dim, pos, status};
    } catch (const std::out_of_range&) {
        return std::nullopt;
    };
}

void listenEvents() {
    auto& evBus = ll::event::EventBus::getInstance();
    // auto& logger = lk::MyMod::getInstance().getSelf().getLogger();
    playerUseItemOnListener =
        evBus.emplaceListener<ll::event::PlayerUseItemEvent>([](ll::event::PlayerUseItemEvent& event) {
            auto&      player   = event.self();
            ItemStack& item     = event.item();
            auto       typeName = item.getTypeName();
            if (typeName == VanillaItemNames::LodestoneCompass().getString()) {
                auto nbt = item.save(*SaveContextFactory::createCloneSaveContext());
                // logger.info(nbt->toSnbt());
                try {
                    auto& tag            = nbt->at("tag").get<CompoundTag>();
                    auto  trackingHandle = tag.at("trackingHandle").get<IntTag>().data;
                    // logger.debug("trackingHandle {}", trackingHandle);
                    if (auto it = cache.find(trackingHandle); it != cache.end()) {
                        auto record = parseTrackingRecord(it->second);
                        if (record.has_value() && !record->status) {
                            sendConfirmForm(player, typeName, record.value().pos, record.value().dimid);
                            return;
                        }
                    }
                    player.sendMessage("message.lodestone.not_found"_tr());
                } catch (const std::out_of_range&) {
                    return;
                }
            } else if (typeName == VanillaItemNames::RecoveryCompass().getString()) {
                auto pos = player.getLastDeathPos();
                auto dim = player.getLastDeathDimension();
                if (pos.has_value() && dim.has_value()) {
                    sendConfirmForm(player, typeName, pos.value(), dim.value());
                }
            } else if (typeName == VanillaItemNames::Compass().getString()) {
                auto respawnPoint = player.getExpectedSpawnPosition();
                auto dimType      = player.getExpectedSpawnDimensionId();
                sendConfirmForm(player, typeName, respawnPoint, dimType);
            }
        });
}

void removeEvents() {
    auto& evBus = ll::event::EventBus::getInstance();
    evBus.removeListener(playerUseItemOnListener);
}
} // namespace lk::compass_teleport