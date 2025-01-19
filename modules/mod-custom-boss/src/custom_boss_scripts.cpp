
#include <unordered_map>
#include "Transmogrification.h"
#include "Chat.h"
#include "ScriptedCreature.h"
#include "ItemTemplate.h"
#include "DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Player.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"

class custom_boss : public ItemScript
{
public:
    custom_boss() : ItemScript("custom_boss") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        WorldSession* session = player->GetSession();
        //LocaleConstant locale = session->GetSessionDbLocaleIndex();
        // Xoa menu
        ClearGossipMenuFor(player);
        // Them menu
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/ICONS/INV_Enchant_Disenchant:30:30:-18:0|t", EQUIPMENT_SLOT_END + 2, 0, "1", 0, false);
        AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:30:30:-18:0|t", EQUIPMENT_SLOT_END + 1, 0);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        WorldSession* session = player->GetSession();
        //LocaleConstant locale = session->GetSessionDbLocaleIndex();
        LOG_INFO("server.vipitem", " >>>> " + std::to_string(sender));
    }
};

class PS_CustomBoss : public PlayerScript
{
private:
    void AddToDatabase(Player* player, Item* item)
    {
        // if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_BOP_TRADEABLE) && !sTransmogrification->GetAllowTradeable())
        //     return;
        // if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_REFUNDABLE))
        //     return;
        // ItemTemplate const* itemTemplate = item->GetTemplate();
        // AddToDatabase(player, itemTemplate);
    }

    void AddToDatabase(Player* player, ItemTemplate const* itemTemplate)
    {
        // LocaleConstant locale = player->GetSession()->GetSessionDbLocaleIndex();
        // if (!sT->GetTrackUnusableItems() && !sT->SuitableForTransmogrification(player, itemTemplate))
        //     return;
        // if (itemTemplate->Class != ITEM_CLASS_ARMOR && itemTemplate->Class != ITEM_CLASS_WEAPON)
        //     return;
        // uint32 itemId = itemTemplate->ItemId;
        // uint32 accountId = player->GetSession()->GetAccountId();
        // std::string itemName = itemTemplate -> Name1;

        // // get locale item name
        // int loc_idex = player->GetSession()->GetSessionDbLocaleIndex();
        // if (ItemLocale const* il = sObjectMgr->GetItemLocale(itemId))
        //     ObjectMgr::GetLocaleString(il->Name, loc_idex, itemName);

        // std::stringstream tempStream;
        // tempStream << std::hex << ItemQualityColors[itemTemplate->Quality];
        // std::string itemQuality = tempStream.str();
        // bool showChatMessage = !(player->GetPlayerSetting("mod-transmog", SETTING_HIDE_TRANSMOG).value) && !sT->CanNeverTransmog(itemTemplate);
        // if (sT->AddCollectedAppearance(accountId, itemId))
        // {
        //     if (showChatMessage)
        //         ChatHandler(player->GetSession()).PSendSysMessage( R"(|c{}|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r {})", itemQuality, itemId, itemName, GetLocaleText(locale, "added_appearance"));

        //     CharacterDatabase.Execute( "INSERT INTO custom_unlocked_appearances (account_id, item_template_id) VALUES ({}, {})", accountId, itemId);
        // }
    }
    
    void RemoveFromDatabase(Player* player, Item* item)
    {
        // ItemTemplate const* itemTemplate = item->GetTemplate();
        // RemoveFromDatabase(player, itemTemplate);
    }
};

void AddSC_CustomBoss()
{
    new custom_boss();
}
