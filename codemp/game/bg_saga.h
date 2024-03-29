/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, SerenityJediEngine2024 contributors

This file is part of the SerenityJediEngine2024 source code.

SerenityJediEngine2024 is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

#define		MAX_SIEGE_INFO_SIZE					16384

#define		SIEGETEAM_TEAM1						1 //e.g. TEAM_RED
#define		SIEGETEAM_TEAM2						2 //e.g. TEAM_BLUE

#define		SIEGE_POINTS_OBJECTIVECOMPLETED		20
#define		SIEGE_POINTS_FINALOBJECTIVECOMPLETED	30
#define		SIEGE_POINTS_TEAMWONROUND			10

#define		SIEGE_ROUND_BEGIN_TIME				5000 //delay 5 secs after players are in game.

#define		MAX_SIEGE_CLASSES					128 //up to 128 classes
#define		MAX_SIEGE_CLASSES_PER_TEAM			16

#define		MAX_SIEGE_TEAMS						16 //up to 16 diffent teams

#define		MAX_EXDATA_ENTS_TO_SEND				MAX_CLIENTS //max number of extended data for ents to send

// The basic siege player classes
typedef enum
{
	SPC_INFANTRY = 0,
	SPC_VANGUARD,
	SPC_SUPPORT,
	SPC_JEDI,
	SPC_DEMOLITIONIST,
	SPC_HEAVY_WEAPONS,
	SPC_MAX
} siegePlayerClassFlags_t;

typedef enum
{
	SPOTTED_SNIPER = 6,
	SPOTTED_TROOPS,
	REQUEST_SUPPLIES,
	REQUEST_MEDIC,
	REQUEST_ASSISTANCE,
	TACTICAL_COVERME,
	TACTICAL_HOLDPOSITION,
	TACTICAL_SPLIT,
	TACTICAL_FOLLOW,
	REPLY_YES,
	REPLY_COMING
} vchatParams_t;

typedef enum
{
	CFL_MORESABERDMG = 0,
	CFL_STRONGAGAINSTPHYSICAL,
	CFL_FASTFORCEREGEN,
	CFL_STATVIEWER,
	CFL_HEAVYMELEE,
	CFL_SINGLE_ROCKET,
	//has only 1 rocket to use with rocketlauncher
	CFL_CUSTOMSKEL,
	//class uses a custom skeleton, be sure to load on server etc
	CFL_EXTRA_AMMO
} siegeClassFlags_t;

#define SIEGE_CLASS_DESC_LEN  4096

typedef struct siegeClassDesc_s
{
	char desc[SIEGE_CLASS_DESC_LEN];
} siegeClassDesc_t;

typedef struct siegeClass_s
{
	char name[512];
	char forcedModel[256];
	char forcedSkin[256];
	char saber1[SABER_NAME_LENGTH], saber2[SABER_NAME_LENGTH];
	int saberStance;
	int weapons;
	int forcePowerLevels[NUM_FORCE_POWERS];
	int classflags;
	int maxhealth;
	int starthealth;
	int maxarmor;
	int startarmor;
	float speed;
	qboolean hasForcedSaberColor;
	int forcedSaberColor;
	qboolean hasForcedSaber2Color;
	int forcedSaber2Color;
	int invenItems;
	int powerups;
	int uiPortraitShader;
	char uiPortrait[256];
	int classShader;
	short playerClass; // SPC_INFANTRY . ..
} siegeClass_t;

typedef struct siegeTeam_s
{
	char name[512];
	siegeClass_t* classes[MAX_SIEGE_CLASSES_PER_TEAM];
	int numClasses;
	int friendlyShader;
} siegeTeam_t;

extern siegeClass_t bgSiegeClasses[MAX_SIEGE_CLASSES];
extern int bgNumSiegeClasses;

extern siegeTeam_t bgSiegeTeams[MAX_SIEGE_TEAMS];
extern int bgNumSiegeTeams;

int BG_SiegeGetValueGroup(const char* buf, char* group, char* outbuf);
int BG_SiegeGetPairedValue(const char* buf, char* key, char* outbuf);
void BG_SiegeStripTabs(char* buf);

void BG_SiegeLoadClasses(siegeClassDesc_t* descBuffer);
void BG_SiegeLoadTeams(void);

siegeTeam_t* BG_SiegeFindThemeForTeam(int team);
void BG_PrecacheSabersForSiegeTeam(int team);
siegeClass_t* BG_SiegeFindClassByName(const char* classname);
qboolean BG_SiegeCheckClassLegality(int team, char* classname);
void BG_SiegeSetTeamTheme(int team, const char* themeName);
int BG_SiegeFindClassIndexByName(const char* classname);

// for ui
int BG_GetUIPortrait(int team, short classIndex, short cntIndex);
char* BG_GetUIPortraitFile(int team, short classIndex, short cntIndex);
siegeClass_t* BG_GetClassOnBaseClass(int team, short classIndex, short cntIndex);
int BG_SiegeCountBaseClass(int team, short classIndex);

extern char siege_info[MAX_SIEGE_INFO_SIZE];
extern int siege_valid;
