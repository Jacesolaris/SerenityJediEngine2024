/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
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

#ifdef XCVAR_PROTO
#define XCVAR_DEF( name, defVal, update, flags ) extern vmCvar_t name;
#endif

#ifdef XCVAR_DECL
#define XCVAR_DEF( name, defVal, update, flags ) vmCvar_t name;
#endif

#ifdef XCVAR_LIST
#define XCVAR_DEF( name, defVal, update, flags ) { & name , #name , defVal , update , flags },
#endif

XCVAR_DEF(bg_fighterAltControl, "0", NULL, CVAR_SYSTEMINFO)
XCVAR_DEF(broadsword, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_animBlend, "1", NULL, CVAR_NONE)
XCVAR_DEF(cg_animSpeed, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_auraShell, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_autoSwitch, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_bobPitch, "0.002", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_bobRoll, "0.002", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_bobUp, "0.005", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_cameraOrbit, "0", NULL, CVAR_CHEAT)
XCVAR_DEF(cg_cameraOrbitDelay, "50", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_centerTime, "8", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_chatBeep, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_chatBox, "10000", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_chatBoxHeight, "350", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_crosshairHealth, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_crosshairSize, "20", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_crosshairX, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_crosshairY, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_currentSelectedPlayer, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_draw2D, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_draw3DIcons, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawAmmoWarning, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawCrosshair, "2", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawCrosshairNames, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_DrawCrosshairItem, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawEnemyInfo, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawFPS, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawFriend, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawGun, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawIcons, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawRadar, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawRewards, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawScores, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawSnapshot, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawStatus, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawTeamOverlay, "0", CG_TeamOverlayChange, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawTimer, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawVehLeadIndicator, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_dynamicCrosshair, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_dynamicCrosshairPrecision, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_debugAnim, "0", NULL, CVAR_CHEAT)
XCVAR_DEF(cg_debugGun, "0", NULL, CVAR_CHEAT)
XCVAR_DEF(cg_debugSaber, "0", NULL, CVAR_CHEAT)
XCVAR_DEF(cg_debugPosition, "0", NULL, CVAR_CHEAT)
XCVAR_DEF(cg_debugEvents, "0", NULL, CVAR_CHEAT)
XCVAR_DEF(cg_duelHeadAngles, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_dismember, "2", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_deferPlayers, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_errorDecay, "100", NULL, CVAR_NONE)
XCVAR_DEF(cg_fallingBob, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_footsteps, "3", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_forceModel, "0", CG_ForceModelChange, CVAR_ARCHIVE)
XCVAR_DEF(cg_saberlockfov, "60", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_fov, "80", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_fovAspectAdjust, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_fovViewmodel, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_fovViewmodelAdjust, "1", NULL, CVAR_ARCHIVE)
#if 0
XCVAR_DEF(cg_fpls, "0", NULL, CVAR_NONE)
#endif
XCVAR_DEF(cg_g2TraceLod, "2", NULL, CVAR_NONE)
XCVAR_DEF(cg_ghoul2Marks, "16", NULL, CVAR_NONE)
XCVAR_DEF(cg_gunX, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_gunY, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_gunZ, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_hudFiles, "ui/sje-hud.txt", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_jumpSounds, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_lagometer, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_marks, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_noPlayerAnims, "0", NULL, CVAR_CHEAT)
XCVAR_DEF(cg_noPredict, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_noProjectileTrail, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_predictItems, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_renderToTextureFX, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_repeaterOrb, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_runPitch, "0.002", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_runRoll, "0.005", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_saberClientVisualCompensation, "1", NULL, CVAR_NONE)
XCVAR_DEF(cg_saberContact, "1", NULL, CVAR_NONE)
XCVAR_DEF(cg_saberDynamicMarks, "1", NULL, CVAR_NONE)
XCVAR_DEF(cg_saberDynamicMarkTime, "60000", NULL, CVAR_NONE)
XCVAR_DEF(cg_saberModelTraceEffect, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_saberTrail, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_scoreboardBots, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_scorePlums, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_stereoSeparation, "0.4", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_shadows, "2", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_simpleItems, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_showMiss, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_showVehBounds, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_showVehMiss, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_smoothCamera, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_smoothClients, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_snapshotTimeout, "10", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_speedTrail, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_stats, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_teamChatBeep, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_teamChatsOnly, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_thirdPerson, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_thirdPersonAlpha, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_thirdPersonAngle, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_thirdPersonCameraDamp, "0.3", NULL, CVAR_NONE)
XCVAR_DEF(cg_thirdPersonHorzOffset, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_thirdPersonPitchOffset, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_thirdPersonRange, "90", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_thirdPersonSpecialCam, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_thirdPersonTargetDamp, "0.5", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_thirdPersonVertOffset, "16", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_timescaleFadeEnd, "1", NULL, CVAR_NONE)
XCVAR_DEF(cg_timescaleFadeSpeed, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_viewsize, "100", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_weaponBob, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cl_paused, "0", NULL, CVAR_ROM)
XCVAR_DEF(com_buildScript, "0", NULL, CVAR_NONE)
XCVAR_DEF(com_cameraMode, "0", NULL, CVAR_CHEAT)
XCVAR_DEF(com_optvehtrace, "0", NULL, CVAR_NONE)
XCVAR_DEF(debugBB, "0", NULL, CVAR_NONE)
XCVAR_DEF(forcepowers, DEFAULT_FORCEPOWERS, NULL, CVAR_USERINFO | CVAR_ARCHIVE)
XCVAR_DEF(g_synchronousClients, "0", NULL, CVAR_SYSTEMINFO)
XCVAR_DEF(model, DEFAULT_MODEL, NULL, CVAR_USERINFO | CVAR_ARCHIVE)
XCVAR_DEF(pmove_fixed, "0", NULL, CVAR_SYSTEMINFO)
XCVAR_DEF(pmove_float, "0", NULL, CVAR_SYSTEMINFO)
XCVAR_DEF(pmove_msec, "8", NULL, CVAR_SYSTEMINFO)
XCVAR_DEF(r_autoMap, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(r_autoMapX, "496", NULL, CVAR_ARCHIVE)
XCVAR_DEF(r_autoMapY, "32", NULL, CVAR_ARCHIVE)
XCVAR_DEF(r_autoMapW, "128", NULL, CVAR_ARCHIVE)
XCVAR_DEF(r_autoMapH, "128", NULL, CVAR_ARCHIVE)
XCVAR_DEF(sv_running, "0", CG_SVRunningChange, CVAR_ROM)
XCVAR_DEF(teamoverlay, "0", NULL, CVAR_ROM | CVAR_USERINFO)
XCVAR_DEF(timescale, "1", NULL, CVAR_CHEAT)
XCVAR_DEF(ui_about_gametype, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_fraglimit, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_capturelimit, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_duellimit, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_timelimit, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_maxclients, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_dmflags, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_mapname, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_hostname, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_needpass, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_about_botminplayers, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_myteam, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm1_c0_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm1_c1_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm1_c2_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm1_c3_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm1_c4_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm1_c5_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm1_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm2_c0_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm2_c1_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm2_c2_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm2_c3_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm2_c4_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm2_c5_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm2_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(ui_tm3_cnt, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(cg_SFXSabers, "3", NULL, CVAR_ARCHIVE)
XCVAR_DEF(rgb_saber1, "0,150,150", NULL, CVAR_USERINFO | CVAR_ARCHIVE)
XCVAR_DEF(rgb_saber2, "150,0,150", NULL, CVAR_USERINFO | CVAR_ARCHIVE)
XCVAR_DEF(rgb_script1, ":255,0,255:500:0,0,255:500:", NULL, CVAR_USERINFO | CVAR_ARCHIVE)
XCVAR_DEF(rgb_script2, ":0,255,255:500:0,255,0:500:", NULL, CVAR_USERINFO | CVAR_ARCHIVE)
XCVAR_DEF(team_sabercolours, "2", NULL, CVAR_ARCHIVE)
XCVAR_DEF(ui_menubrief, "0", NULL, CVAR_ROM | CVAR_INTERNAL)
XCVAR_DEF(cg_holsteredweapons, "2", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_holsterdebug, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_holsterdebug_boneindex, "0", NULL, CVAR_NONE)
XCVAR_DEF(cg_holsterdebug_posoffset, "0.0 0.0 0.0", NULL, CVAR_NONE)
XCVAR_DEF(cg_holsterdebug_angoffset, "0.0 0.0 0.0", NULL, CVAR_NONE)

XCVAR_DEF(cg_trueguns, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_trueroll, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_trueflip, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_truespin, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_truemoveroll, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_truesaberonly, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_trueeyeposition, "0.0", NULL, CVAR_NONE)
XCVAR_DEF(cg_trueinvertsaber, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_truefov, "90", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_truebobbing, "1", NULL, CVAR_ARCHIVE)

XCVAR_DEF(g_vehAutoAimLead, "0", NULL, CVAR_ARCHIVE)

XCVAR_DEF(SJE_clientplugin, CURRENT_SJE_CLIENTVERSION, NULL, CVAR_USERINFO | CVAR_ROM)
XCVAR_DEF(cg_centerfailTime, "20", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_awardsounds, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersGlowSize, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSize, "1.0", NULL, CVAR_ARCHIVE)

XCVAR_DEF(cg_SFXSabersGlowSizeBF2, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSizeBF2, "0.8", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersGlowSizeSFX, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSizeSFX, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersGlowSizeEP1, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSizeEP1, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersGlowSizeEP2, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSizeEP2, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersGlowSizeEP3, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSizeEP3, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersGlowSizeOT, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSizeOT, "0.9", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersGlowSizeTFA, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSizeTFA, "0.8", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersGlowSizeCSM, "1.0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SFXSabersCoreSizeCSM, "1.0", NULL, CVAR_ARCHIVE)

XCVAR_DEF(cg_gunMomentumDamp, "0.001", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_gunMomentumFall, "0.5", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_gunMomentumEnable, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_gunMomentumInterval, "75", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_rollSounds, "2", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_drawwidescreenmodemp, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SpinningBarrels, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_weaponcrosshairs, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_crosshairDualSize, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(r_weather, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(g_DebugSaberCombat, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(d_attackinfo, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(com_outcast, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_SaberInnonblockableAttackWarning, "0", NULL, CVAR_ARCHIVE)
XCVAR_DEF(g_AllowLedgeGrab, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(g_saberLockCinematicCamera, "1", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_oversizedview, "110", NULL, CVAR_ARCHIVE)
XCVAR_DEF(cg_undersizedview, "60", NULL, CVAR_ARCHIVE)
XCVAR_DEF(com_rend2, "0", NULL, CVAR_ARCHIVE)

#undef XCVAR_DEF
