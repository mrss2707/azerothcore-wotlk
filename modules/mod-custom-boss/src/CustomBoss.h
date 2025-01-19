#ifndef DEF_CUSTOM_BOSS_H
#define DEF_CUSTOM_BOSS_H

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


class CustomBoss
{
public:
    static CustomBoss* instance();

    
};
#define sCustomBoss CustomBoss::instance()

#endif
