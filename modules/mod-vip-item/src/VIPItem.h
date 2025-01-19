#ifndef DEF_VIP_ITEM_H
#define DEF_VIP_ITEM_H

#include "Player.h"
#include "Config.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "Item.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "ItemTemplate.h"
#include "QuestDef.h"
#include "ItemTemplate.h"
#include <unordered_map>
#include <vector>

class VIPItem
{
public:
    static VIPItem* instance();

};
#define sVIPItem VIPItem::instance()

#endif
