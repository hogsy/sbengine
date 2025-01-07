////////////////////////////////////////////////////////////////////
//
// Darkness.spa.h
//
// Auto-generated on Monday, 13 November 2006 at 15:02:21
// XLAST project version 1.0.166.0
// SPA Compiler version 2.0.4314.0
//
////////////////////////////////////////////////////////////////////

#ifndef __THE_DARKNESS_SPA_H__
#define __THE_DARKNESS_SPA_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Title info
//

#define TITLEID_THE_DARKNESS                        0x17635423

//
// Context ids
//
// These values are passed as the dwContextId to XUserSetContext.
//

#define CONTEXT_MAP                                 0
#define CONTEXT_GAME_STYLE                          4
#define CONTEXT_GAME_SUB_MODE                       5

//
// Context values
//
// These values are passed as the dwContextValue to XUserSetContext.
//

// Values for CONTEXT_MAP

#define CONTEXT_MAP_MP01                            0
#define CONTEXT_MAP_CTF01                           1
#define CONTEXT_MAP_CTF02                           2
#define CONTEXT_MAP_MP02                            3
#define CONTEXT_MAP_MP03                            4

// Values for CONTEXT_GAME_STYLE

#define CONTEXT_GAME_STYLE_SHAPESHIFTER             0
#define CONTEXT_GAME_STYLE_DARKLINGS_VS_DARKLINGS   1
#define CONTEXT_GAME_STYLE_SURVIVOR                 2
#define CONTEXT_GAME_STYLE_DARKLINGS_VS_HUMANS      3
#define CONTEXT_GAME_STYLE_LASTHUMAN                4

// Values for CONTEXT_GAME_SUB_MODE

#define CONTEXT_GAME_SUB_MODE_TEAM_DEATHMATCH       0
#define CONTEXT_GAME_SUB_MODE_CAPTURE_THE_FLAG      1

// Values for X_CONTEXT_PRESENCE

#define CONTEXT_PRESENCE_MULTIPLAYER                0
#define CONTEXT_PRESENCE_SINGLEPLAYER               1

// Values for X_CONTEXT_GAME_MODE

#define CONTEXT_GAME_MODE_FREE_FOR_ALL              0
#define CONTEXT_GAME_MODE_TEAM_GAMES                1

//
// Property ids
//
// These values are passed as the dwPropertyId value to XUserSetProperty
// and as the dwPropertyId value in the XUSER_PROPERTY structure.
//

#define PROPERTY_CAPTURES                           0x10000004
#define PROPERTY_DEATHS                             0x10000005
#define PROPERTY_SCORE_LIMIT                        0x10000006
#define PROPERTY_CAPTURE_LIMIT                      0x10000007
#define PROPERTY_TIME_LIMIT                         0x10000008
#define PROPERTY_SCORE                              0x20000003

//
// Achievement ids
//
// These values are used in the dwAchievementId member of the
// XUSER_ACHIEVEMENT structure that is used with
// XUserWriteAchievements and XUserCreateAchievementEnumerator.
//

#define ACHIEVEMENT_COMPLETE_THE_DARKNESS_NORMAL    1
#define ACHIEVEMENT_COMPLETE_THE_DARKNESS_HARD      2
#define ACHIEVEMENT_KILL_ALL_WORKERS_ON_TUNNEL      3
#define ACHIEVEMENT_HUNTER_PEACEFULLY               4
#define ACHIEVEMENT_JENNY_ROMANCE                   5
#define ACHIEVEMENT_COMPLETED_NY1                   6
#define ACHIEVEMENT_COMPLETED_NY2                   7
#define ACHIEVEMENT_GOT_CD                          8
#define ACHIEVEMENT_GOT_DA                          9
#define ACHIEVEMENT_GOT_BH                          10
#define ACHIEVEMENT_GOT_AW                          11
#define ACHIEVEMENT_GOT_ALL_DARKLINGS               12
#define ACHIEVEMENT_MAXED_DARKNESS_LEVEL            13
#define ACHIEVEMENT_THROW_GERMANS_INTO_THE_VOID     14
#define ACHIEVEMENT_NO_CIVILIAN_DEATHS              15
#define ACHIEVEMENT_KILLED_ALL_CIVILIANS            16
#define ACHIEVEMENT_MANSION_SPEED_CHALLENGE         17
#define ACHIEVEMENT_GAME_SPEED_CHALLENGE            18
#define ACHIEVEMENT_FIRST_EXECUTION_KILL            19
#define ACHIEVEMENT_EXECUTION_MID_LEVEL             20
#define ACHIEVEMENT_EXECUTION_MASTER                21
#define ACHIEVEMENT_FIRST_COLLECTIBLE               22
#define ACHIEVEMENT_COLLECTIBLE_PROGRESS_2          23
#define ACHIEVEMENT_COLLECTIBLE_PROGRESS_3          24
#define ACHIEVEMENT_COLLECTIBLE_PROGRESS_4          25
#define ACHIEVEMENT_COLLECTIBLE_PROGRESS_5          26
#define ACHIEVEMENT_SURVIVOR                        27
#define ACHIEVEMENT_CREEPING_DARK_MASTER            28
#define ACHIEVEMENT_DEVOUR_MASTER                   29
#define ACHIEVEMENT_COLLECTED_ALL_BERSERKER_WEAPONS 30
#define ACHIEVEMENT_SPAWNED_ALL_SCRIPTED_DARKLINGS  31
#define ACHIEVEMENT_BLACK_HOLE_MASTER               32
#define ACHIEVEMENT_DEMON_ARM_MASTER                33
#define ACHIEVEMENT_ANCIENT_WEAPON_KILLER           34
#define ACHIEVEMENT_DARKNESS_MASTER                 35
#define ACHIEVEMENT_CREEPING_DARK_KILLER            36
#define ACHIEVEMENT_DEMON_ARM_KILLER                37
#define ACHIEVEMENT_BLACK_HOLE_KILLER               38
#define ACHIEVEMENT_FIRST_MP_VICTORY                39
#define ACHIEVEMENT_5_VICTORIES_IN_DM               40
#define ACHIEVEMENT_5_VICTORIES_IN_TDM              41
#define ACHIEVEMENT_5_VICTORIES_IN_CTF              42
#define ACHIEVEMENT_5_VICTORIES_IN_SURVIVOR         43
#define ACHIEVEMENT_5_VICTORIES_IN_LASTHUMAN        44
#define ACHIEVEMENT_KILLING_STREAK                  45

//
// Stats view ids
//
// These are used in the dwViewId member of the XUSER_STATS_SPEC structure
// passed to the XUserReadStats* and XUserCreateStatsEnumerator* functions.
//

// Skill leaderboards for ranked game modes

#define STATS_VIEW_SKILL_RANKED_FREE_FOR_ALL        0xFFFF0000
#define STATS_VIEW_SKILL_RANKED_TEAM_GAMES          0xFFFF0001

// Skill leaderboards for unranked (standard) game modes

#define STATS_VIEW_SKILL_STANDARD_FREE_FOR_ALL      0xFFFE0000
#define STATS_VIEW_SKILL_STANDARD_TEAM_GAMES        0xFFFE0001

// Title defined leaderboards

#define STATS_VIEW_DEATHMATCH_DARKLINGS             2
#define STATS_VIEW_TEAMDEATHMATCH_DARKLINGS         3
#define STATS_VIEW_CAPTURETHEFLAG_DARKLINGS         4
#define STATS_VIEW_DEATHMATCH_SHAPESHIFTER          5
#define STATS_VIEW_TEAMDEATHMATCH_SHAPESHIFTER      6
#define STATS_VIEW_CAPTURETHEFLAG_SHAPESHIFTER      7
#define STATS_VIEW_SURVIVOR                         8
#define STATS_VIEW_CAPTURETHEFLAG_DVH               9
#define STATS_VIEW_LASTHUMAN                        10

//
// Stats view column ids
//
// These ids are used to read columns of stats views.  They are specified in
// the rgwColumnIds array of the XUSER_STATS_SPEC structure.  Rank, rating
// and gamertag are not retrieved as custom columns and so are not included
// in the following definitions.  They can be retrieved from each row's
// header (e.g., pStatsResults->pViews[x].pRows[y].dwRank, etc.).
//

// Column ids for DEATHMATCH_DARKLINGS

#define STATS_COLUMN_DEATHMATCH_DARKLINGS_DEATHS    1

// Column ids for TEAMDEATHMATCH_DARKLINGS

#define STATS_COLUMN_TEAMDEATHMATCH_DARKLINGS_DEATHS 1

// Column ids for CAPTURETHEFLAG_DARKLINGS

#define STATS_COLUMN_CAPTURETHEFLAG_DARKLINGS_CAPTURES 1
#define STATS_COLUMN_CAPTURETHEFLAG_DARKLINGS_DEATHS 2

// Column ids for DEATHMATCH_SHAPESHIFTER

#define STATS_COLUMN_DEATHMATCH_SHAPESHIFTER_DEATHS 1

// Column ids for TEAMDEATHMATCH_SHAPESHIFTER

#define STATS_COLUMN_TEAMDEATHMATCH_SHAPESHIFTER_DEATHS 1

// Column ids for CAPTURETHEFLAG_SHAPESHIFTER

#define STATS_COLUMN_CAPTURETHEFLAG_SHAPESHIFTER_CAPTURES 1
#define STATS_COLUMN_CAPTURETHEFLAG_SHAPESHIFTER_DEATHS 2

// Column ids for SURVIVOR


// Column ids for CAPTURETHEFLAG_DVH

#define STATS_COLUMN_CAPTURETHEFLAG_DVH_CAPTURES    1
#define STATS_COLUMN_CAPTURETHEFLAG_DVH_DEATHS      2

// Column ids for LASTHUMAN


//
// Matchmaking queries
//
// These values are passed as the dwProcedureIndex parameter to
// XSessionSearch to indicate which matchmaking query to run.
//

#define SESSION_MATCH_QUERY_SIMPLE                  0
#define SESSION_MATCH_QUERY_EXACT                   2

//
// Gamer pictures
//
// These ids are passed as the dwPictureId parameter to XUserAwardGamerTile.
//

#define GAMER_PICTURE_JACKIE                        1


#ifdef __cplusplus
}
#endif

#endif // __THE_DARKNESS_SPA_H__


