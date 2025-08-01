/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
    \ingroup u2w
*/

#include "WorldSession.h"
#include "AccountMgr.h"
#include "BattlegroundMgr.h"
#include "BanMgr.h"
#include "CharacterPackets.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Hyperlinks.h"
#include "Log.h"
#include "MapMgr.h"
#include "Metric.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "OutdoorPvPMgr.h"
#include "PacketUtilities.h"
#include "Pet.h"
#include "Player.h"
#include "QueryHolder.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "Transport.h"
#include "Tokenize.h"
#include "Vehicle.h"
#include "WardenWin.h"
#include "World.h"
#include "WorldGlobals.h"
#include "WorldPacket.h"
#include "WorldSocket.h"
#include "WorldState.h"
#include <zlib.h>

namespace
{
    std::string const DefaultPlayerName = "<none>";
}

bool MapSessionFilter::Process(WorldPacket* packet)
{
    ClientOpcodeHandler const* opHandle = opcodeTable[static_cast<OpcodeClient>(packet->GetOpcode())];

    //let's check if our has an anxiety disorder can be really processed in Map::Update()
    if (opHandle->ProcessingPlace == PROCESS_INPLACE)
        return true;

    //we do not process thread-unsafe packets
    if (opHandle->ProcessingPlace == PROCESS_THREADUNSAFE)
        return false;

    Player* player = m_pSession->GetPlayer();
    if (!player)
        return false;

    //in Map::Update() we do not process packets where player is not in world!
    return player->IsInWorld();
}

//we should process ALL packets when player is not in world/logged in
//OR packet handler is not thread-safe!
bool WorldSessionFilter::Process(WorldPacket* packet)
{
    ClientOpcodeHandler const* opHandle = opcodeTable[static_cast<OpcodeClient>(packet->GetOpcode())];

    //check if packet handler is supposed to be safe
    if (opHandle->ProcessingPlace == PROCESS_INPLACE)
        return true;

    //thread-unsafe packets should be processed in World::UpdateSessions()
    if (opHandle->ProcessingPlace == PROCESS_THREADUNSAFE)
        return true;

    //no player attached? -> our client! ^^
    Player* player = m_pSession->GetPlayer();
    if (!player)
        return true;

    //lets process all packets for non-in-the-world player
    return !player->IsInWorld();
}

/// WorldSession constructor
WorldSession::WorldSession(uint32 id, std::string&& name, uint32 accountFlags, std::shared_ptr<WorldSocket> sock, AccountTypes sec, uint8 expansion,
    time_t mute_time, LocaleConstant locale, uint32 recruiter, bool isARecruiter, bool skipQueue, uint32 TotalTime) :
    m_muteTime(mute_time),
    m_timeOutTime(0),
    AntiDOS(this),
    m_GUIDLow(0),
    _player(nullptr),
    m_Socket(sock),
    _security(sec),
    _skipQueue(skipQueue),
    _accountId(id),
    _accountName(std::move(name)),
    _accountFlags(accountFlags),
    m_expansion(expansion),
    m_total_time(TotalTime),
    _logoutTime(0),
    m_inQueue(false),
    m_playerLoading(false),
    m_playerLogout(false),
    m_playerRecentlyLogout(false),
    m_playerSave(false),
    m_sessionDbcLocale(sWorld->GetDefaultDbcLocale()),
    m_sessionDbLocaleIndex(locale),
    m_latency(0),
    m_TutorialsChanged(false),
    recruiterId(recruiter),
    isRecruiter(isARecruiter),
    m_currentVendorEntry(0),
    _calendarEventCreationCooldown(0),
    _addonMessageReceiveCount(0),
    _timeSyncClockDeltaQueue(6),
    _timeSyncClockDelta(0),
    _pendingTimeSyncRequests()
{
    memset(m_Tutorials, 0, sizeof(m_Tutorials));

    _offlineTime = 0;
    _kicked = false;

    _timeSyncNextCounter = 0;
    _timeSyncTimer = 0;

    if (sock)
    {
        m_Address = sock->GetRemoteIpAddress().to_string();
        ResetTimeOutTime(false);
        LoginDatabase.Execute("UPDATE account SET online = 1 WHERE id = {};", GetAccountId()); // One-time query
    }
}

/// WorldSession destructor
WorldSession::~WorldSession()
{
    LoginDatabase.Execute("UPDATE account SET totaltime = {} WHERE id = {}", GetTotalTime(), GetAccountId());

    ///- unload player if not unloaded
    if (_player)
        LogoutPlayer(true);

    /// - If have unclosed socket, close it
    if (m_Socket)
    {
        m_Socket->CloseSocket();
        m_Socket = nullptr;
    }

    ///- empty incoming packet queue
    WorldPacket* packet = nullptr;
    while (_recvQueue.next(packet))
        delete packet;

    LoginDatabase.Execute("UPDATE account SET online = 0 WHERE id = {};", GetAccountId());     // One-time query
}

void WorldSession::UpdateAccountFlag(uint32 flag, bool remove /*= flase*/)
{
    if (remove)
        _accountFlags &= ~flag;
    else
        _accountFlags |= flag;

    // Async update
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_SET_ACCOUNT_FLAG);
    stmt->SetData(0, _accountFlags);
    stmt->SetData(1, GetAccountId());
    LoginDatabase.Execute(stmt);
}

void WorldSession::ValidateAccountFlags()
{
    bool hasGMFlag = HasAccountFlag(ACCOUNT_FLAG_GM);

    if (IsGMAccount() && !hasGMFlag)
        UpdateAccountFlag(ACCOUNT_FLAG_GM);
    else if (hasGMFlag && !IsGMAccount())
        UpdateAccountFlag(ACCOUNT_FLAG_GM, true);
}

bool WorldSession::IsGMAccount() const
{
    return GetSecurity() >= SEC_GAMEMASTER;
}

std::string const& WorldSession::GetPlayerName() const
{
    return _player ? _player->GetName() : DefaultPlayerName;
}

std::string WorldSession::GetPlayerInfo() const
{
    std::ostringstream ss;

    ss << "[Player: ";

    if (!m_playerLoading && _player)
    {
        ss << _player->GetName() << ' ' << _player->GetGUID().ToString() << ", ";
    }

    ss << "Account: " << GetAccountId() << "]";

    return ss.str();
}

void WorldSession::SendAreaTriggerMessage(std::string_view str)
{
    std::vector<std::string_view> lines = Acore::Tokenize(str, '\n', true);
    for (std::string_view line : lines)
    {
        uint32 length = line.size() + 1;
        WorldPacket data(SMSG_AREA_TRIGGER_MESSAGE, 4 + length);
        data << length;
        data << line.data();
        SendPacket(&data);
    }
}

/// Get player guid if available. Use for logging purposes only
ObjectGuid::LowType WorldSession::GetGuidLow() const
{
    return GetPlayer() ? GetPlayer()->GetGUID().GetCounter() : 0;
}

/// Send a packet to the client
void WorldSession::SendPacket(WorldPacket const* packet)
{
    if (!m_Socket)
        return;

#if defined(ACORE_DEBUG)
    // Code for network use statistic
    static uint64 sendPacketCount = 0;
    static uint64 sendPacketBytes = 0;

    static time_t firstTime = GameTime::GetGameTime().count();
    static time_t lastTime = firstTime;                     // next 60 secs start time

    static uint64 sendLastPacketCount = 0;
    static uint64 sendLastPacketBytes = 0;

    time_t cur_time = GameTime::GetGameTime().count();

    if ((cur_time - lastTime) < 60)
    {
        sendPacketCount += 1;
        sendPacketBytes += packet->size();

        sendLastPacketCount += 1;
        sendLastPacketBytes += packet->size();
    }
    else
    {
        uint64 minTime = uint64(cur_time - lastTime);
        uint64 fullTime = uint64(lastTime - firstTime);

        LOG_DEBUG("network", "Send all time packets count: {} bytes: {} avr.count/sec: {} avr.bytes/sec: {} time: {}", sendPacketCount, sendPacketBytes, float(sendPacketCount) / fullTime, float(sendPacketBytes) / fullTime, uint32(fullTime));

        LOG_DEBUG("network", "Send last min packets count: {} bytes: {} avr.count/sec: {} avr.bytes/sec: {}", sendLastPacketCount, sendLastPacketBytes, float(sendLastPacketCount) / minTime, float(sendLastPacketBytes) / minTime);

        lastTime = cur_time;
        sendLastPacketCount = 1;
        sendLastPacketBytes = packet->wpos();               // wpos is real written size
    }
#endif                                                      // !ACORE_DEBUG

    if (!sScriptMgr->CanPacketSend(this, *packet))
    {
        return;
    }

    m_Socket->SendPacket(*packet);
}

/// Add an incoming packet to the queue
void WorldSession::QueuePacket(WorldPacket* new_packet)
{
    _recvQueue.add(new_packet);
}

/// Logging helper for unexpected opcodes
void WorldSession::LogUnexpectedOpcode(WorldPacket* packet, char const* status, const char* reason)
{
    LOG_ERROR("network.opcode", "Received unexpected opcode {} Status: {} Reason: {} from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet->GetOpcode())), status, reason, GetPlayerInfo());
}

/// Logging helper for unexpected opcodes
void WorldSession::LogUnprocessedTail(WorldPacket* packet)
{
    if (!sLog->ShouldLog("network.opcode", LogLevel::LOG_LEVEL_TRACE) || packet->rpos() >= packet->wpos())
        return;

    LOG_TRACE("network.opcode", "Unprocessed tail data (read stop at {} from {}) Opcode {} from {}",
        uint32(packet->rpos()), uint32(packet->wpos()), GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet->GetOpcode())), GetPlayerInfo());

    packet->print_storage();
}

/// Update the WorldSession (triggered by World update)
bool WorldSession::Update(uint32 diff, PacketFilter& updater)
{
    ///- Before we process anything:
    /// If necessary, kick the player because the client didn't send anything for too long
    /// (or they've been idling in character select)
    if (sWorld->getBoolConfig(CONFIG_CLOSE_IDLE_CONNECTIONS) && IsConnectionIdle() && m_Socket)
        m_Socket->CloseSocket();

    if (updater.ProcessUnsafe())
        UpdateTimeOutTime(diff);

    HandleTeleportTimeout(updater.ProcessUnsafe());

    ///- Retrieve packets from the receive queue and call the appropriate handlers
    /// not process packets if socket already closed
    WorldPacket* packet = nullptr;

    //! Delete packet after processing by default
    bool deletePacket = true;
    std::vector<WorldPacket*> requeuePackets;
    uint32 processedPackets = 0;
    time_t currentTime = GameTime::GetGameTime().count();

    constexpr uint32 MAX_PROCESSED_PACKETS_IN_SAME_WORLDSESSION_UPDATE = 150;

    while (m_Socket && _recvQueue.next(packet, updater))
    {
        OpcodeClient opcode = static_cast<OpcodeClient>(packet->GetOpcode());
        ClientOpcodeHandler const* opHandle = opcodeTable[opcode];

        METRIC_DETAILED_TIMER("worldsession_update_opcode_time", METRIC_TAG("opcode", opHandle->Name));
        LOG_DEBUG("network", "message id {} ({}) under READ", opcode, opHandle->Name);

        WorldSession::DosProtection::Policy const evaluationPolicy = AntiDOS.EvaluateOpcode(*packet, currentTime);
        switch (evaluationPolicy)
        {
            case WorldSession::DosProtection::Policy::Kick:
            case WorldSession::DosProtection::Policy::Ban:
                processedPackets = MAX_PROCESSED_PACKETS_IN_SAME_WORLDSESSION_UPDATE;
                break;
            case WorldSession::DosProtection::Policy::BlockingThrottle:
                requeuePackets.push_back(packet);
                deletePacket = false;
                processedPackets = MAX_PROCESSED_PACKETS_IN_SAME_WORLDSESSION_UPDATE;
                break;
            default:
                break;
        }

        if (evaluationPolicy == WorldSession::DosProtection::Policy::Process
            || evaluationPolicy == WorldSession::DosProtection::Policy::Log)
        {
            try
            {
                switch (opHandle->Status)
                {
                case STATUS_LOGGEDIN:
                    if (!_player)
                    {
                        // skip STATUS_LOGGEDIN opcode unexpected errors if player logout sometime ago - this can be network lag delayed packets
                        //! If player didn't log out a while ago, it means packets are being sent while the server does not recognize
                        //! the client to be in world yet. We will re-add the packets to the bottom of the queue and process them later.
                        if (!m_playerRecentlyLogout)
                        {
                            //requeuePackets.push_back(packet);
                            //deletePacket = false;

                            LOG_DEBUG("network", "Delaying processing of message with status STATUS_LOGGEDIN: No players in the world for account id {}", GetAccountId());
                        }
                    }
                    else if (_player->IsInWorld())
                    {
                        if (!sScriptMgr->CanPacketReceive(this, *packet))
                            break;

                        opHandle->Call(this, *packet);
                        LogUnprocessedTail(packet);
                    }

                    // lag can cause STATUS_LOGGEDIN opcodes to arrive after the player started a transfer
                    break;
                case STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT:
                    if (!_player && !m_playerRecentlyLogout) // There's a short delay between _player = null and m_playerRecentlyLogout = true during logout
                    {
                        LogUnexpectedOpcode(packet, "STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT",
                            "the player has not logged in yet and not recently logout");
                    }
                    else
                    {
                        // not expected _player or must checked in packet hanlder
                        if (!sScriptMgr->CanPacketReceive(this, *packet))
                            break;

                        opHandle->Call(this, *packet);
                        LogUnprocessedTail(packet);
                    }
                    break;
                case STATUS_TRANSFER:
                    if (_player && !_player->IsInWorld())
                    {
                        if (!sScriptMgr->CanPacketReceive(this, *packet))
                            break;

                        opHandle->Call(this, *packet);
                        LogUnprocessedTail(packet);
                    }
                    break;
                case STATUS_AUTHED:
                    if (m_inQueue) // prevent cheating
                        break;

                    // some auth opcodes can be recieved before STATUS_LOGGEDIN_OR_RECENTLY_LOGGOUT opcodes
                    // however when we recieve CMSG_CHAR_ENUM we are surely no longer during the logout process.
                    if (packet->GetOpcode() == CMSG_CHAR_ENUM)
                        m_playerRecentlyLogout = false;

                    if (!sScriptMgr->CanPacketReceive(this, *packet))
                        break;

                    opHandle->Call(this, *packet);
                    LogUnprocessedTail(packet);
                    break;
                case STATUS_NEVER:
                    LOG_ERROR("network.opcode", "Received not allowed opcode {} from {}",
                        GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet->GetOpcode())), GetPlayerInfo());
                    break;
                case STATUS_UNHANDLED:
                    LOG_DEBUG("network.opcode", "Received not handled opcode {} from {}",
                        GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet->GetOpcode())), GetPlayerInfo());
                    break;
                }
            }
            catch (WorldPackets::InvalidHyperlinkException const& ihe)
            {
                LOG_ERROR("network", "{} sent {} with an invalid link:\n{}", GetPlayerInfo(),
                    GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet->GetOpcode())), ihe.GetInvalidValue());

                if (sWorld->getIntConfig(CONFIG_CHAT_STRICT_LINK_CHECKING_KICK))
                {
                    KickPlayer("WorldSession::Update Invalid chat link");
                }
            }
            catch (WorldPackets::IllegalHyperlinkException const& ihe)
            {
                LOG_ERROR("network", "{} sent {} which illegally contained a hyperlink:\n{}", GetPlayerInfo(),
                    GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet->GetOpcode())), ihe.GetInvalidValue());

                if (sWorld->getIntConfig(CONFIG_CHAT_STRICT_LINK_CHECKING_KICK))
                {
                    KickPlayer("WorldSession::Update Illegal chat link");
                }
            }
            catch (WorldPackets::PacketArrayMaxCapacityException const& pamce)
            {
                LOG_ERROR("network", "PacketArrayMaxCapacityException: {} while parsing {} from {}.",
                    pamce.what(), GetOpcodeNameForLogging(static_cast<OpcodeClient>(packet->GetOpcode())), GetPlayerInfo());
            }
            catch (ByteBufferException const&)
            {
                LOG_ERROR("network", "WorldSession::Update ByteBufferException occured while parsing a packet (opcode: {}) from client {}, accountid={}. Skipped packet.", packet->GetOpcode(), GetRemoteAddress(), GetAccountId());
                if (sLog->ShouldLog("network", LogLevel::LOG_LEVEL_DEBUG))
                {
                    LOG_DEBUG("network", "Dumping error causing packet:");
                    packet->hexlike();
                }
            }
        }

        if (deletePacket)
            delete packet;

        deletePacket = true;

        processedPackets++;

        //process only a max amout of packets in 1 Update() call.
        //Any leftover will be processed in next update
        if (processedPackets > MAX_PROCESSED_PACKETS_IN_SAME_WORLDSESSION_UPDATE)
            break;
    }

    _recvQueue.readd(requeuePackets.begin(), requeuePackets.end());

    METRIC_VALUE("processed_packets", processedPackets);
    METRIC_VALUE("addon_messages", _addonMessageReceiveCount.load());
    _addonMessageReceiveCount = 0;

    if (!updater.ProcessUnsafe()) // <=> updater is of type MapSessionFilter
    {
        // Send time sync packet every 10s.
        if (_timeSyncTimer > 0)
        {
            if (diff >= _timeSyncTimer)
            {
                SendTimeSync();
            }
            else
            {
                _timeSyncTimer -= diff;
            }
        }
    }

    ProcessQueryCallbacks();

    //check if we are safe to proceed with logout
    //logout procedure should happen only in World::UpdateSessions() method!!!
    if (updater.ProcessUnsafe())
    {
        if (m_Socket && m_Socket->IsOpen() && _warden)
        {
            _warden->Update(diff);
        }

        if (ShouldLogOut(currentTime) && !m_playerLoading)
        {
            LogoutPlayer(true);
        }

        if (m_Socket && !m_Socket->IsOpen())
        {
            if (GetPlayer() && _warden)
                _warden->Update(diff);

            m_Socket = nullptr;
        }

        if (!m_Socket)
        {
            return false;                                       //Will remove this session from the world session map
        }
    }

    return true;
}

bool WorldSession::HandleSocketClosed()
{
    if (m_Socket && !m_Socket->IsOpen() && !IsKicked() && GetPlayer() && !PlayerLogout() && GetPlayer()->m_taxi.empty() && GetPlayer()->IsInWorld() && !World::IsStopped())
    {
        m_Socket = nullptr;
        GetPlayer()->TradeCancel(false);
        return true;
    }

    return false;
}

bool WorldSession::IsSocketClosed() const
{
    return !m_Socket || !m_Socket->IsOpen();
}

void WorldSession::HandleTeleportTimeout(bool updateInSessions)
{
    // pussywizard: handle teleport ack timeout
    if (m_Socket && m_Socket->IsOpen() && GetPlayer() && GetPlayer()->IsBeingTeleported())
    {
        time_t currTime = GameTime::GetGameTime().count();
        if (updateInSessions) // session update from World::UpdateSessions
        {
            if (GetPlayer()->IsBeingTeleportedFar() && GetPlayer()->GetSemaphoreTeleportFar() + sWorld->getIntConfig(CONFIG_TELEPORT_TIMEOUT_FAR) < currTime)
                while (GetPlayer() && GetPlayer()->IsBeingTeleportedFar())
                    HandleMoveWorldportAck();
        }
        else // session update from Map::Update
        {
            if (GetPlayer()->IsBeingTeleportedNear() && GetPlayer()->GetSemaphoreTeleportNear() + sWorld->getIntConfig(CONFIG_TELEPORT_TIMEOUT_NEAR) < currTime)
                while (GetPlayer() && GetPlayer()->IsInWorld() && GetPlayer()->IsBeingTeleportedNear())
                {
                    Player* plMover = GetPlayer()->m_mover->ToPlayer();
                    if (!plMover)
                        break;
                    WorldPacket pkt(MSG_MOVE_TELEPORT_ACK, 20);
                    pkt << plMover->GetPackGUID();
                    pkt << uint32(0); // flags
                    pkt << uint32(0); // time
                    HandleMoveTeleportAck(pkt);
                }
        }
    }
}

/// %Log the player out
void WorldSession::LogoutPlayer(bool save)
{
    // finish pending transfers before starting the logout
    while (_player && _player->IsBeingTeleportedFar())
        HandleMoveWorldportAck();

    m_playerLogout = true;
    m_playerSave = save;

    if (_player)
    {
        //! Call script hook before other logout events
        sScriptMgr->OnPlayerBeforeLogout(_player);

        if (ObjectGuid lguid = _player->GetLootGUID())
            DoLootRelease(lguid);

        ///- If the player just died before logging out, make him appear as a ghost
        //FIXME: logout must be delayed in case lost connection with client in time of combat
        if (_player->GetDeathTimer())
        {
            _player->getHostileRefMgr().deleteReferences(true);
            _player->BuildPlayerRepop();
            _player->RepopAtGraveyard();
        }
        else if (_player->HasSpiritOfRedemptionAura())
        {
            // this will kill character by SPELL_AURA_SPIRIT_OF_REDEMPTION
            _player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
            _player->KillPlayer();
            _player->BuildPlayerRepop();
            _player->RepopAtGraveyard();
        }
        else if (_player->HasPendingBind())
        {
            _player->RepopAtGraveyard();
            _player->SetPendingBind(0, 0);
        }

        // pussywizard: leave whole bg on logout (character stays ingame when necessary)
        // pussywizard: GetBattleground() checked inside
        _player->LeaveBattleground();

        // pussywizard: checked first time
        if (!_player->IsBeingTeleportedFar() && !_player->m_InstanceValid && !_player->IsGameMaster())
            _player->RepopAtGraveyard();

        sOutdoorPvPMgr->HandlePlayerLeaveZone(_player, _player->GetZoneId());
        sWorldState->HandlePlayerLeaveZone(_player, static_cast<AreaTableIDs>(_player->GetZoneId()));

        // pussywizard: remove from battleground queues on logout
        for (int i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
            if (BattlegroundQueueTypeId bgQueueTypeId = _player->GetBattlegroundQueueTypeId(i))
            {
                // track if player logs out after invited to join BG
                if (_player->IsInvitedForBattlegroundInstance())
                {
                    if (sWorld->getBoolConfig(CONFIG_BATTLEGROUND_TRACK_DESERTERS))
                    {
                        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_DESERTER_TRACK);
                        stmt->SetData(0, _player->GetGUID().GetCounter());
                        stmt->SetData(1, BG_DESERTION_TYPE_INVITE_LOGOUT);
                        CharacterDatabase.Execute(stmt);
                    }

                    sScriptMgr->OnPlayerBattlegroundDesertion(_player, BG_DESERTION_TYPE_INVITE_LOGOUT);
                }

                if (bgQueueTypeId >= BATTLEGROUND_QUEUE_2v2 && bgQueueTypeId < MAX_BATTLEGROUND_QUEUE_TYPES && _player->IsInvitedForBattlegroundQueueType(bgQueueTypeId))
                    sScriptMgr->OnPlayerBattlegroundDesertion(_player, ARENA_DESERTION_TYPE_INVITE_LOGOUT);

                _player->RemoveBattlegroundQueueId(bgQueueTypeId);
                sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId).RemovePlayer(_player->GetGUID(), true);
            }

        ///- If the player is in a guild, update the guild roster and broadcast a logout message to other guild members
        if (Guild* guild = sGuildMgr->GetGuildById(_player->GetGuildId()))
            guild->HandleMemberLogout(this);

        ///- Remove pet
        _player->RemovePet(nullptr, PET_SAVE_AS_CURRENT);

        // pussywizard: on logout remove auras that are removed at map change (before saving to db)
        // there are some positive auras from boss encounters that can be kept by logging out and logging in after boss is dead, and may be used on next bosses
        _player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CHANGE_MAP);

        if (Group *group = _player->GetGroupInvite())
            sWorld->getBoolConfig(CONFIG_LEAVE_GROUP_ON_LOGOUT)
                ? _player->UninviteFromGroup()  // Can disband group.
                : group->RemoveInvite(_player); // Just removes invite.

        // remove player from the group if he is:
        // a) in group; b) not in raid group; c) logging out normally (not being kicked or disconnected) d) LeaveGroupOnLogout is enabled
        if (_player->GetGroup() && !_player->GetGroup()->isRaidGroup() && !_player->GetGroup()->isLFGGroup() && m_Socket && sWorld->getBoolConfig(CONFIG_LEAVE_GROUP_ON_LOGOUT))
            _player->RemoveFromGroup();

        // pussywizard: checked second time after being removed from a group
        if (!_player->IsBeingTeleportedFar() && !_player->m_InstanceValid && !_player->IsGameMaster())
            _player->RepopAtGraveyard();

        // Repop at Graveyard or other player far teleport will prevent saving player because of not present map
        // Teleport player immediately for correct player save
        while (_player && _player->IsBeingTeleportedFar())
            HandleMoveWorldportAck();

        ///- empty buyback items and save the player in the database
        // some save parts only correctly work in case player present in map/player_lists (pets, etc)
        if (save)
        {
            uint32 eslot;
            for (int j = BUYBACK_SLOT_START; j < BUYBACK_SLOT_END; ++j)
            {
                eslot = j - BUYBACK_SLOT_START;
                _player->SetGuidValue(PLAYER_FIELD_VENDORBUYBACK_SLOT_1 + (eslot * 2), ObjectGuid::Empty);
                _player->SetUInt32Value(PLAYER_FIELD_BUYBACK_PRICE_1 + eslot, 0);
                _player->SetUInt32Value(PLAYER_FIELD_BUYBACK_TIMESTAMP_1 + eslot, 0);
            }
            _player->SaveToDB(false, true);
        }

        ///- Leave all channels before player delete...
        _player->CleanupChannels();

        //! Send update to group and reset stored max enchanting level
        if (_player->GetGroup())
        {
            _player->GetGroup()->SendUpdate();
            _player->GetGroup()->ResetMaxEnchantingLevel();

            if (_player->GetMap()->IsDungeon() || _player->GetMap()->IsRaidOrHeroicDungeon())
            {
                Map::PlayerList const &playerList = _player->GetMap()->GetPlayers();
                if (playerList.IsEmpty())
                    _player->TeleportToEntryPoint();
            }
        }

        //! Broadcast a logout message to the player's friends
        sSocialMgr->SendFriendStatus(_player, FRIEND_OFFLINE, _player->GetGUID(), true);
        sSocialMgr->RemovePlayerSocial(_player->GetGUID());

        //! Call script hook before deletion
        sScriptMgr->OnPlayerLogout(_player);

        METRIC_EVENT("player_events", "Logout", _player->GetName());

        LOG_INFO("entities.player", "Account: {} (IP: {}) Logout Character:[{}] ({}) Level: {}",
            GetAccountId(), GetRemoteAddress(), _player->GetName(), _player->GetGUID().ToString(), _player->GetLevel());

        //! Remove the player from the world
        // the player may not be in the world when logging out
        // e.g if he got disconnected during a transfer to another map
        // calls to GetMap in this case may cause crashes
        _player->CleanupsBeforeDelete();
        if (Map* _map = _player->FindMap())
        {
            _map->RemovePlayerFromMap(_player, true);
            _map->AfterPlayerUnlinkFromMap();
        }

        SetPlayer(nullptr); // pointer already deleted

        //! Send the 'logout complete' packet to the client
        //! Client will respond by sending 3x CMSG_CANCEL_TRADE, which we currently dont handle
        SendPacket(WorldPackets::Character::LogoutComplete().Write());
        LOG_DEBUG("network", "SESSION: Sent SMSG_LOGOUT_COMPLETE Message");

        //! Since each account can only have one online character at any given time, ensure all characters for active account are marked as offline
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ACCOUNT_ONLINE);
        stmt->SetData(0, GetAccountId());
        CharacterDatabase.Execute(stmt);
    }

    m_playerLogout = false;
    m_playerSave = false;
    m_playerRecentlyLogout = true;
    SetLogoutStartTime(0);
}

/// Kick a player out of the World
void WorldSession::KickPlayer(std::string const& reason, bool setKicked)
{
    if (m_Socket)
    {
        LOG_INFO("network.kick", "Account: {} Character: '{}' {} kicked with reason: {}", GetAccountId(), _player ? _player->GetName() : "<none>",
            _player ? _player->GetGUID().ToString() : "", reason);

        m_Socket->CloseSocket();
    }

    if (setKicked)
        SetKicked(true); // pussywizard: the session won't be left ingame for 60 seconds and to also kick offline session
}

bool WorldSession::ValidateHyperlinksAndMaybeKick(std::string_view str)
{
    if (Acore::Hyperlinks::CheckAllLinks(str))
        return true;

    LOG_ERROR("network", "Player {} {} sent a message with an invalid link:\n{}", GetPlayer()->GetName(),
        GetPlayer()->GetGUID().ToString(), str);

    if (sWorld->getIntConfig(CONFIG_CHAT_STRICT_LINK_CHECKING_KICK))
        KickPlayer("WorldSession::ValidateHyperlinksAndMaybeKick Invalid chat link");

    return false;
}

bool WorldSession::DisallowHyperlinksAndMaybeKick(std::string_view str)
{
    if (str.find('|') == std::string_view::npos)
        return true;

    LOG_ERROR("network", "Player {} {} sent a message which illegally contained a hyperlink:\n{}", GetPlayer()->GetName(),
        GetPlayer()->GetGUID().ToString(), str);

    if (sWorld->getIntConfig(CONFIG_CHAT_STRICT_LINK_CHECKING_KICK))
        KickPlayer("WorldSession::DisallowHyperlinksAndMaybeKick Illegal chat link");

    return false;
}

std::string WorldSession::GetAcoreString(uint32 entry) const
{
    return sObjectMgr->GetAcoreString(entry, GetSessionDbLocaleIndex());
}

std::string const* WorldSession::GetModuleString(std::string module, uint32 id) const
{
    return sObjectMgr->GetModuleString(module, id, GetSessionDbLocaleIndex());
}

void WorldSession::Handle_NULL(WorldPacket& null)
{
    LOG_ERROR("network.opcode", "Received unhandled opcode {} from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeClient>(null.GetOpcode())), GetPlayerInfo());
}

void WorldSession::Handle_EarlyProccess(WorldPacket& recvPacket)
{
    LOG_ERROR("network.opcode", "Received opcode {} that must be processed in WorldSocket::ReadDataHandler from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeClient>(recvPacket.GetOpcode())), GetPlayerInfo());
}

void WorldSession::Handle_ServerSide(WorldPacket& recvPacket)
{
    LOG_ERROR("network.opcode", "Received server-side opcode {} from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeServer>(recvPacket.GetOpcode())), GetPlayerInfo());
}

void WorldSession::Handle_Deprecated(WorldPacket& recvPacket)
{
    LOG_ERROR("network.opcode", "Received deprecated opcode {} from {}",
        GetOpcodeNameForLogging(static_cast<OpcodeClient>(recvPacket.GetOpcode())), GetPlayerInfo());
}

void WorldSession::SendAuthWaitQueue(uint32 position)
{
    if (position == 0)
    {
        WorldPacket packet(SMSG_AUTH_RESPONSE, 1);
        packet << uint8(AUTH_OK);
        SendPacket(&packet);
    }
    else
    {
        WorldPacket packet(SMSG_AUTH_RESPONSE, 6);
        packet << uint8(AUTH_WAIT_QUEUE);
        packet << uint32(position);
        packet << uint8(0);                                 // unk
        SendPacket(&packet);
    }
}

void WorldSession::LoadAccountData(PreparedQueryResult result, uint32 mask)
{
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
        if (mask & (1 << i))
            m_accountData[i] = AccountData();

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 type = fields[0].Get<uint8>();
        if (type >= NUM_ACCOUNT_DATA_TYPES)
        {
            LOG_ERROR("network", "Table `{}` have invalid account data type ({}), ignore.", mask == GLOBAL_CACHE_MASK ? "account_data" : "character_account_data", type);
            continue;
        }

        if ((mask & (1 << type)) == 0)
        {
            LOG_ERROR("network", "Table `{}` have non appropriate for table  account data type ({}), ignore.", mask == GLOBAL_CACHE_MASK ? "account_data" : "character_account_data", type);
            continue;
        }

        m_accountData[type].Time = time_t(fields[1].Get<uint32>());
        m_accountData[type].Data = fields[2].Get<std::string>();
    } while (result->NextRow());
}

void WorldSession::SetAccountData(AccountDataType type, time_t tm, std::string const& data)
{
    uint32 id = 0;
    CharacterDatabaseStatements index;
    if ((1 << type) & GLOBAL_CACHE_MASK)
    {
        id = GetAccountId();
        index = CHAR_REP_ACCOUNT_DATA;
    }
    else
    {
        // _player can be nullptr and packet received after logout but m_GUID still store correct guid
        if (!m_GUIDLow)
            return;

        id = m_GUIDLow;
        index = CHAR_REP_PLAYER_ACCOUNT_DATA;
    }

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(index);
    stmt->SetData(0, id);
    stmt->SetData(1, type);
    stmt->SetData(2, uint32(tm));
    stmt->SetData(3, data);
    CharacterDatabase.Execute(stmt);

    m_accountData[type].Time = tm;
    m_accountData[type].Data = data;
}

void WorldSession::SendAccountDataTimes(uint32 mask)
{
    WorldPacket data(SMSG_ACCOUNT_DATA_TIMES, 4 + 1 + 4 + 8 * 4); // changed in WotLK
    data << uint32(GameTime::GetGameTime().count());                             // unix time of something
    data << uint8(1);
    data << uint32(mask);                                   // type mask
    for (uint32 i = 0; i < NUM_ACCOUNT_DATA_TYPES; ++i)
        if (mask & (1 << i))
            data << uint32(GetAccountData(AccountDataType(i))->Time);// also unix time
    SendPacket(&data);
}

void WorldSession::LoadTutorialsData(PreparedQueryResult result)
{
    memset(m_Tutorials, 0, sizeof(uint32) * MAX_ACCOUNT_TUTORIAL_VALUES);

    if (result)
    {
        for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        {
            m_Tutorials[i] = (*result)[i].Get<uint32>();
        }
    }

    m_TutorialsChanged = false;
}

void WorldSession::SendTutorialsData()
{
    WorldPacket data(SMSG_TUTORIAL_FLAGS, 4 * MAX_ACCOUNT_TUTORIAL_VALUES);
    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        data << m_Tutorials[i];
    SendPacket(&data);
}

void WorldSession::SaveTutorialsData(CharacterDatabaseTransaction trans)
{
    if (!m_TutorialsChanged)
        return;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_HAS_TUTORIALS);
    stmt->SetData(0, GetAccountId());
    bool hasTutorials = bool(CharacterDatabase.Query(stmt));

    stmt = CharacterDatabase.GetPreparedStatement(hasTutorials ? CHAR_UPD_TUTORIALS : CHAR_INS_TUTORIALS);

    for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
        stmt->SetData(i, m_Tutorials[i]);
    stmt->SetData(MAX_ACCOUNT_TUTORIAL_VALUES, GetAccountId());
    trans->Append(stmt);

    m_TutorialsChanged = false;
}

void WorldSession::ReadMovementInfo(WorldPacket& data, MovementInfo* mi)
{
    data >> mi->flags;
    data >> mi->flags2;
    data >> mi->time;
    data >> mi->pos.PositionXYZOStream();

    if (mi->HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
    {
        data >> mi->transport.guid.ReadAsPacked();

        data >> mi->transport.pos.PositionXYZOStream();
        data >> mi->transport.time;
        data >> mi->transport.seat;

        if (mi->HasExtraMovementFlag(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT))
            data >> mi->transport.time2;
    }

    if (mi->HasMovementFlag(MovementFlags(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || (mi->HasExtraMovementFlag(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING)))
        data >> mi->pitch;

    data >> mi->fallTime;

    if (mi->HasMovementFlag(MOVEMENTFLAG_FALLING))
    {
        data >> mi->jump.zspeed;
        data >> mi->jump.sinAngle;
        data >> mi->jump.cosAngle;
        data >> mi->jump.xyspeed;
    }

    if (mi->HasMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION))
        data >> mi->splineElevation;

    //! Anti-cheat checks. Please keep them in seperate if () blocks to maintain a clear overview.
    //! Might be subject to latency, so just remove improper flags.
#ifdef ACORE_DEBUG
#define REMOVE_VIOLATING_FLAGS(check, maskToRemove) \
    { \
        if (check) \
        { \
            LOG_DEBUG("entities.unit", "WorldSession::ReadMovementInfo: Violation of MovementFlags found ({}). " \
                "MovementFlags: {}, MovementFlags2: {} for player {}. Mask {} will be removed.", \
                STRINGIZE(check), mi->GetMovementFlags(), mi->GetExtraMovementFlags(), GetPlayer()->GetGUID().ToString(), maskToRemove); \
            mi->RemoveMovementFlag((maskToRemove)); \
        } \
    }
#else
#define REMOVE_VIOLATING_FLAGS(check, maskToRemove) \
        if (check) \
            mi->RemoveMovementFlag((maskToRemove));
#endif

    /*! This must be a packet spoofing attempt. MOVEMENTFLAG_ROOT sent from the client is not valid
        in conjunction with any of the moving movement flags such as MOVEMENTFLAG_FORWARD.
        It will freeze clients that receive this player's movement info.
    */
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ROOT),
        MOVEMENTFLAG_ROOT);

    //! Cannot hover without SPELL_AURA_HOVER
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_HOVER) && !GetPlayer()->HasHoverAura(),
        MOVEMENTFLAG_HOVER);

    //! Cannot ascend and descend at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_ASCENDING) && mi->HasMovementFlag(MOVEMENTFLAG_DESCENDING),
        MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_DESCENDING);

    //! Cannot move left and right at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_LEFT) && mi->HasMovementFlag(MOVEMENTFLAG_RIGHT),
        MOVEMENTFLAG_LEFT | MOVEMENTFLAG_RIGHT);

    //! Cannot strafe left and right at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_STRAFE_LEFT) && mi->HasMovementFlag(MOVEMENTFLAG_STRAFE_RIGHT),
        MOVEMENTFLAG_STRAFE_LEFT | MOVEMENTFLAG_STRAFE_RIGHT);

    //! Cannot pitch up and down at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_PITCH_UP) && mi->HasMovementFlag(MOVEMENTFLAG_PITCH_DOWN),
        MOVEMENTFLAG_PITCH_UP | MOVEMENTFLAG_PITCH_DOWN);

    //! Cannot move forwards and backwards at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FORWARD) && mi->HasMovementFlag(MOVEMENTFLAG_BACKWARD),
        MOVEMENTFLAG_FORWARD | MOVEMENTFLAG_BACKWARD);

    //! Cannot walk on water without SPELL_AURA_WATER_WALK
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_WATERWALKING) &&
        !GetPlayer()->HasWaterWalkAura() &&
        !GetPlayer()->HasGhostAura(),
        MOVEMENTFLAG_WATERWALKING);

    //! Cannot feather fall without SPELL_AURA_FEATHER_FALL
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FALLING_SLOW) && !GetPlayer()->HasFeatherFallAura(),
        MOVEMENTFLAG_FALLING_SLOW);

    /*! Cannot fly if no fly auras present. Exception is being a GM.
        Note that we check for account level instead of Player::IsGameMaster() because in some
        situations it may be feasable to use .gm fly on as a GM without having .gm on,
        e.g. aerial combat.
    */

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_CAN_FLY) && GetSecurity() == SEC_PLAYER && !GetPlayer()->m_mover->HasFlyAura() && !GetPlayer()->m_mover->HasIncreaseMountedFlightSpeedAura(),
        MOVEMENTFLAG_FLYING | MOVEMENTFLAG_CAN_FLY);

    //! Cannot fly and fall at the same time
    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_DISABLE_GRAVITY) && mi->HasMovementFlag(MOVEMENTFLAG_FALLING),
        MOVEMENTFLAG_FALLING);

    REMOVE_VIOLATING_FLAGS(mi->HasMovementFlag(MOVEMENTFLAG_SPLINE_ENABLED) &&
        (!GetPlayer()->movespline->Initialized() || GetPlayer()->movespline->Finalized()), MOVEMENTFLAG_SPLINE_ENABLED);

#undef REMOVE_VIOLATING_FLAGS
}

void WorldSession::WriteMovementInfo(WorldPacket* data, MovementInfo* mi)
{
    *data << mi->guid.WriteAsPacked();

    *data << mi->flags;
    *data << mi->flags2;
    *data << mi->time;
    *data << mi->pos.PositionXYZOStream();

    if (mi->HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
    {
        *data << mi->transport.guid.WriteAsPacked();

        *data << mi->transport.pos.PositionXYZOStream();
        *data << mi->transport.time;
        *data << mi->transport.seat;

        if (mi->HasExtraMovementFlag(MOVEMENTFLAG2_INTERPOLATED_MOVEMENT))
            *data << mi->transport.time2;
    }

    if (mi->HasMovementFlag(MovementFlags(MOVEMENTFLAG_SWIMMING | MOVEMENTFLAG_FLYING)) || mi->HasExtraMovementFlag(MOVEMENTFLAG2_ALWAYS_ALLOW_PITCHING))
        *data << mi->pitch;

    *data << mi->fallTime;

    if (mi->HasMovementFlag(MOVEMENTFLAG_FALLING))
    {
        *data << mi->jump.zspeed;
        *data << mi->jump.sinAngle;
        *data << mi->jump.cosAngle;
        *data << mi->jump.xyspeed;
    }

    if (mi->HasMovementFlag(MOVEMENTFLAG_SPLINE_ELEVATION))
        *data << mi->splineElevation;
}

void WorldSession::ReadAddonsInfo(ByteBuffer& data)
{
    if (data.rpos() + 4 > data.size())
        return;

    uint32 size;
    data >> size;

    if (!size)
        return;

    if (size > 0xFFFFF)
    {
        LOG_ERROR("network", "WorldSession::ReadAddonsInfo addon info too big, size {}", size);
        return;
    }

    uLongf uSize = size;

    uint32 pos = data.rpos();

    ByteBuffer addonInfo;
    addonInfo.resize(size);

    if (uncompress(addonInfo.contents(), &uSize, data.contents() + pos, data.size() - pos) == Z_OK)
    {
        try
        {
            uint32 addonsCount;
            addonInfo >> addonsCount;                         // addons count

            for (uint32 i = 0; i < addonsCount; ++i)
            {
                std::string addonName;
                uint8 enabled;
                uint32 crc, unk1;

                // check next addon data format correctness
                if (addonInfo.rpos() + 1 > addonInfo.size())
                    return;

                addonInfo >> addonName;

                addonInfo >> enabled >> crc >> unk1;

                LOG_DEBUG("network", "ADDON: Name: {}, Enabled: 0x{:x}, CRC: 0x{:x}, Unknown2: 0x{:x}", addonName, enabled, crc, unk1);

                AddonInfo addon(addonName, enabled, crc, 2, true);

                SavedAddon const* savedAddon = AddonMgr::GetAddonInfo(addonName);
                if (savedAddon)
                {
                    bool match = true;

                    if (addon.CRC != savedAddon->CRC)
                        match = false;

                    if (!match)
                        LOG_DEBUG("network", "ADDON: {} was known, but didn't match known CRC (0x{:x})!", addon.Name, savedAddon->CRC);
                    else
                        LOG_DEBUG("network", "ADDON: {} was known, CRC is correct (0x{:x})", addon.Name, savedAddon->CRC);
                }
                else
                {
                    AddonMgr::SaveAddon(addon);

                    LOG_DEBUG("network", "ADDON: {} (0x{:x}) was not known, saving...", addon.Name, addon.CRC);
                }

                /// @todo: Find out when to not use CRC/pubkey, and other possible states.
                m_addonsList.push_back(addon);
            }

            uint32 currentTime;
            addonInfo >> currentTime;
            LOG_DEBUG("network", "ADDON: CurrentTime: {}", currentTime);

            if (addonInfo.rpos() != addonInfo.size())
                LOG_DEBUG("network", "packet under-read!");
        }
        catch (ByteBufferException const& e)
        {
            LOG_ERROR("network", "Addon packet read error! {}", e.what());
        }
    }
    else
        LOG_ERROR("network", "Addon packet uncompress error!");
}

void WorldSession::SendAddonsInfo()
{
    uint8 addonPublicKey[256] =
    {
        0xC3, 0x5B, 0x50, 0x84, 0xB9, 0x3E, 0x32, 0x42, 0x8C, 0xD0, 0xC7, 0x48, 0xFA, 0x0E, 0x5D, 0x54,
        0x5A, 0xA3, 0x0E, 0x14, 0xBA, 0x9E, 0x0D, 0xB9, 0x5D, 0x8B, 0xEE, 0xB6, 0x84, 0x93, 0x45, 0x75,
        0xFF, 0x31, 0xFE, 0x2F, 0x64, 0x3F, 0x3D, 0x6D, 0x07, 0xD9, 0x44, 0x9B, 0x40, 0x85, 0x59, 0x34,
        0x4E, 0x10, 0xE1, 0xE7, 0x43, 0x69, 0xEF, 0x7C, 0x16, 0xFC, 0xB4, 0xED, 0x1B, 0x95, 0x28, 0xA8,
        0x23, 0x76, 0x51, 0x31, 0x57, 0x30, 0x2B, 0x79, 0x08, 0x50, 0x10, 0x1C, 0x4A, 0x1A, 0x2C, 0xC8,
        0x8B, 0x8F, 0x05, 0x2D, 0x22, 0x3D, 0xDB, 0x5A, 0x24, 0x7A, 0x0F, 0x13, 0x50, 0x37, 0x8F, 0x5A,
        0xCC, 0x9E, 0x04, 0x44, 0x0E, 0x87, 0x01, 0xD4, 0xA3, 0x15, 0x94, 0x16, 0x34, 0xC6, 0xC2, 0xC3,
        0xFB, 0x49, 0xFE, 0xE1, 0xF9, 0xDA, 0x8C, 0x50, 0x3C, 0xBE, 0x2C, 0xBB, 0x57, 0xED, 0x46, 0xB9,
        0xAD, 0x8B, 0xC6, 0xDF, 0x0E, 0xD6, 0x0F, 0xBE, 0x80, 0xB3, 0x8B, 0x1E, 0x77, 0xCF, 0xAD, 0x22,
        0xCF, 0xB7, 0x4B, 0xCF, 0xFB, 0xF0, 0x6B, 0x11, 0x45, 0x2D, 0x7A, 0x81, 0x18, 0xF2, 0x92, 0x7E,
        0x98, 0x56, 0x5D, 0x5E, 0x69, 0x72, 0x0A, 0x0D, 0x03, 0x0A, 0x85, 0xA2, 0x85, 0x9C, 0xCB, 0xFB,
        0x56, 0x6E, 0x8F, 0x44, 0xBB, 0x8F, 0x02, 0x22, 0x68, 0x63, 0x97, 0xBC, 0x85, 0xBA, 0xA8, 0xF7,
        0xB5, 0x40, 0x68, 0x3C, 0x77, 0x86, 0x6F, 0x4B, 0xD7, 0x88, 0xCA, 0x8A, 0xD7, 0xCE, 0x36, 0xF0,
        0x45, 0x6E, 0xD5, 0x64, 0x79, 0x0F, 0x17, 0xFC, 0x64, 0xDD, 0x10, 0x6F, 0xF3, 0xF5, 0xE0, 0xA6,
        0xC3, 0xFB, 0x1B, 0x8C, 0x29, 0xEF, 0x8E, 0xE5, 0x34, 0xCB, 0xD1, 0x2A, 0xCE, 0x79, 0xC3, 0x9A,
        0x0D, 0x36, 0xEA, 0x01, 0xE0, 0xAA, 0x91, 0x20, 0x54, 0xF0, 0x72, 0xD8, 0x1E, 0xC7, 0x89, 0xD2
    };

    WorldPacket data(SMSG_ADDON_INFO, 4);

    for (AddonsList::iterator itr = m_addonsList.begin(); itr != m_addonsList.end(); ++itr)
    {
        data << uint8(itr->State);

        uint8 crcpub = itr->UsePublicKeyOrCRC;
        data << uint8(crcpub);
        if (crcpub)
        {
            uint8 usepk = (itr->CRC != STANDARD_ADDON_CRC); // If addon is Standard addon CRC
            data << uint8(usepk);
            if (usepk)                                      // if CRC is wrong, add public key (client need it)
            {
                LOG_DEBUG("network", "ADDON: CRC (0x{:x}) for addon {} is wrong (does not match expected 0x{:x}), sending pubkey", itr->CRC, itr->Name, STANDARD_ADDON_CRC);
                data.append(addonPublicKey, sizeof(addonPublicKey));
            }

            data << uint32(0);                              /// @todo: Find out the meaning of this.
        }

        uint8 unk3 = 0;                                     // 0 is sent here
        data << uint8(unk3);
        if (unk3)
        {
            // String, length 256 (null terminated)
            data << uint8(0);
        }
    }

    m_addonsList.clear();

    AddonMgr::BannedAddonList const* bannedAddons = AddonMgr::GetBannedAddons();
    data << uint32(bannedAddons->size());
    for (AddonMgr::BannedAddonList::const_iterator itr = bannedAddons->begin(); itr != bannedAddons->end(); ++itr)
    {
        data << uint32(itr->Id);
        data.append(itr->NameMD5);
        data.append(itr->VersionMD5);
        data << uint32(itr->Timestamp);
        data << uint32(1);  // IsBanned
    }

    SendPacket(&data);
}

void WorldSession::SetPlayer(Player* player)
{
    _player = player;

    // set m_GUID that can be used while player loggined and later until m_playerRecentlyLogout not reset
    if (_player)
        m_GUIDLow = _player->GetGUID().GetCounter();
}

void WorldSession::ProcessQueryCallbacks()
{
    _queryProcessor.ProcessReadyCallbacks();
    _transactionCallbacks.ProcessReadyCallbacks();
    _queryHolderProcessor.ProcessReadyCallbacks();
}

TransactionCallback& WorldSession::AddTransactionCallback(TransactionCallback&& callback)
{
    return _transactionCallbacks.AddCallback(std::move(callback));
}

SQLQueryHolderCallback& WorldSession::AddQueryHolderCallback(SQLQueryHolderCallback&& callback)
{
    return _queryHolderProcessor.AddCallback(std::move(callback));
}

void WorldSession::InitWarden(SessionKey const& k, std::string const& os)
{
    if (os == "Win")
    {
        _warden = std::make_unique<WardenWin>();
        _warden->Init(this, k);
    }
    else if (os == "OSX")
    {
        // Disabled as it is causing the client to crash
        // _warden = new WardenMac();
        // _warden->Init(this, k);
    }
}

Warden* WorldSession::GetWarden()
{
    return &(*_warden);
}

WorldSession::DosProtection::Policy WorldSession::DosProtection::EvaluateOpcode(WorldPacket const& p, time_t const time) const
{
    AntiDosOpcodePolicy const* policy = sWorldGlobals->GetAntiDosPolicyForOpcode(p.GetOpcode());
    if (!policy)
        return WorldSession::DosProtection::Policy::Process; // Return true if there is no policy for the opcode

    uint32 const maxPacketCounterAllowed = policy->MaxAllowedCount;
    if (!maxPacketCounterAllowed)
        return WorldSession::DosProtection::Policy::Process; // Return true if there no limit for the opcode

    // packetCounter is opcodes handled in the same world second, so MaxAllowedCount is per second
    PacketCounter& packetCounter = _PacketThrottlingMap[p.GetOpcode()];
    if (packetCounter.lastReceiveTime != time)
    {
        packetCounter.lastReceiveTime = time;
        packetCounter.amountCounter = 0;
    }

    // Check if player is flooding some packets
    if (++packetCounter.amountCounter <= maxPacketCounterAllowed)
        return WorldSession::DosProtection::Policy::Process;

    if (WorldSession::DosProtection::Policy(policy->Policy) != WorldSession::DosProtection::Policy::BlockingThrottle)
    {
        LOG_WARN("network", "AntiDOS: Account {}, IP: {}, Ping: {}, Character: {}, flooding packet (opc: {} (0x{:X}), count: {})",
            Session->GetAccountId(), Session->GetRemoteAddress(), Session->GetLatency(), Session->GetPlayerName(),
            opcodeTable[static_cast<OpcodeClient>(p.GetOpcode())]->Name, p.GetOpcode(), packetCounter.amountCounter);
    }

    switch (WorldSession::DosProtection::Policy(policy->Policy))
    {
        case WorldSession::DosProtection::Policy::Kick:
        {
            LOG_INFO("network", "AntiDOS: Player {} kicked!", Session->GetPlayerName());
            Session->KickPlayer();
            break;
        }
        case WorldSession::DosProtection::Policy::Ban:
        {
            uint32 bm = sWorld->getIntConfig(CONFIG_PACKET_SPOOF_BANMODE);
            uint32 duration = sWorld->getIntConfig(CONFIG_PACKET_SPOOF_BANDURATION); // in seconds
            std::string nameOrIp = "";
            switch (bm)
            {
                case 0: // Ban account
                    (void)AccountMgr::GetName(Session->GetAccountId(), nameOrIp);
                    sBan->BanAccount(nameOrIp, std::to_string(duration), "DOS (Packet Flooding/Spoofing", "Server: AutoDOS");
                    break;
                case 1: // Ban ip
                    nameOrIp = Session->GetRemoteAddress();
                    sBan->BanIP(nameOrIp, std::to_string(duration), "DOS (Packet Flooding/Spoofing", "Server: AutoDOS");
                    break;
            }

            LOG_INFO("network", "AntiDOS: Player automatically banned for {} seconds.", duration);
            break;
        }
        case WorldSession::DosProtection::Policy::DropPacket:
        {
            LOG_INFO("network", "AntiDOS: Opcode packet {} from player {} will be dropped.", p.GetOpcode(), Session->GetPlayerName());
            break;
        }
        default: // invalid policy
            break;
    }

    return WorldSession::DosProtection::Policy(policy->Policy);
}

WorldSession::DosProtection::DosProtection(WorldSession* s) :
    Session(s) { }

void WorldSession::ResetTimeSync()
{
    _timeSyncNextCounter = 0;
    _pendingTimeSyncRequests.clear();
}

void WorldSession::SendTimeSync()
{
    WorldPacket data(SMSG_TIME_SYNC_REQ, 4);
    data << uint32(_timeSyncNextCounter);
    SendPacket(&data);

    _pendingTimeSyncRequests[_timeSyncNextCounter] = getMSTime();

    // Schedule next sync in 10 sec (except for the 2 first packets, which are spaced by only 5s)
    _timeSyncTimer = _timeSyncNextCounter == 0 ? 5000 : 10000;
    _timeSyncNextCounter++;
}

class AccountInfoQueryHolderPerRealm : public CharacterDatabaseQueryHolder
{
public:
    enum
    {
        GLOBAL_ACCOUNT_DATA = 0,
        TUTORIALS,

        MAX_QUERIES
    };

    AccountInfoQueryHolderPerRealm() { SetSize(MAX_QUERIES); }

    bool Initialize(uint32 accountId)
    {
        bool ok = true;

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_DATA);
        stmt->SetData(0, accountId);
        ok = SetPreparedQuery(GLOBAL_ACCOUNT_DATA, stmt) && ok;

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_TUTORIALS);
        stmt->SetData(0, accountId);
        ok = SetPreparedQuery(TUTORIALS, stmt) && ok;

        return ok;
    }
};

void WorldSession::InitializeSession()
{
    uint32 cacheVersion = sWorld->getIntConfig(CONFIG_CLIENTCACHE_VERSION);
    sScriptMgr->OnBeforeFinalizePlayerWorldSession(cacheVersion);

    std::shared_ptr<AccountInfoQueryHolderPerRealm> realmHolder = std::make_shared<AccountInfoQueryHolderPerRealm>();
    if (!realmHolder->Initialize(GetAccountId()))
    {
        SendAuthResponse(AUTH_SYSTEM_ERROR, false);
        return;
    }

    AddQueryHolderCallback(CharacterDatabase.DelayQueryHolder(realmHolder)).AfterComplete([this, cacheVersion](SQLQueryHolderBase const& holder)
    {
        InitializeSessionCallback(static_cast<AccountInfoQueryHolderPerRealm const&>(holder), cacheVersion);
    });
}

void WorldSession::InitializeSessionCallback(CharacterDatabaseQueryHolder const& realmHolder, uint32 clientCacheVersion)
{
    LoadAccountData(realmHolder.GetPreparedResult(AccountInfoQueryHolderPerRealm::GLOBAL_ACCOUNT_DATA), GLOBAL_CACHE_MASK);
    LoadTutorialsData(realmHolder.GetPreparedResult(AccountInfoQueryHolderPerRealm::TUTORIALS));

    if (!m_inQueue)
    {
        SendAuthResponse(AUTH_OK, true);
    }
    else
    {
        SendAuthWaitQueue(0);
    }

    SetInQueue(false);
    ResetTimeOutTime(false);

    SendAddonsInfo();
    SendClientCacheVersion(clientCacheVersion);
    SendTutorialsData();
}
