/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#ifndef DEF_TOC_H
#define DEF_TOC_H

#define DataHeader "TC"

Position const LesserChampLoc[18] =
{
    // Horde Lesser Champions
    { 715.12f, 612.69f, 411.77f },
    { 715.97f, 618.20f, 411.56f },
    { 716.55f, 623.53f, 411.79f },
    { 717.10f, 627.32f, 411.83f },
    { 717.91f, 632.39f, 411.87f },
    { 718.54f, 636.32f, 411.89f },
    { 718.82f, 641.95f, 411.90f },
    { 719.28f, 645.82f, 411.91f },
    { 719.86f, 650.74f, 411.92f },
    // Alliance Lesser Champions
    { 774.66f, 600.71f, 411.71f },
    { 775.31f, 604.16f, 411.75f },
    { 775.96f, 607.60f, 411.76f },
    { 776.95f, 612.87f, 411.76f },
    { 777.60f, 616.31f, 411.76f },
    { 778.22f, 619.64f, 411.71f },
    { 779.11f, 624.34f, 411.83f },
    { 779.69f, 627.44f, 411.87f },
    { 780.34f, 630.89f, 411.89f }
};

Position const GrandChampFinalLoc[6] =
{
    // Horde Grand Champions
    { 706.49f, 618.48f, 411.88f },
    { 709.49f, 634.63f, 411.92f },
    { 713.13f, 651.33f, 412.39f },
    // Alliance Grand Champions
    { 780.96f, 603.13f, 411.84f },
    { 783.76f, 615.98f, 411.84f },
    { 785.75f, 625.89f, 411.89f }
};

float const hordeOrientation = 6.175f;
float const allianceOrientation = 2.982f;

enum Data
{
    BOSS_GRAND_CHAMPIONS,
    BOSS_ARGENT_CHALLENGE_E,
    BOSS_ARGENT_CHALLENGE_P,
    BOSS_BLACK_KNIGHT,
    DATA_LESSER_CHAMPIONS_PREPARE,
    DATA_LESSER_CHAMPIONS_DEFEATED,
    DATA_START,
    DATA_PLAYERS_TEAM,
    DATA_ARGENT_SOLDIER_DEFEATED
};

enum Data64
{
    DATA_ANNOUNCER,
    DATA_MAIN_GATE,

    DATA_GRAND_CHAMPION_VEHICLE_1,
    DATA_GRAND_CHAMPION_VEHICLE_2,
    DATA_GRAND_CHAMPION_VEHICLE_3,

    DATA_GRAND_CHAMPION_1,
    DATA_GRAND_CHAMPION_2,
    DATA_GRAND_CHAMPION_3,

    DATA_TIRION,
    DATA_VARIAN,
    DATA_JAINA,
    DATA_GARROSH,
    DATA_THRALL
};

enum Waypoints
{
    WAYPOINT_MAP_BOSS_1         = 1,
    WAYPOINT_MAP_BOSS_2,
    WAYPOINT_MAP_BOSS_3,
    WAYPOINT_MAP_ADDS
};

enum CreatureIds
{
    // Horde Champions
    NPC_MOKRA                   = 35572,
    NPC_ERESSEA                 = 35569,
    NPC_RUNOK                   = 35571,
    NPC_ZULTORE                 = 35570,
    NPC_VISCERI                 = 35617,

    // Alliance Champions
    NPC_JACOB                   = 34705,
    NPC_AMBROSE                 = 34702,
    NPC_COLOSOS                 = 34701,
    NPC_JAELYNE                 = 34657,
    NPC_LANA                    = 34703,

    NPC_EADRIC                  = 35119,
    NPC_PALETRESS               = 34928,

    NPC_ARGENT_LIGHWIELDER      = 35309,
    NPC_ARGENT_MONK             = 35305,
    NPC_PRIESTESS               = 35307,

    NPC_BLACK_KNIGHT            = 35451,

    NPC_RISEN_JAEREN            = 35545,
    NPC_RISEN_ARELAS            = 35564,

    NPC_JAEREN                  = 35004,
    NPC_ARELAS                  = 35005,

    NPC_TIRION                  = 34996,
    NPC_JAINA                   = 34992,
    NPC_VARIAN                  = 34990,
    NPC_THRALL                  = 34994,
    NPC_GARROSH                 = 34995,

    // Spectators
    NPC_SPECTATOR_HUMAN         = 34900,
    NPC_SPECTATOR_ORC           = 34901,
    NPC_SPECTATOR_TROLL         = 34902,
    NPC_SPECTATOR_TAUREN        = 34903,
    NPC_SPECTATOR_BELF          = 34904,
    NPC_SPECTATOR_UNDEAD        = 34905,
    NPC_SPECTATOR_DWARF         = 34906,
    NPC_SPECTATOR_DRAENEI       = 34908,
    NPC_SPECTATOR_NELF          = 34909,
    NPC_SPECTATOR_GNOME         = 34910
};

enum GameObjects
{
    GO_MAIN_GATE                = 195647,

    GO_CHAMPIONS_LOOT           = 195709,
    GO_CHAMPIONS_LOOT_H            = 195710,

    GO_EADRIC_LOOT              = 195374,
    GO_EADRIC_LOOT_H            = 195375,

    GO_PALETRESS_LOOT           = 195323,
    GO_PALETRESS_LOOT_H            = 195324
};

enum Vehicles
{
    //Grand Champions Alliance Vehicles
    VEHICLE_MARSHAL_JACOB_ALERIUS_MOUNT             = 35637,
    VEHICLE_AMBROSE_BOLTSPARK_MOUNT                 = 35633,
    VEHICLE_COLOSOS_MOUNT                           = 35768,
    VEHICLE_EVENSONG_MOUNT                          = 34658,
    VEHICLE_LANA_STOUTHAMMER_MOUNT                  = 35636,
    //Faction Champions (ALLIANCE)
    VEHICLE_DARNASSUS_CHAMPION                      = 35332,
    VEHICLE_EXODAR_CHAMPION                         = 35330,
    VEHICLE_STORMWIND_CHAMPION                      = 35328,
    VEHICLE_GNOMEREGAN_CHAMPION                     = 35331,
    VEHICLE_IRONFORGE_CHAMPION                      = 35329,
    //Grand Champions Horde Vehicles
    VEHICLE_MOKRA_SKILLCRUSHER_MOUNT                = 35638,
    VEHICLE_ERESSEA_DAWNSINGER_MOUNT                = 35635,
    VEHICLE_RUNOK_WILDMANE_MOUNT                    = 35640,
    VEHICLE_ZUL_TORE_MOUNT                          = 35641,
    VEHICLE_DEATHSTALKER_VESCERI_MOUNT              = 35634,
    //Faction Champions (HORDE)
    VEHICLE_UNDERCITY_CHAMPION                      = 35327,
    VEHICLE_THUNDER_BLUFF_CHAMPION                  = 35325,
    VEHICLE_ORGRIMMAR_CHAMPION                      = 35314,
    VEHICLE_SILVERMOON_CHAMPION                     = 35326,
    VEHICLE_SENJIN_CHAMPION                         = 35323,

    VEHICLE_ARGENT_WARHORSE_H                       = 35644,
    VEHICLE_ARGENT_WARHORSE_A                       = 36557,
    VEHICLE_ARGENT_BATTLEWORG_H                     = 36558,
    VEHICLE_ARGENT_BATTLEWORG_A                     = 36559,

    VEHICLE_BLACK_KNIGHT                            = 35491
};

enum FlagSpells
{
    SPELL_FLAG_DARNASSUS                            = 63406,
    SPELL_FLAG_EXODAR                               = 63423,
    SPELL_FLAG_GNOMEREGAN                           = 63396,
    SPELL_FLAG_IRONFORGE                            = 63427,
    SPELL_FLAG_ORGRIMMAR                            = 63433,
    SPELL_FLAG_SENJIN                               = 63399,
    SPELL_FLAG_SILVERMOON                           = 63403,
    SPELL_FLAG_STORMWIND                            = 62594,
    SPELL_FLAG_THUNDER_BLUFF                        = 63436,
    SPELL_FLAG_UNDERCITY                            = 63430

};

#endif
