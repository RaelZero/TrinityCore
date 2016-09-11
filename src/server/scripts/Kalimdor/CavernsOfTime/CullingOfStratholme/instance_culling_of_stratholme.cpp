/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "CreatureTextMgr.h"
#include "culling_of_stratholme.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "SpellInfo.h"

/* Culling of Stratholme encounters:
0 - Meathook
1 - Salramm the Fleshcrafter
2 - Chrono-Lord Epoch
3 - Mal'Ganis
4 - Infinite Corruptor (Heroic only)
*/

enum Events
{
    EVENT_GUARDIAN_TICK = 1,
    EVENT_CRIER_CALL_TO_GATES,
    EVENT_SCOURGE_WAVE
};

enum Entries
{
    NPC_GENERIC_BUNNY           =  28960,
    NPC_CRATE_HELPER            =  27827,
    NPC_ARTHAS                  =  26499,
    NPC_LORDAERON_CRIER         =  27913,

    NPC_DEVOURING_GHOUL         =  28249,
    NPC_ENRAGED_GHOUL           =  27729,
    NPC_NECROMANCER             =  28200,
    NPC_CRYPT_FIEND             =  27734,
    NPC_ACOLYTE                 =  27731,
    NPC_CRYPT_STALKER           =  28199,
    NPC_ABOMINATION             =  27736,

    GO_MALGANIS_GATE_2          = 187723,
    GO_EXIT_GATE                = 191788,

    SPELL_CRATES_KILL_CREDIT    =  58109
};

enum Yells
{
    CRIER_SAY_CALL_TO_GATES     = 0,
    CRIER_SAY_KINGS_SQUARE      = 1,
    CRIER_SAY_MARKET_ROW        = 2,
    CRIER_SAY_FESTIVAL_LANE     = 3,
    CRIER_SAY_ELDERS_SQUARE     = 4,
    CRIER_SAY_TOWN_HALL         = 5
};
enum WaveLocations
{
    WAVE_LOC_MIN = CRIER_SAY_KINGS_SQUARE,
    WAVE_LOC_MAX = CRIER_SAY_TOWN_HALL,
    WAVE_MARKER_MIN = WORLDSTATE_WAVE_MARKER_ES,
    WAVE_MARKER_MAX = WORLDSTATE_WAVE_MARKER_TH
};

enum Misc
{
    NUM_PLAGUE_CRATES   =  5,
    NUM_SCOURGE_WAVES   = 10,
    MAX_SPAWNS_PER_WAVE =  6,
    WAVE_MEATHOOK       =  5,
    WAVE_SALRAMM        = 10
};

Position const ChromieSummonPos[] =
{
    { 1813.298f, 1283.578f, 142.3258f, 3.878161f },
    { 2273.725f, 1483.684f, 128.7205f, 6.057528f }
};

Position const InfiniteCorruptorPos = { 2335.47f, 1262.04f, 132.921f, 1.42079f };
Position const TimeRiftPos = { 2334.626f, 1280.45f, 133.0066f, 1.727876f };
Position const GuardianOfTimePos = { 2321.489f, 1268.383f, 132.8507f, 0.418879f };

DoorData const doorData[] =
{
    { GO_MALGANIS_GATE_2, DATA_MAL_GANIS, DOOR_TYPE_ROOM },
    { GO_EXIT_GATE,       DATA_MAL_GANIS, DOOR_TYPE_PASSAGE },
    { 0,                  0,              DOOR_TYPE_ROOM } // END
};

class instance_culling_of_stratholme : public InstanceMapScript
{
    public:
        instance_culling_of_stratholme() : InstanceMapScript("instance_culling_of_stratholme", 595) { }

        typedef std::array<std::array<uint32, MAX_SPAWNS_PER_WAVE>, NUM_SCOURGE_WAVES> WaveData;
        static const WaveData _heroicWaves;

        struct WaveLocation
        {
            InstanceMisc const worldState;
            std::array<Position, MAX_SPAWNS_PER_WAVE> spawnPoints;
        };
        typedef std::array<WaveLocation, WAVE_LOC_MAX-WAVE_LOC_MIN+1> WaveLocationData;
        static const WaveLocationData _waveLocations;

        struct instance_culling_of_stratholme_InstanceMapScript : public InstanceScript
        {
            instance_culling_of_stratholme_InstanceMapScript(Map* map) : InstanceScript(map), _currentState(JUST_STARTED), _infiniteGuardianTimeout(0), _waveCount(0)
            {
                SetHeaders("CS");
                SetBossNumber(NUM_BOSS_ENCOUNTERS);
                LoadDoorData(doorData);

                _currentWorldStates[WORLDSTATE_SHOW_CRATES] = _currentWorldStates[WORLDSTATE_CRATES_REVEALED] = _currentWorldStates[WORLDSTATE_WAVE_COUNT] = _currentWorldStates[WORLDSTATE_TIME_GUARDIAN_SHOW] = _currentWorldStates[WORLDSTATE_TIME_GUARDIAN] = 0;
                _sentWorldStates = _currentWorldStates;
                _plagueCrates.reserve(NUM_PLAGUE_CRATES);
            }

            void FillInitialWorldStates(WorldPacket& data) override
            {
                for (WorldStateMap::const_iterator it = _sentWorldStates.begin(); it != _sentWorldStates.end(); ++it)
                    data << uint32(it->first) << uint32(it->second);
            }

            void WriteSaveDataMore(std::ostringstream& data) override
            {
                data << _currentState << ' ' << _infiniteGuardianTimeout;
            }

            void ReadSaveDataMore(std::istringstream& data) override
            {
                // read current instance progress from save data, then regress to the previous stable state
                uint32 state = JUST_STARTED, loadState;
                time_t infiniteGuardianTime = 0;
                data >> state;
                data >> infiniteGuardianTime; // UNIX timestamp
                switch(state)
                {
                    case JUST_STARTED:
                    default:
                        loadState = JUST_STARTED;
                        break;
                    case CRATES_IN_PROGRESS:
                        loadState = CRATES_IN_PROGRESS;
                        break;
                    case CRATES_DONE:
                        loadState = CRATES_DONE;
                        break;
                    case UTHER_TALK:
                    case PURGE_PENDING:
                    case WAVES_IN_PROGRESS:
                        loadState = PURGE_PENDING;
                        break;
                    case WAVES_DONE:
                    case TOWN_HALL:
                        loadState = WAVES_DONE;
                        break;
                    case TOWN_HALL_COMPLETE:
                    case GAUNTLET_TRANSITION:
                    case GAUNTLET_PENDING:
                    case GAUNTLET_IN_PROGRESS:
                        loadState = GAUNTLET_PENDING;
                        break;
                    case GAUNTLET_COMPLETE:
                    case MALGANIS_IN_PROGRESS:
                        loadState = GAUNTLET_COMPLETE;
                        break;
                }

                SetInstanceProgress(ProgressStates(loadState));
                
                if (infiniteGuardianTime)
                {
                    _infiniteGuardianTimeout = infiniteGuardianTime;
                    events.ScheduleEvent(EVENT_GUARDIAN_TICK, Seconds(0));
                }

                time_t timediff = (infiniteGuardianTime - time(NULL));
                std::cout << "Loaded with state " << (uint32)loadState << " and guardian timeout at " << timediff/MINUTE << " minutes " << timediff%MINUTE << " seconds  from now." << std::endl;
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_GM_OVERRIDE:
                        SetInstanceProgress(ProgressStates(data));
                        break;
                    case DATA_CRATES_START:
                        if (_currentState == JUST_STARTED)
                            SetInstanceProgress(CRATES_IN_PROGRESS);
                        break;
                    case DATA_CRATE_REVEALED:
                        if (uint32 missingCrates = missingPlagueCrates())
                            SetWorldState(WORLDSTATE_CRATES_REVEALED, NUM_PLAGUE_CRATES - missingCrates);
                        else
                            SetInstanceProgress(CRATES_DONE);
                        break;
                    case DATA_UTHER_START:
                        if (_currentState == CRATES_DONE)
                            SetInstanceProgress(UTHER_TALK);
                        break;
                    case DATA_SKIP_TO_PURGE:
                        if (_currentState <= CRATES_DONE)
                            SetInstanceProgress(PURGE_PENDING);
                        break;
                    case DATA_NOTIFY_DEATH:
                        if (_currentState == WAVES_IN_PROGRESS && !_waveSpawns.empty())
                        {
                            for (ObjectGuid const& guid : _waveSpawns)
                                if (Creature* creature = instance->GetCreature(guid))
                                    if (creature->IsAlive())
                                        return;
                            _waveSpawns.clear();

                            if (_waveCount < NUM_SCOURGE_WAVES)
                                events.ScheduleEvent(EVENT_SCOURGE_WAVE, (_waveCount == WAVE_MEATHOOK) ? Seconds(20) : Seconds(1));
                            else
                                SetInstanceProgress(WAVES_DONE);
                        }
                        break;
                    default:
                        break;
                }
            }

            void OnUnitDeath(Unit* unit) override
            {
                // @todo debug
                if (unit->ToCreature())
                    SetData(DATA_NOTIFY_DEATH, 1);
            }

            uint32 GetData(uint32 type) const override
            {
                if (type == DATA_INSTANCE_PROGRESS)
                    return _currentState;
                return 0;
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                return true;
            }

            void SetInstanceProgress(ProgressStates state)
            {
                std::cout << "Instance progress is now " << Trinity::StringFormat("0x%X",(uint32)state) << std::endl;
                ProgressStates oldState = _currentState;
                _currentState = state;

                // Notify all creatures using spawn control AI of the change so they can despawn as appropriate
                if (oldState)
                    for (ObjectGuid const& guid : _myCreatures[oldState])
                        if (Creature* creature = instance->GetCreature(guid))
                            creature->AI()->DoAction(-ACTION_PROGRESS_UPDATE);

                // Notify Arthas of the change so he can adjust
                if (Creature* arthas = instance->GetCreature(_arthasGUID))
                    arthas->AI()->DoAction(-ACTION_PROGRESS_UPDATE);

                /* World state handling */
                // Plague crates
                if (state == CRATES_IN_PROGRESS)
                {
                    SetWorldState(WORLDSTATE_SHOW_CRATES, 1, false);
                    SetWorldState(WORLDSTATE_CRATES_REVEALED, 0, false);
                }
                else if (state == CRATES_DONE)
                {
                    SetWorldState(WORLDSTATE_SHOW_CRATES, 1, false);
                    SetWorldState(WORLDSTATE_CRATES_REVEALED, NUM_PLAGUE_CRATES, false);
                }
                else
                {
                    SetWorldState(WORLDSTATE_SHOW_CRATES, 0, false);
                    SetWorldState(WORLDSTATE_CRATES_REVEALED, state == JUST_STARTED ? 0 : NUM_PLAGUE_CRATES, false);
                }
                // Scourge wave management
                if (state == WAVES_IN_PROGRESS)
                    SetWorldState(WORLDSTATE_WAVE_COUNT, _waveCount, false); // @todo
                else if (state == WAVES_DONE)
                    SetWorldState(WORLDSTATE_WAVE_COUNT, NUM_SCOURGE_WAVES, false);
                else
                    SetWorldState(WORLDSTATE_WAVE_COUNT, 0, false);

                PropagateWorldStateUpdate();

                switch (state)
                {
                    case CRATES_IN_PROGRESS:
                        break;
                    case CRATES_DONE:
                        if (Creature* bunny = instance->GetCreature(_genericBunnyGUID))
                            bunny->CastSpell(nullptr, SPELL_CRATES_KILL_CREDIT, TRIGGERED_FULL_MASK);
                        events.ScheduleEvent(EVENT_CRIER_CALL_TO_GATES, Seconds(5));
                        break;
                    case UTHER_TALK:
                        // @todo debug skip
                        if (Creature* crier = instance->GetCreature(_crierGUID))
                            crier->Talk("Stratholme AI: There should be an RP sequence here. It's not yet implemented. Forwarding the instance to start of purge...", CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, 50000.0f, (WorldObject*)nullptr);
                        SetInstanceProgress(WAVES_IN_PROGRESS);
                        break;
                    case PURGE_PENDING:
                        break;
                    case WAVES_IN_PROGRESS:
                        _waveCount = 0;
                        events.ScheduleEvent(EVENT_SCOURGE_WAVE, Seconds(1));
                        break;
                }
            }

            void Update(uint32 diff) override
            {
                if (!_infiniteGuardianTimeout && GetBossState(DATA_INFINITE_CORRUPTOR) != FAIL)
                {
                    std::cout << "debug: setting guardian timer to 10 minutes, propagating..." << std::endl;
                    _infiniteGuardianTimeout = time(NULL) + 10 * MINUTE;
                    SetWorldState(WORLDSTATE_TIME_GUARDIAN_SHOW, 1);
                    events.ScheduleEvent(EVENT_GUARDIAN_TICK, Seconds(0));
                }
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_GUARDIAN_TICK:
                        {
                            time_t secondsToGuardianDeath = _infiniteGuardianTimeout - time(NULL);
                            if (secondsToGuardianDeath <= 0)
                            {
                                _infiniteGuardianTimeout = 0;
                                SetWorldState(WORLDSTATE_TIME_GUARDIAN_SHOW, 0, false);
                                SetWorldState(WORLDSTATE_TIME_GUARDIAN, 0);
                                SetBossState(DATA_INFINITE_CORRUPTOR, FAIL);
                            }
                            else
                            {
                                SetWorldState(WORLDSTATE_TIME_GUARDIAN, (secondsToGuardianDeath + (MINUTE - 1)) / MINUTE);
                                events.Repeat(Seconds((secondsToGuardianDeath % MINUTE) + 1));
                            }
                            break;
                        }
                        case EVENT_CRIER_CALL_TO_GATES:
                            if (_currentState == CRATES_DONE)
                                if (Creature* crier = instance->GetCreature(_crierGUID))
                                    crier->AI()->Talk(CRIER_SAY_CALL_TO_GATES);
                            break;
                        case EVENT_SCOURGE_WAVE:
                        {
                            if (_currentState != WAVES_IN_PROGRESS)
                                break;
                            ++_waveCount;
                            SetWorldState(WORLDSTATE_WAVE_COUNT, _waveCount);

                            uint8 spawnLoc = urand(WAVE_LOC_MIN, WAVE_LOC_MAX);
                            WaveLocation const& spawnLocation = _waveLocations[spawnLoc - WAVE_LOC_MIN];

                            switch (_waveCount)
                            {
                                case WAVE_MEATHOOK:
                                    std::cout << "Spawn Meathook (spawn placeholder ghoul)" << std::endl;
                                    if (Creature* spawn = instance->SummonCreature(NPC_DEVOURING_GHOUL, spawnLocation.spawnPoints[0]))
                                    {
                                        _waveSpawns.push_back(spawn->GetGUID());
                                        spawn->SetName("Meathook Placeholder");
                                    }
                                    break;
                                case WAVE_SALRAMM:
                                    std::cout << "Spawn Salramm (spawn placeholder ghoul)" << std::endl;
                                    if (Creature* spawn = instance->SummonCreature(NPC_DEVOURING_GHOUL, spawnLocation.spawnPoints[0]))
                                    {
                                        _waveSpawns.push_back(spawn->GetGUID());
                                        spawn->SetName("Salramm Placeholder");
                                    }
                                    break;
                                default:
                                    if (instance->GetSpawnMode() == DUNGEON_DIFFICULTY_HEROIC)
                                    {
                                        for (uint32 i = 0; i < MAX_SPAWNS_PER_WAVE; ++i)
                                            if (uint32 entry = _heroicWaves[_waveCount - 1][i])
                                                if (Creature* spawn = instance->SummonCreature(entry, spawnLocation.spawnPoints[i]))
                                                    _waveSpawns.push_back(spawn->GetGUID());
                                    }
                                    else
                                    {
                                        for (uint32 i = 0; i <= 1; ++i)
                                            if (Creature* spawn = instance->SummonCreature(NPC_DEVOURING_GHOUL, spawnLocation.spawnPoints[i]))
                                                _waveSpawns.push_back(spawn->GetGUID());
                                    }
                                    break;
                            }

                            if (Creature* crier = instance->GetCreature(_crierGUID))
                                crier->AI()->Talk(spawnLoc);

                            for (uint32 marker = WAVE_MARKER_MIN; marker <= WAVE_MARKER_MAX; ++marker)
                                SetWorldState(InstanceMisc(marker), 0, false);
                            SetWorldState(spawnLocation.worldState, 1);

                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_GENERIC_BUNNY:
                        _genericBunnyGUID = creature->GetGUID();
                        creature->setActive(true);
                        break;
                    case NPC_CRATE_HELPER:
                        _plagueCrates.push_back(creature->GetGUID());
                        break;
                    case NPC_ARTHAS:
                        _arthasGUID = creature->GetGUID();
                        creature->setActive(true);
                        break;
                    case NPC_LORDAERON_CRIER:
                        _crierGUID = creature->GetGUID();
                        creature->setActive(true);
                        break;
                    default:
                        break;
                }
            }

            void CreatureAIHello(ObjectGuid const& me, ProgressStates myStates)
            {
                for (ProgressStates curState = JUST_STARTED; curState <= COMPLETE; curState = ProgressStates(curState << 1))
                    if (myStates & curState)
                        _myCreatures[curState].insert(me);
            }

            void CreatureAIGoodbye(ObjectGuid const& me, ProgressStates myStates)
            {
                for (ProgressStates curState = JUST_STARTED; curState <= COMPLETE; curState = ProgressStates(curState << 1))
                    if (myStates & curState)
                        _myCreatures[curState].erase(me);
            }

            void SetWorldState(InstanceMisc state, uint32 value, bool immediate = true)
            {
                std::cout << "SetWorldState " << state << " " << value << std::endl;
                _currentWorldStates[state] = value;
                if (immediate)
                    PropagateWorldStateUpdate();
            }

            void PropagateWorldStateUpdate()
            {
                std::cout << "Propagate world states" << std::endl;
                for (WorldStateMap::const_iterator it = _currentWorldStates.begin(); it != _currentWorldStates.end(); ++it)
                {
                    uint32& sent = _sentWorldStates[it->first];
                    if (sent != it->second)
                    {
                        std::cout << "Sending world state " << it->first << " (" << it->second << ")" << std::endl;
                        DoUpdateWorldState(it->first, it->second);
                        sent = it->second;
                    }
                }
            }

        private:
            EventMap events;
            ProgressStates _currentState;
            typedef std::map<InstanceMisc,uint32> WorldStateMap;
            WorldStateMap _sentWorldStates;
            WorldStateMap _currentWorldStates;
            time_t _infiniteGuardianTimeout;
            std::map<ProgressStates, std::set<ObjectGuid>> _myCreatures;

            ObjectGuid _genericBunnyGUID;
            std::vector<ObjectGuid> _plagueCrates;
            uint32 missingPlagueCrates() const
            {
                uint32 num = 0;
                for (ObjectGuid crateHelperGUID : _plagueCrates)
                    if (Creature* crateHelper = instance->GetCreature(crateHelperGUID))
                        if (crateHelper->IsAlive() && !crateHelper->AI()->GetData(DATA_CRATE_REVEALED))
                            ++num;
                return num;
            }

            ObjectGuid _arthasGUID;
            ObjectGuid _crierGUID;

            uint32 _waveCount;
            std::vector<ObjectGuid> _waveSpawns;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_culling_of_stratholme_InstanceMapScript(map);
        }
};

const instance_culling_of_stratholme::WaveData instance_culling_of_stratholme::_heroicWaves =
{{
    {{ NPC_DEVOURING_GHOUL, NPC_DEVOURING_GHOUL, NPC_DEVOURING_GHOUL }}, // wave 1
    {{ NPC_DEVOURING_GHOUL, NPC_ENRAGED_GHOUL, NPC_NECROMANCER }}, // wave 2
    {{ NPC_DEVOURING_GHOUL, NPC_ENRAGED_GHOUL, NPC_NECROMANCER, NPC_CRYPT_FIEND }}, // wave 3
    {{ NPC_NECROMANCER, NPC_CRYPT_FIEND, NPC_ACOLYTE, NPC_ACOLYTE, NPC_ACOLYTE, NPC_ACOLYTE }}, // wave 4
    {{ 0 }}, // wave 5, meathook (special)
    {{ NPC_DEVOURING_GHOUL, NPC_NECROMANCER, NPC_CRYPT_FIEND, NPC_CRYPT_STALKER }}, // wave 6
    {{ NPC_DEVOURING_GHOUL, NPC_ENRAGED_GHOUL, NPC_ENRAGED_GHOUL, NPC_ABOMINATION }}, // wave 7
    {{ NPC_DEVOURING_GHOUL, NPC_ENRAGED_GHOUL, NPC_NECROMANCER, NPC_ABOMINATION }}, // wave 8
    {{ NPC_DEVOURING_GHOUL, NPC_NECROMANCER, NPC_CRYPT_FIEND, NPC_ABOMINATION }}, // wave 9
    {{ 0 }} // wave 10, salramm (special)
}};

const instance_culling_of_stratholme::WaveLocationData instance_culling_of_stratholme::_waveLocations =
{{
    { // King's Square
        WORLDSTATE_WAVE_MARKER_KS,
        {{
            {2131.474f, 1352.615f, 131.372f, 6.10960f},
            {2131.463f, 1357.127f, 131.587f, 5.95173f},
            {2129.795f, 1345.093f, 131.194f, 0.17905f},
            {2136.235f, 1347.894f, 131.628f, 0.20262f},
            {2138.219f, 1356.240f, 132.169f, 5.95173f},
            {2140.584f, 1351.624f, 132.142f, 6.08525f}
        }}
    },
    { // Market Row
        WORLDSTATE_WAVE_MARKER_MR,
        {{
            {2229.593f, 1332.397f, 126.846f, 3.24212f},
            {2229.934f, 1329.146f, 127.057f, 3.24605f},
            {2225.028f, 1327.269f, 127.791f, 3.03792f},
            {2223.844f, 1335.282f, 127.749f, 3.47774f},
            {2222.192f, 1330.859f, 127.526f, 3.18793f},
            {2225.865f, 1331.029f, 127.007f, 3.18793f}
        }}
    },
    { // Festival Lane
        WORLDSTATE_WAVE_MARKER_FL,
        {{
            {2183.596f, 1238.823f, 136.551f, 2.16377f},
            {2181.420f, 1237.357f, 136.565f, 2.16377f},
            {2178.692f, 1237.446f, 136.694f, 1.99098f},
            {2184.980f, 1242.458f, 136.772f, 2.59181f},
            {2176.873f, 1240.463f, 136.420f, 2.10094f},
            {2181.523f, 1244.298f, 136.338f, 2.38997f}
        }}
    },
    { // Elders' Square
        WORLDSTATE_WAVE_MARKER_ES,
        {{
			{2267.003f, 1168.055f, 137.821f, 2.79050f},
            {2264.392f, 1162.145f, 137.910f, 2.39937f},
            {2262.785f, 1166.648f, 138.053f, 2.71353f},
            {2265.214f, 1170.771f, 137.972f, 2.80385f},
            {2259.745f, 1159.360f, 138.198f, 2.34047f},
            {2264.222f, 1171.708f, 138.047f, 2.82742f}
        }}
    },
    { // Town Hall
        WORLDSTATE_WAVE_MARKER_TH,
        {{
            {2351.656f, 1218.682f, 130.062f, 4.63383f},
            {2354.921f, 1218.425f, 130.280f, 4.63383f},
            {2347.516f, 1216.976f, 130.491f, 5.02496f},
            {2356.508f, 1216.656f, 130.445f, 4.29061f},
            {2346.674f, 1216.739f, 130.576f, 5.32341f},
            {2351.728f, 1214.561f, 130.255f, 4.61891f}
        }}
    }
}};

void StratholmeAIHello(InstanceScript* instance, ObjectGuid const& me, ProgressStates myStates)
{
    reinterpret_cast<instance_culling_of_stratholme::instance_culling_of_stratholme_InstanceMapScript*>(instance)->CreatureAIHello(me, myStates);

}
void StratholmeAIGoodbye(InstanceScript* instance, ObjectGuid const& me, ProgressStates myStates)
{
    reinterpret_cast<instance_culling_of_stratholme::instance_culling_of_stratholme_InstanceMapScript*>(instance)->CreatureAIGoodbye(me, myStates);
}

void AddSC_instance_culling_of_stratholme()
{
    new instance_culling_of_stratholme();
}
