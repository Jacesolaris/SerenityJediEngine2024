/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"
#include "cl_cgameapi.h"
#include "cl_uiapi.h"
#ifndef _WIN32
#include <cmath>
#endif
unsigned frame_msec;
int old_com_frameTime;

float cl_mPitchOverride = 0.0f;
float cl_mYawOverride = 0.0f;
float cl_mSensitivityOverride = 0.0f;
qboolean cl_bUseFighterPitch = qfalse;
qboolean cl_crazyShipControls = qfalse;

#ifdef VEH_CONTROL_SCHEME_4
#define	OVERRIDE_MOUSE_SENSITIVITY 5.0f//20.0f = 180 degree turn in one mouse swipe across keyboard
#else// VEH_CONTROL_SCHEME_4
constexpr auto OVERRIDE_MOUSE_SENSITIVITY = 10.0f; //20.0f = 180 degree turn in one mouse swipe across keyboard;
#endif// VEH_CONTROL_SCHEME_4
/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/

kbutton_t in_left, in_right, in_forward, in_back;
kbutton_t in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t in_strafe, in_speed;
kbutton_t in_up, in_down;

constexpr auto MAX_KBUTTONS = 32;

kbutton_t in_buttons[MAX_KBUTTONS];

qboolean in_mlooking;

void IN_Button11Down(void);
void IN_Button11Up(void);
void IN_Button10Down(void);
void IN_Button10Up(void);
void IN_Button6Down(void);
void IN_Button6Up(void);

static void IN_UseGivenForce(void)
{
	const char* c = Cmd_Argv(1);
	int force_num;
	int genCmdNum = 0;

	if (c)
	{
		force_num = atoi(c);
	}
	else
	{
		return;
	}

	switch (force_num)
	{
	case FP_DRAIN:
		IN_Button11Down();
		IN_Button11Up();
		break;
	case FP_PUSH:
		genCmdNum = GENCMD_FORCE_THROW;
		break;
	case FP_SPEED:
		genCmdNum = GENCMD_FORCE_SPEED;
		break;
	case FP_PULL:
		genCmdNum = GENCMD_FORCE_PULL;
		break;
	case FP_TELEPATHY:
		genCmdNum = GENCMD_FORCE_DISTRACT;
		break;
	case FP_GRIP:
		IN_Button6Down();
		IN_Button6Up();
		break;
	case FP_LIGHTNING:
		IN_Button10Down();
		IN_Button10Up();
		break;
	case FP_RAGE:
		genCmdNum = GENCMD_FORCE_RAGE;
		break;
	case FP_PROTECT:
		genCmdNum = GENCMD_FORCE_PROTECT;
		break;
	case FP_ABSORB:
		genCmdNum = GENCMD_FORCE_ABSORB;
		break;
	case FP_SEE:
		genCmdNum = GENCMD_FORCE_SEEING;
		break;
	case FP_HEAL:
		genCmdNum = GENCMD_FORCE_HEAL;
		break;
	case FP_TEAM_HEAL:
		genCmdNum = GENCMD_FORCE_HEALOTHER;
		break;
	case FP_TEAM_FORCE:
		genCmdNum = GENCMD_FORCE_FORCEPOWEROTHER;
		break;
	default:
		assert(0);
		break;
	}

	if (genCmdNum != 0)
	{
		cl.gcmdSendValue = qtrue;
		cl.gcmdValue = genCmdNum;
	}
}

static void IN_MLookDown(void)
{
	in_mlooking = qtrue;
}

void IN_CenterView(void);

static void IN_MLookUp(void)
{
	in_mlooking = qfalse;
	if (!cl_freelook->integer)
	{
		IN_CenterView();
	}
}

static void IN_GenCMD1(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_SABERSWITCH;
}

static void IN_GenCMD2(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_ENGAGE_DUEL;
}

static void IN_GenCMD3(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_HEAL;
}

static void IN_GenCMD4(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_SPEED;
}

static void IN_GenCMD5(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_PULL;
}

static void IN_GenCMD6(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_DISTRACT;
}

static void IN_GenCMD7(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_RAGE;
}

static void IN_GenCMD8(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_PROTECT;
}

static void IN_GenCMD9(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_ABSORB;
}

static void IN_GenCMD10(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_HEALOTHER;
}

static void IN_GenCMD11(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_FORCEPOWEROTHER;
}

static void IN_GenCMD12(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_SEEING;
}

static void IN_GenCMD13(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_SEEKER;
}

static void IN_GenCMD14(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_FIELD;
}

static void IN_GenCMD15(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_BACTA;
}

static void IN_GenCMD16(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_ELECTROBINOCULARS;
}

static void IN_GenCMD17(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_ZOOM;
}

static void IN_GenCMD18(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_SENTRY;
}

static void IN_GenCMD19(void)
{
	if (Cvar_VariableIntegerValue("d_saberStanceDebug"))
	{
		Com_Printf("SABERSTANCEDEBUG: Gencmd on client set successfully.\n");
	}
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_SABERATTACKCYCLE;
}

static void IN_GenCMD20(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FORCE_THROW;
}

static void IN_GenCMD21(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_JETPACK;
}

static void IN_GenCMD22(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_BACTABIG;
}

static void IN_GenCMD23(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_HEALTHDISP;
}

static void IN_GenCMD24(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_AMMODISP;
}

static void IN_GenCMD25(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_EWEB;
}

static void IN_GenCMD26(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_USE_CLOAK;
}

static void IN_GenCMD27(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_TAUNT;
}

static void IN_GenCMD28(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_BOW;
}

static void IN_GenCMD29(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_MEDITATE;
}

static void IN_GenCMD30(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_FLOURISH;
}

static void IN_GenCMD31(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_GLOAT;
}

static void IN_GenCMD32(void)
{
	cl.gcmdSendValue = qtrue;
	cl.gcmdValue = GENCMD_SURRENDER;
}

//toggle automap view mode
static bool g_clAutoMapMode = false;

static void IN_AutoMapButton(void)
{
	g_clAutoMapMode = !g_clAutoMapMode;
}

//toggle between automap, radar, nothing
extern cvar_t* r_autoMap;

static void IN_AutoMapToggle(void)
{
	Cvar_User_SetValue("cg_drawRadar", !Cvar_VariableValue("cg_drawRadar"));
}

static void IN_VoiceChatButton(void)
{
	if (!cls.uiStarted)
	{
		//ui not loaded so this command is useless
		return;
	}
	UIVM_SetActiveMenu(UIMENU_VOICECHAT);
}

static void IN_KeyDown(kbutton_t* b)
{
	int k;

	const char* c = Cmd_Argv(1);
	if (c[0])
	{
		k = atoi(c);
	}
	else
	{
		k = -1; // typed manually at the console for continuous down
	}

	if (k == b->down[0] || k == b->down[1])
	{
		return; // repeating key
	}

	if (!b->down[0])
	{
		b->down[0] = k;
	}
	else if (!b->down[1])
	{
		b->down[1] = k;
	}
	else
	{
		Com_Printf("Three keys down for a button!\n");
		return;
	}

	if (b->active)
	{
		return; // still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}

static void IN_KeyUp(kbutton_t* b)
{
	int k;

	const char* c = Cmd_Argv(1);
	if (c[0])
	{
		k = atoi(c);
	}
	else
	{
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if (b->down[0] == k)
	{
		b->down[0] = 0;
	}
	else if (b->down[1] == k)
	{
		b->down[1] = 0;
	}
	else
	{
		return; // key up without coresponding down (menu pass through)
	}
	if (b->down[0] || b->down[1])
	{
		return; // some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	const unsigned uptime = atoi(c);
	if (uptime)
	{
		b->msec += uptime - b->downtime;
	}
	else
	{
		b->msec += frame_msec / 2;
	}

	b->active = qfalse;
}

/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState(kbutton_t* key)
{
	float val;

	int msec = key->msec;
	key->msec = 0;

	if (key->active)
	{
		// still down
		if (!key->downtime)
		{
			msec = com_frameTime;
		}
		else
		{
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

#if 0
	if (msec) {
		Com_Printf("%i ", msec);
	}
#endif

	val = static_cast<float>(msec) / frame_msec;
	if (val < 0)
	{
		val = 0;
	}
	if (val > 1)
	{
		val = 1;
	}

	return val;
}

constexpr auto AUTOMAP_KEY_FORWARD = 1;
constexpr auto AUTOMAP_KEY_BACK = 2;
constexpr auto AUTOMAP_KEY_YAWLEFT = 3;
constexpr auto AUTOMAP_KEY_YAWRIGHT = 4;
constexpr auto AUTOMAP_KEY_PITCHUP = 5;
constexpr auto AUTOMAP_KEY_PITCHDOWN = 6;
constexpr auto AUTOMAP_KEY_DEFAULTVIEW = 7;

static autoMapInput_t g_clAutoMapInput;
//intercept certain keys during automap mode
static void CL_AutoMapKey(const int autoMapKey, const qboolean up)
{
	const auto data = reinterpret_cast<autoMapInput_t*>(cl.mSharedMemory);

	switch (autoMapKey)
	{
	case AUTOMAP_KEY_FORWARD:
		if (up)
		{
			g_clAutoMapInput.up = 0.0f;
		}
		else
		{
			g_clAutoMapInput.up = 16.0f;
		}
		break;
	case AUTOMAP_KEY_BACK:
		if (up)
		{
			g_clAutoMapInput.down = 0.0f;
		}
		else
		{
			g_clAutoMapInput.down = 16.0f;
		}
		break;
	case AUTOMAP_KEY_YAWLEFT:
		if (up)
		{
			g_clAutoMapInput.yaw = 0.0f;
		}
		else
		{
			g_clAutoMapInput.yaw = -4.0f;
		}
		break;
	case AUTOMAP_KEY_YAWRIGHT:
		if (up)
		{
			g_clAutoMapInput.yaw = 0.0f;
		}
		else
		{
			g_clAutoMapInput.yaw = 4.0f;
		}
		break;
	case AUTOMAP_KEY_PITCHUP:
		if (up)
		{
			g_clAutoMapInput.pitch = 0.0f;
		}
		else
		{
			g_clAutoMapInput.pitch = -4.0f;
		}
		break;
	case AUTOMAP_KEY_PITCHDOWN:
		if (up)
		{
			g_clAutoMapInput.pitch = 0.0f;
		}
		else
		{
			g_clAutoMapInput.pitch = 4.0f;
		}
		break;
	case AUTOMAP_KEY_DEFAULTVIEW:
		memset(&g_clAutoMapInput, 0, sizeof(autoMapInput_t));
		g_clAutoMapInput.goToDefaults = qtrue;
		break;
	default:
		break;
	}

	memcpy(data, &g_clAutoMapInput, sizeof(autoMapInput_t));

	if (cls.cgameStarted)
	{
		CGVM_AutomapInput();
	}

	g_clAutoMapInput.goToDefaults = qfalse;
}

static void IN_UpDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_PITCHUP, qfalse);
	}
	else
	{
		IN_KeyDown(&in_up);
	}
}

static void IN_UpUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_PITCHUP, qtrue);
	}
	else
	{
		IN_KeyUp(&in_up);
	}
}

static void IN_DownDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_PITCHDOWN, qfalse);
	}
	else
	{
		IN_KeyDown(&in_down);
	}
}

static void IN_DownUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_PITCHDOWN, qtrue);
	}
	else
	{
		IN_KeyUp(&in_down);
	}
}

static void IN_LeftDown(void) { IN_KeyDown(&in_left); }
static void IN_LeftUp(void) { IN_KeyUp(&in_left); }
static void IN_RightDown(void) { IN_KeyDown(&in_right); }
static void IN_RightUp(void) { IN_KeyUp(&in_right); }

static void IN_ForwardDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_FORWARD, qfalse);
	}
	else
	{
		IN_KeyDown(&in_forward);
	}
}

static void IN_ForwardUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_FORWARD, qtrue);
	}
	else
	{
		IN_KeyUp(&in_forward);
	}
}

static void IN_BackDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_BACK, qfalse);
	}
	else
	{
		IN_KeyDown(&in_back);
	}
}

static void IN_BackUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_BACK, qtrue);
	}
	else
	{
		IN_KeyUp(&in_back);
	}
}

static void IN_LookupDown(void) { IN_KeyDown(&in_lookup); }
static void IN_LookupUp(void) { IN_KeyUp(&in_lookup); }
static void IN_LookdownDown(void) { IN_KeyDown(&in_lookdown); }
static void IN_LookdownUp(void) { IN_KeyUp(&in_lookdown); }

static void IN_MoveleftDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWLEFT, qfalse);
	}
	else
	{
		IN_KeyDown(&in_moveleft);
	}
}

static void IN_MoveleftUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWLEFT, qtrue);
	}
	else
	{
		IN_KeyUp(&in_moveleft);
	}
}

static void IN_MoverightDown(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWRIGHT, qfalse);
	}
	else
	{
		IN_KeyDown(&in_moveright);
	}
}

static void IN_MoverightUp(void)
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_YAWRIGHT, qtrue);
	}
	else
	{
		IN_KeyUp(&in_moveright);
	}
}

static void IN_SpeedDown(void) { IN_KeyDown(&in_speed); }
static void IN_SpeedUp(void) { IN_KeyUp(&in_speed); }
static void IN_StrafeDown(void) { IN_KeyDown(&in_strafe); }
static void IN_StrafeUp(void) { IN_KeyUp(&in_strafe); }

static void IN_Button0Down(void) { IN_KeyDown(&in_buttons[0]); }
static void IN_Button0Up(void) { IN_KeyUp(&in_buttons[0]); }
static void IN_Button1Down(void) { IN_KeyDown(&in_buttons[1]); }
static void IN_Button1Up(void) { IN_KeyUp(&in_buttons[1]); }
static void IN_Button2Down(void) { IN_KeyDown(&in_buttons[2]); }
static void IN_Button2Up(void) { IN_KeyUp(&in_buttons[2]); }
static void IN_Button3Down(void) { IN_KeyDown(&in_buttons[3]); }
static void IN_Button3Up(void) { IN_KeyUp(&in_buttons[3]); }
static void IN_Button4Down(void) { IN_KeyDown(&in_buttons[4]); }
static void IN_Button4Up(void) { IN_KeyUp(&in_buttons[4]); }

static void IN_Button5Down(void) //use key
{
	if (g_clAutoMapMode)
	{
		CL_AutoMapKey(AUTOMAP_KEY_DEFAULTVIEW, qfalse);
	}
	else
	{
		IN_KeyDown(&in_buttons[5]);
	}
}

static void IN_Button5Up(void) { IN_KeyUp(&in_buttons[5]); }
void IN_Button6Down(void) { IN_KeyDown(&in_buttons[6]); }
void IN_Button6Up(void) { IN_KeyUp(&in_buttons[6]); }
static void IN_Button7Down(void) { IN_KeyDown(&in_buttons[7]); }
static void IN_Button7Up(void) { IN_KeyUp(&in_buttons[7]); }
static void IN_Button8Down(void) { IN_KeyDown(&in_buttons[8]); }
static void IN_Button8Up(void) { IN_KeyUp(&in_buttons[8]); }
static void IN_Button9Down(void) { IN_KeyDown(&in_buttons[9]); }
static void IN_Button9Up(void) { IN_KeyUp(&in_buttons[9]); }
void IN_Button10Down(void) { IN_KeyDown(&in_buttons[10]); }
void IN_Button10Up(void) { IN_KeyUp(&in_buttons[10]); }
void IN_Button11Down(void) { IN_KeyDown(&in_buttons[11]); }
void IN_Button11Up(void) { IN_KeyUp(&in_buttons[11]); }
static void IN_Button12Down(void) { IN_KeyDown(&in_buttons[12]); }
static void IN_Button12Up(void) { IN_KeyUp(&in_buttons[12]); }
static void IN_Button13Down(void) { IN_KeyDown(&in_buttons[13]); }
static void IN_Button13Up(void) { IN_KeyUp(&in_buttons[13]); }
static void IN_Button14Down(void) { IN_KeyDown(&in_buttons[14]); }
static void IN_Button14Up(void) { IN_KeyUp(&in_buttons[14]); }
static void IN_Button15Down(void) { IN_KeyDown(&in_buttons[15]); }
static void IN_Button15Up(void) { IN_KeyUp(&in_buttons[15]); }
static void IN_Button16Down(void) { IN_KeyDown(&in_buttons[16]); }
static void IN_Button16Up(void) { IN_KeyUp(&in_buttons[16]); }
static void IN_Button17Down(void) { IN_KeyDown(&in_buttons[17]); }
static void IN_Button17Up(void) { IN_KeyUp(&in_buttons[17]); }
static void IN_Button18Down(void) { IN_KeyDown(&in_buttons[18]); }
static void IN_Button18Up(void) { IN_KeyUp(&in_buttons[18]); }
static void IN_Button19Down(void) { IN_KeyDown(&in_buttons[19]); }
static void IN_Button19Up(void) { IN_KeyUp(&in_buttons[19]); }
static void IN_Button20Down(void) { IN_KeyDown(&in_buttons[20]); }
static void IN_Button20Up(void) { IN_KeyUp(&in_buttons[20]); }

void IN_CenterView(void)
{
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.snap.ps.delta_angles[PITCH]);
}

//==========================================================================

cvar_t* cl_upspeed;
cvar_t* cl_forwardspeed;
cvar_t* cl_sidespeed;

cvar_t* cl_yawspeed;
cvar_t* cl_pitchspeed;

cvar_t* cl_run;

cvar_t* cl_anglespeedkey;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
static void CL_AdjustAngles(void)
{
	float speed;

	if (in_speed.active)
	{
		speed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	}
	else
	{
		speed = 0.001 * cls.frametime;
	}

	if (!in_strafe.active)
	{
		if (cl_mYawOverride)
		{
			if (cl_mSensitivityOverride)
			{
				cl.viewangles[YAW] -= cl_mYawOverride * cl_mSensitivityOverride * speed * cl_yawspeed->value *
					CL_KeyState(&in_right);
				cl.viewangles[YAW] += cl_mYawOverride * cl_mSensitivityOverride * speed * cl_yawspeed->value *
					CL_KeyState(&in_left);
			}
			else
			{
				cl.viewangles[YAW] -= cl_mYawOverride * OVERRIDE_MOUSE_SENSITIVITY * speed * cl_yawspeed->value *
					CL_KeyState(&in_right);
				cl.viewangles[YAW] += cl_mYawOverride * OVERRIDE_MOUSE_SENSITIVITY * speed * cl_yawspeed->value *
					CL_KeyState(&in_left);
			}
		}
		else
		{
			cl.viewangles[YAW] -= speed * cl_yawspeed->value * CL_KeyState(&in_right);
			cl.viewangles[YAW] += speed * cl_yawspeed->value * CL_KeyState(&in_left);
		}
	}

	if (cl_mPitchOverride)
	{
		if (cl_mSensitivityOverride)
		{
			cl.viewangles[PITCH] -= cl_mPitchOverride * cl_mSensitivityOverride * speed * cl_pitchspeed->value *
				CL_KeyState(&in_lookup);
			cl.viewangles[PITCH] += cl_mPitchOverride * cl_mSensitivityOverride * speed * cl_pitchspeed->value *
				CL_KeyState(&in_lookdown);
		}
		else
		{
			cl.viewangles[PITCH] -= cl_mPitchOverride * OVERRIDE_MOUSE_SENSITIVITY * speed * cl_pitchspeed->value *
				CL_KeyState(&in_lookup);
			cl.viewangles[PITCH] += cl_mPitchOverride * OVERRIDE_MOUSE_SENSITIVITY * speed * cl_pitchspeed->value *
				CL_KeyState(&in_lookdown);
		}
	}
	else
	{
		cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * CL_KeyState(&in_lookup);
		cl.viewangles[PITCH] += speed * cl_pitchspeed->value * CL_KeyState(&in_lookdown);
	}
}

/*
================
CL_KeyMove

Sets the usercmd_t based on key states
================
*/
static void CL_KeyMove(usercmd_t* cmd)
{
	int movespeed;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if (in_speed.active ^ cl_run->integer)
	{
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	}
	else
	{
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 64;
	}

	int forward = 0;
	int side = 0;
	int up = 0;
	if (in_strafe.active)
	{
		side += movespeed * CL_KeyState(&in_right);
		side -= movespeed * CL_KeyState(&in_left);
	}

	side += movespeed * CL_KeyState(&in_moveright);
	side -= movespeed * CL_KeyState(&in_moveleft);

	up += movespeed * CL_KeyState(&in_up);
	up -= movespeed * CL_KeyState(&in_down);

	forward += movespeed * CL_KeyState(&in_forward);
	forward -= movespeed * CL_KeyState(&in_back);

	cmd->forwardmove = ClampChar(forward);
	cmd->rightmove = ClampChar(side);
	cmd->upmove = ClampChar(up);
}

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent(const int dx, const int dy, int time)
{
	if (g_clAutoMapMode && cls.cgameStarted)
	{
		//automap input
		const auto data = reinterpret_cast<autoMapInput_t*>(cl.mSharedMemory);

		g_clAutoMapInput.yaw = dx;
		g_clAutoMapInput.pitch = dy;
		memcpy(data, &g_clAutoMapInput, sizeof(autoMapInput_t));
		CGVM_AutomapInput();

		g_clAutoMapInput.yaw = 0.0f;
		g_clAutoMapInput.pitch = 0.0f;
	}
	else if (Key_GetCatcher() & KEYCATCH_UI)
	{
		UIVM_MouseEvent(dx, dy);
	}
	else if (Key_GetCatcher() & KEYCATCH_CGAME)
	{
		CGVM_MouseEvent(dx, dy);
	}
	else
	{
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
=================
CL_JoystickEvent

Joystick values stay set until changed
=================
*/
void CL_JoystickEvent(const int axis, const int value, int time)
{
	if (axis < 0 || axis >= MAX_JOYSTICK_AXIS)
	{
		Com_Error(ERR_DROP, "CL_JoystickEvent: bad axis %i", axis);
	}

	if (Key_GetCatcher() & (KEYCATCH_UI))
	{
		return;
	}
	cl.joystickAxis[axis] = value;
}

/*
=================
CL_JoystickMove
=================
*/
extern cvar_t* in_joystick;

extern cvar_t* j_pitch;
extern cvar_t* j_yaw;
extern cvar_t* j_forward;
extern cvar_t* j_side;
extern cvar_t* j_up;
extern cvar_t* j_pitch_axis;
extern cvar_t* j_yaw_axis;
extern cvar_t* j_forward_axis;
extern cvar_t* j_side_axis;
extern cvar_t* j_up_axis;
extern cvar_t* j_sensitivity;

static void CL_JoystickMove(usercmd_t* cmd)
{
	float	anglespeed;

	if (!in_joystick->integer)
	{
		return;
	}

	float yaw = j_yaw->value * cl.joystickAxis[j_yaw_axis->integer];
	float right = j_side->value * cl.joystickAxis[j_side_axis->integer];
	float forward = j_forward->value * cl.joystickAxis[j_forward_axis->integer];
	float pitch = j_pitch->value * cl.joystickAxis[j_pitch_axis->integer];
	float up = j_up->value * cl.joystickAxis[j_up_axis->integer];

	if (!(in_speed.active ^ cl_run->integer))
	{
		cmd->buttons |= BUTTON_WALKING;
	}
	else if (pitch <= 180 && pitch >= -180 && pitch != 0)
	{
		cmd->buttons |= BUTTON_WALKING;
	}

	if (in_speed.active)
	{
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	}
	else
	{
		anglespeed = 0.001 * cls.frametime;
	}

	if (!in_strafe.active)
	{
		if (cl_mYawOverride)
		{
			if (cl_mSensitivityOverride)
			{
				cl.viewangles[YAW] += cl_mYawOverride * cl_mSensitivityOverride * yaw / 2.0f;
			}
			else
			{
				cl.viewangles[YAW] += cl_mYawOverride * OVERRIDE_MOUSE_SENSITIVITY * yaw / 2.0f;
			}
		}
		else
		{
			cl.viewangles[YAW] += anglespeed * (yaw * j_sensitivity->value);
		}
		cmd->rightmove = ClampCharMove(cmd->rightmove + (int)right);
	}
	else
	{
		cl.viewangles[YAW] += anglespeed * right;
		cmd->rightmove = ClampCharMove(cmd->rightmove + (int)yaw);
	}

	if (in_mlooking || cl_freelook->integer)
	{
		if (cl_mPitchOverride)
		{
			if (cl_mSensitivityOverride)
			{
				cl.viewangles[PITCH] += cl_mPitchOverride * cl_mSensitivityOverride * forward / 2.0f;
			}
			else
			{
				cl.viewangles[PITCH] += cl_mPitchOverride * OVERRIDE_MOUSE_SENSITIVITY * forward / 2.0f;
			}
		}
		else
		{
			cl.viewangles[PITCH] -= anglespeed * ((forward / 11.38) * j_sensitivity->value);
		}
		cmd->forwardmove = ClampCharMove(cmd->forwardmove + (int)pitch);
	}
	else
	{
		cl.viewangles[PITCH] += anglespeed * pitch;
		cmd->forwardmove = ClampCharMove(cmd->forwardmove + (int)forward);
	}

	cmd->upmove = ClampCharMove(cmd->upmove + (int)up);

#if 0
	if (!in_strafe.active)
	{
		if (cl_mYawOverride)
		{
			if (cl_mSensitivityOverride)
			{
				cl.viewangles[YAW] += cl_mYawOverride * cl_mSensitivityOverride * cl.joystickAxis[AXIS_SIDE] / 2.0f;
			}
			else
			{
				cl.viewangles[YAW] += cl_mYawOverride * OVERRIDE_MOUSE_SENSITIVITY * cl.joystickAxis[AXIS_SIDE] / 2.0f;
			}
		}
		else
		{
			cl.viewangles[YAW] += anglespeed * (cl_yawspeed->value / 100.0f) * cl.joystickAxis[AXIS_SIDE];
		}
	}
	else
	{
		cmd->rightmove = ClampChar(cmd->rightmove + cl.joystickAxis[AXIS_SIDE]);
	}

	if (in_mlooking || cl_freelook->integer)
	{
		if (cl_mPitchOverride)
		{
			if (cl_mSensitivityOverride)
			{
				cl.viewangles[PITCH] += cl_mPitchOverride * cl_mSensitivityOverride * cl.joystickAxis[AXIS_FORWARD] / 2.0f;
			}
			else
			{
				cl.viewangles[PITCH] += cl_mPitchOverride * OVERRIDE_MOUSE_SENSITIVITY * cl.joystickAxis[AXIS_FORWARD] / 2.0f;
			}
		}
		else
		{
			cl.viewangles[PITCH] += anglespeed * (cl_pitchspeed->value / 100.0f) * cl.joystickAxis[AXIS_FORWARD];
		}
	}
	else
	{
		cmd->forwardmove = ClampChar(cmd->forwardmove + cl.joystickAxis[AXIS_FORWARD]);
	}

	cmd->upmove = ClampChar(cmd->upmove + cl.joystickAxis[AXIS_UP]);
#endif
}
/*
=================
CL_MouseMove
=================
*/
static void CL_MouseMove(usercmd_t* cmd)
{
	float mx, my;
	const float speed = static_cast<float>(frame_msec);

	// allow mouse smoothing
	if (m_filter->integer)
	{
		mx = (cl.mouseDx[0] + cl.mouseDx[1]) * 0.5;
		my = (cl.mouseDy[0] + cl.mouseDy[1]) * 0.5;
	}
	else
	{
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}

	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	if (mx == 0.0f && my == 0.0f)
		return;

	if (cl_mouseAccel->value != 0.0f)
	{
		if (cl_mouseAccelStyle->integer == 0)
		{
			float accelSensitivity;

			const float rate = SQRTFAST(mx * mx + my * my) / speed;

			if (cl_mYawOverride || cl_mPitchOverride)
			{
				//FIXME: different people have different speed mouses,
				if (cl_mSensitivityOverride)
				{
					//this will fuck things up for them, need to clamp
					//max input?
					accelSensitivity = cl_mSensitivityOverride;
				}
				else
				{
					accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
				}
			}
			else
			{
				accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
			}
			mx *= accelSensitivity;
			my *= accelSensitivity;

			if (cl_showMouseRate->integer)
				Com_Printf("rate: %f, accelSensitivity: %f\n", rate, accelSensitivity);
		}
		else
		{
			float rate[2]{};
			float power[2]{};

			// sensitivity remains pretty much unchanged at low speeds
			// cl_mouseAccel is a power value to how the acceleration is shaped
			// cl_mouseAccelOffset is the rate for which the acceleration will have doubled the non accelerated amplification
			// NOTE: decouple the config cvars for independent acceleration setup along X and Y?

			rate[0] = fabs(mx) / speed;
			rate[1] = fabs(my) / speed;
			power[0] = powf(rate[0] / cl_mouseAccelOffset->value, cl_mouseAccel->value);
			power[1] = powf(rate[1] / cl_mouseAccelOffset->value, cl_mouseAccel->value);

			if (cl_mYawOverride || cl_mPitchOverride)
			{
				//FIXME: different people have different speed mouses,
				if (cl_mSensitivityOverride)
				{
					//this will fuck things up for them, need to clamp
					//max input?
					mx = cl_mSensitivityOverride * (mx + (mx < 0 ? -power[0] : power[0]) * cl_mouseAccelOffset->
						value);
					my = cl_mSensitivityOverride * (my + (my < 0 ? -power[1] : power[1]) * cl_mouseAccelOffset->
						value);
				}
				else
				{
					mx = cl_sensitivity->value * (mx + (mx < 0 ? -power[0] : power[0]) * cl_mouseAccelOffset->value);
					my = cl_sensitivity->value * (my + (my < 0 ? -power[1] : power[1]) * cl_mouseAccelOffset->value);
				}
			}
			else
			{
				mx = cl_sensitivity->value * (mx + (mx < 0 ? -power[0] : power[0]) * cl_mouseAccelOffset->value);
				my = cl_sensitivity->value * (my + (my < 0 ? -power[1] : power[1]) * cl_mouseAccelOffset->value);
			}

			if (cl_showMouseRate->integer)
				Com_Printf("ratex: %f, ratey: %f, powx: %f, powy: %f\n", rate[0], rate[1], power[0], power[1]);
		}
	}
	else
	{
		if (cl_mYawOverride || cl_mPitchOverride)
		{
			//FIXME: different people have different speed mouses,
			if (cl_mSensitivityOverride)
			{
				//this will fuck things up for them, need to clamp
				//max input?
				mx *= cl_mSensitivityOverride;
				my *= cl_mSensitivityOverride;
			}
			else
			{
				mx *= cl_sensitivity->value;
				my *= cl_sensitivity->value;
			}
		}
		else
		{
			mx *= cl_sensitivity->value;
			my *= cl_sensitivity->value;
		}
	}

	// ingame FOV
	mx *= cl.cgameSensitivity;
	my *= cl.cgameSensitivity;

	// add mouse X/Y movement to cmd
	if (in_strafe.active)
		cmd->rightmove = ClampChar(cmd->rightmove + m_side->value * mx);
	else
	{
		if (cl_mYawOverride)
			cl.viewangles[YAW] -= cl_mYawOverride * mx;
		else
			cl.viewangles[YAW] -= m_yaw->value * mx;
	}

	if ((in_mlooking || cl_freelook->integer) && !in_strafe.active)
	{
		// VVFIXME - This is supposed to be a CVAR
		constexpr float cl_pitchSensitivity = 1.0f;
		const float pitch = cl_bUseFighterPitch ? m_pitchVeh->value : m_pitch->value;
		if (cl_mPitchOverride)
		{
			if (pitch > 0)
				cl.viewangles[PITCH] += cl_mPitchOverride * my * cl_pitchSensitivity;
			else
				cl.viewangles[PITCH] -= cl_mPitchOverride * my * cl_pitchSensitivity;
		}
		else
			cl.viewangles[PITCH] += pitch * my * cl_pitchSensitivity;
	}
	else
		cmd->forwardmove = ClampChar(cmd->forwardmove - m_forward->value * my);
}

static qboolean CL_NoUseableForce(void)
{
	if (!cls.cgameStarted)
	{
		//ahh, no cgame loaded
		return qfalse;
	}

	return CGVM_NoUseableForce();
}

/*
==============
CL_CmdButtons
==============
*/
static void CL_CmdButtons(usercmd_t* cmd)
{
	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//
	for (int i = 0; i < MAX_KBUTTONS; i++)
	{
		if (in_buttons[i].active || in_buttons[i].wasPressed)
		{
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = qfalse;
	}

	if (cmd->buttons & BUTTON_FORCEPOWER)
	{
		//check for transferring a use force to a use inventory...
		if (cmd->buttons & BUTTON_USE || CL_NoUseableForce())
		{
			//it's pushed, remap it!
			cmd->buttons &= ~BUTTON_FORCEPOWER;
			cmd->buttons |= BUTTON_USE_HOLDABLE;
		}
	}

	if (Key_GetCatcher())
	{
		cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	if (kg.anykeydown && Key_GetCatcher() == 0)
	{
		cmd->buttons |= BUTTON_ANY;
	}
}

/*
==============
CL_FinishMove
==============
*/
vec3_t cl_sendAngles = { 0 };
vec3_t cl_lastViewAngles = { 0 };

static void CL_FinishMove(usercmd_t* cmd)
{
	int i;

	// copy the state that the cgame is currently sending
	cmd->weapon = cl.cgameUserCmdValue;
	cmd->forcesel = cl.cgameForceSelection;
	cmd->invensel = cl.cgameInvenSelection;

	if (cl.gcmdSendValue)
	{
		cmd->generic_cmd = cl.gcmdValue;
		//cl.gcmdSendValue = qfalse;
		cl.gcmdSentValue = qtrue;
	}
	else
	{
		cmd->generic_cmd = 0;
	}

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	if (cl.cgameViewAngleForceTime > cl.serverTime)
	{
		cl.cgameViewAngleForce[YAW] -= SHORT2ANGLE(cl.snap.ps.delta_angles[YAW]);

		cl.viewangles[YAW] = cl.cgameViewAngleForce[YAW];
		cl.cgameViewAngleForceTime = 0;
	}

	if (cl_crazyShipControls)
	{
		const float yawDelta = AngleSubtract(cl.viewangles[YAW], cl_lastViewAngles[YAW]);
		//yawDelta *= (4.0f*p_veh->m_fTimeModifier);
		cl_sendAngles[ROLL] -= yawDelta;

		float nRoll = fabs(cl_sendAngles[ROLL]);

		const float pitchDelta = AngleSubtract(cl.viewangles[PITCH], cl_lastViewAngles[PITCH]);
		//pitchDelta *= (2.0f*p_veh->m_fTimeModifier);
		float pitchSubtract = pitchDelta * (nRoll / 90.0f);
		cl_sendAngles[PITCH] += pitchDelta - pitchSubtract;

		//yaw-roll calc should be different
		if (nRoll > 90.0f)
		{
			nRoll -= 180.0f;
		}
		if (nRoll < 0.0f)
		{
			nRoll = -nRoll;
		}
		pitchSubtract = pitchDelta * (nRoll / 90.0f);
		if (cl_sendAngles[ROLL] > 0.0f)
		{
			cl_sendAngles[YAW] += pitchSubtract;
		}
		else
		{
			cl_sendAngles[YAW] -= pitchSubtract;
		}

		cl_sendAngles[PITCH] = AngleNormalize180(cl_sendAngles[PITCH]);
		cl_sendAngles[YAW] = AngleNormalize360(cl_sendAngles[YAW]);
		cl_sendAngles[ROLL] = AngleNormalize180(cl_sendAngles[ROLL]);

		for (i = 0; i < 3; i++)
		{
			cmd->angles[i] = ANGLE2SHORT(cl_sendAngles[i]);
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
		}
		//in case we switch to the cl_crazyShipControls
		VectorCopy(cl.viewangles, cl_sendAngles);
	}
	//always needed in for the cl_crazyShipControls
	VectorCopy(cl.viewangles, cl_lastViewAngles);
}

/*
=================
CL_CreateCmd
=================
*/
static usercmd_t CL_CreateCmd(void)
{
	usercmd_t cmd;
	vec3_t oldAngles;

	VectorCopy(cl.viewangles, oldAngles);

	// keyboard angle adjustment
	CL_AdjustAngles();

	Com_Memset(&cmd, 0, sizeof cmd);

	CL_CmdButtons(&cmd);

	// get basic movement from keyboard
	CL_KeyMove(&cmd);

	// get basic movement from mouse
	CL_MouseMove(&cmd);

	// get basic movement from joystick
	CL_JoystickMove(&cmd);

	// check to make sure the angles haven't wrapped
	if (cl.viewangles[PITCH] - oldAngles[PITCH] > 90)
	{
		cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
	}
	else if (oldAngles[PITCH] - cl.viewangles[PITCH] > 90)
	{
		cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
	}

	// store out the final values
	CL_FinishMove(&cmd);

	// draw debug graphs of turning for mouse testing
	if (cl_debugMove->integer)
	{
		if (cl_debugMove->integer == 1)
		{
			SCR_DebugGraph(abs(cl.viewangles[YAW] - oldAngles[YAW]), 0);
		}
		if (cl_debugMove->integer == 2)
		{
			SCR_DebugGraph(abs(cl.viewangles[PITCH] - oldAngles[PITCH]), 0);
		}
	}

	return cmd;
}

/*
=================
CL_CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
static void CL_CreateNewCommands(void)
{
	// no need to create usercmds until we have a gamestate
	if (cls.state < CA_PRIMED)
		return;

	frame_msec = com_frameTime - old_com_frameTime;

	// if running over 1000fps, act as if each frame is 1ms
	// prevents divisions by zero
	if (frame_msec < 1)
		frame_msec = 1;

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if (frame_msec > 200)
		frame_msec = 200;

	old_com_frameTime = com_frameTime;

	// generate a command for this frame
	cl.cmdNumber++;
	const int cmdNum = cl.cmdNumber & CMD_MASK;
	cl.cmds[cmdNum] = CL_CreateCmd();
}

/*
=================
CL_ReadyToSendPacket

Returns qfalse if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
static qboolean CL_ReadyToSendPacket(void)
{
	// don't send anything if playing back a demo
	if (clc.demoplaying || cls.state == CA_CINEMATIC)
	{
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if (*clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 50)
	{
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if (cls.state != CA_ACTIVE &&
		cls.state != CA_PRIMED &&
		!*clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 1000)
	{
		return qfalse;
	}

	// send every frame for loopbacks
	if (clc.netchan.remoteAddress.type == NA_LOOPBACK)
	{
		return qtrue;
	}

	// send every frame for LAN
	if (cl_lanForcePackets->integer && Sys_IsLANAddress(clc.netchan.remoteAddress))
	{
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if (cl_maxpackets->integer < 20)
	{
		Cvar_Set("cl_maxpackets", "20");
	}
	else if (cl_maxpackets->integer > 1000)
	{
		Cvar_Set("cl_maxpackets", "1000");
	}
	const int oldPacketNum = clc.netchan.outgoingSequence - 1 & PACKET_MASK;
	const int delta = cls.realtime - cl.outPackets[oldPacketNum].p_realtime;
	if (delta < 1000 / cl_maxpackets->integer)
	{
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	command count
<count * usercmds>

===================
*/
void CL_WritePacket(void)
{
	msg_t buf;
	byte data[MAX_MSGLEN];
	int i;
	usercmd_t nullcmd;

	// don't send anything if playing back a demo
	if (clc.demoplaying || cls.state == CA_CINEMATIC)
	{
		return;
	}

	Com_Memset(&nullcmd, 0, sizeof nullcmd);
	usercmd_t* oldcmd = &nullcmd;

	MSG_Init(&buf, data, sizeof data);

	MSG_Bitstream(&buf);
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong(&buf, cl.serverId);

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong(&buf, clc.serverMessageSequence);

	// write the last reliable message we received
	MSG_WriteLong(&buf, clc.serverCommandSequence);

	// write any unacknowledged clientCommands
	for (i = clc.reliableAcknowledge + 1; i <= clc.reliableSequence; i++)
	{
		MSG_WriteByte(&buf, clc_clientCommand);
		MSG_WriteLong(&buf, i);
		MSG_WriteString(&buf, clc.reliableCommands[i & MAX_RELIABLE_COMMANDS - 1]);
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if (cl_packetdup->integer < 0)
	{
		Cvar_Set("cl_packetdup", "0");
	}
	else if (cl_packetdup->integer > 5)
	{
		Cvar_Set("cl_packetdup", "5");
	}
	const int oldPacketNum = clc.netchan.outgoingSequence - 1 - cl_packetdup->integer & PACKET_MASK;
	int count = cl.cmdNumber - cl.outPackets[oldPacketNum].p_CmdNumber;
	if (count > MAX_PACKET_USERCMDS)
	{
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}
	if (count >= 1)
	{
		if (cl_showSend->integer)
		{
			Com_Printf("(%i)", count);
		}

		// begin a client move command
		if (cl_nodelta->integer || !cl.snap.valid
			|| clc.demowaiting
			|| clc.serverMessageSequence != cl.snap.messageNum)
		{
			MSG_WriteByte(&buf, clc_moveNoDelta);
		}
		else
		{
			MSG_WriteByte(&buf, clc_move);
		}

		// write the command count
		MSG_WriteByte(&buf, count);

		// use the checksum feed in the key
		int key = clc.checksumFeed;
		// also use the message acknowledge
		key ^= clc.serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey(clc.serverCommands[clc.serverCommandSequence & MAX_RELIABLE_COMMANDS - 1], 32);

		// write all the commands, including the predicted command
		for (i = 0; i < count; i++)
		{
			const int j = cl.cmdNumber - count + i + 1 & CMD_MASK;
			usercmd_t* cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmdKey(&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}

		if (cl.gcmdSentValue)
		{
			//hmm, just clear here, I guess.. hoping it will resolve issues with gencmd values sometimes not going through.
			cl.gcmdSendValue = qfalse;
			cl.gcmdSentValue = qfalse;
			cl.gcmdValue = 0;
		}
	}

	//
	// deliver the message
	//
	const int packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[packetNum].p_realtime = cls.realtime;
	cl.outPackets[packetNum].p_serverTime = oldcmd->serverTime;
	cl.outPackets[packetNum].p_CmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;

	if (cl_showSend->integer)
	{
		Com_Printf("%i ", buf.cursize);
	}

	CL_Netchan_Transmit(&clc.netchan, &buf);

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	while (clc.netchan.unsentFragments)
	{
		CL_Netchan_TransmitNextFragment(&clc.netchan);
	}
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd(void)
{
	// don't send any message if not connected
	if (cls.state < CA_CONNECTED)
	{
		return;
	}

	// don't send commands if paused
	if (com_sv_running->integer && sv_paused->integer && cl_paused->integer)
	{
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if (!CL_ReadyToSendPacket())
	{
		if (cl_showSend->integer)
		{
			Com_Printf(". ");
		}
		return;
	}

	CL_WritePacket();
}

static constexpr cmdList_t inputCmds[] =
{
	{"centerview", "Centers view on screen", IN_CenterView, nullptr},
	{"+moveup", "Jump", IN_UpDown, nullptr},
	{"-moveup", nullptr, IN_UpUp, nullptr},
	{"+movedown", "Crouch", IN_DownDown, nullptr},
	{"-movedown", nullptr, IN_DownUp, nullptr},
	{"+left", "Rotate camera left", IN_LeftDown, nullptr},
	{"-left", nullptr, IN_LeftUp, nullptr},
	{"+right", "Rotate camera right", IN_RightDown, nullptr},
	{"-right", nullptr, IN_RightUp, nullptr},
	{"+forward", "Move forward", IN_ForwardDown, nullptr},
	{"-forward", nullptr, IN_ForwardUp, nullptr},
	{"+back", "Move backward", IN_BackDown, nullptr},
	{"-back", nullptr, IN_BackUp, nullptr},
	{"+lookup", "Tilt camera up", IN_LookupDown, nullptr},
	{"-lookup", nullptr, IN_LookupUp, nullptr},
	{"+lookdown", "Tilt camera down", IN_LookdownDown, nullptr},
	{"-lookdown", nullptr, IN_LookdownUp, nullptr},
	{"+strafe", "Hold to strafe", IN_StrafeDown, nullptr},
	{"-strafe", nullptr, IN_StrafeUp, nullptr},
	{"+moveleft", "Strafe left", IN_MoveleftDown, nullptr},
	{"-moveleft", nullptr, IN_MoveleftUp, nullptr},
	{"+moveright", "Strafe right", IN_MoverightDown, nullptr},
	{"-moveright", nullptr, IN_MoverightUp, nullptr},
	{"+speed", "Walk or run", IN_SpeedDown, nullptr},
	{"-speed", nullptr, IN_SpeedUp, nullptr},
	{"+attack", "Primary Attack", IN_Button0Down, nullptr},
	{"-attack", nullptr, IN_Button0Up, nullptr},
	{"+use", "Use item", IN_Button5Down, nullptr},
	{"-use", nullptr, IN_Button5Up, nullptr},
	{"+force_grip", "Hold to use grip force power", IN_Button6Down, nullptr},
	{"-force_grip", nullptr, IN_Button6Up, nullptr},
	{"+altattack", "Alternate Attack", IN_Button7Down, nullptr},
	{"-altattack", nullptr, IN_Button7Up, nullptr},
	{"+useforce", "Use selected force power", IN_Button9Down, nullptr},
	{"-useforce", nullptr, IN_Button9Up, nullptr},
	{"+force_lightning", "Hold to use lightning force power", IN_Button10Down, nullptr},
	{"-force_lightning", nullptr, IN_Button10Up, nullptr},
	{"+force_drain", "Hold to use drain force power", IN_Button11Down, nullptr},
	{"-force_drain", nullptr, IN_Button11Up, nullptr},
	{"+button0", "Button 0", IN_Button0Down, nullptr},
	{"-button0", nullptr, IN_Button0Up, nullptr},
	{"+button1", "Button 1", IN_Button1Down, nullptr},
	{"-button1", nullptr, IN_Button1Up, nullptr},
	{"+button2", "Button 2", IN_Button2Down, nullptr},
	{"-button2", nullptr, IN_Button2Up, nullptr},
	{"+button3", "Button 3", IN_Button3Down, nullptr},
	{"-button3", nullptr, IN_Button3Up, nullptr},
	{"+button4", "Button 4", IN_Button4Down, nullptr},
	{"-button4", nullptr, IN_Button4Up, nullptr},
	{"+button5", "Button 5", IN_Button5Down, nullptr},
	{"-button5", nullptr, IN_Button5Up, nullptr},
	{"+button6", "Button 6", IN_Button6Down, nullptr},
	{"-button6", nullptr, IN_Button6Up, nullptr},
	{"+button7", "Button 7", IN_Button7Down, nullptr},
	{"-button7", nullptr, IN_Button7Up, nullptr},
	{"+button8", "Button 8", IN_Button8Down, nullptr},
	{"-button8", nullptr, IN_Button8Up, nullptr},
	{"+button9", "Button 9", IN_Button9Down, nullptr},
	{"-button9", nullptr, IN_Button9Up, nullptr},
	{"+button10", "Button 10", IN_Button10Down, nullptr},
	{"-button10", nullptr, IN_Button10Up, nullptr},
	{"+button11", "Button 11", IN_Button11Down, nullptr},
	{"-button11", nullptr, IN_Button11Up, nullptr},
	{"+button12", "Button 12", IN_Button12Down, nullptr},
	{"-button12", nullptr, IN_Button12Up, nullptr},
	{"+button13", "Button 13", IN_Button13Down, nullptr},
	{"-button13", nullptr, IN_Button13Up, nullptr},
	{"+button14", "Button 14", IN_Button14Down, nullptr},
	{"-button14", nullptr, IN_Button14Up, nullptr},
	{"+button15", "Button 15", IN_Button15Down, nullptr},
	{"-button15", nullptr, IN_Button15Up, nullptr},

	{"+button16", "Button 16", IN_Button16Down, nullptr},
	{"-button16", nullptr, IN_Button16Up, nullptr},
	{"+button17", "Button 17", IN_Button17Down, nullptr},
	{"-button17", nullptr, IN_Button17Up, nullptr},
	{"+button18", "Button 18", IN_Button18Down, nullptr},
	{"-button18", nullptr, IN_Button18Up, nullptr},
	{"+button19", "Button 19", IN_Button19Down, nullptr},
	{"-button19", nullptr, IN_Button19Up, nullptr},
	{"+button20", "Button 20", IN_Button20Down, nullptr},
	{"-button20", nullptr, IN_Button20Up, nullptr},

	{"+mlook", "Hold to use mouse look", IN_MLookDown, nullptr},
	{"-mlook", nullptr, IN_MLookUp, nullptr},
	{"sv_saberswitch", "Holster/activate lightsaber", IN_GenCMD1, nullptr},
	{"engage_duel", "Engage private duel", IN_GenCMD2, nullptr},
	{"force_heal", "Use heal force power", IN_GenCMD3, nullptr},
	{"force_speed", "Activate speed force power", IN_GenCMD4, nullptr},
	{"force_pull", "Use pull force power", IN_GenCMD5, nullptr},
	{"force_distract", "Activate mind trick force power", IN_GenCMD6, nullptr},
	{"force_rage", "Activate rage force power", IN_GenCMD7, nullptr},
	{"force_protect", "Activate protect force power", IN_GenCMD8, nullptr},
	{"force_absorb", "Activate absorb force power", IN_GenCMD9, nullptr},
	{"force_healother", "Use team heal force power", IN_GenCMD10, nullptr},
	{"force_forcepowerother", "Use team energize force power", IN_GenCMD11, nullptr},
	{"force_seeing", "Activate seeing force power", IN_GenCMD12, nullptr},
	{"use_seeker", "Use seeker drone item", IN_GenCMD13, nullptr},
	{"use_field", "Use forcefield item", IN_GenCMD14, nullptr},
	{"use_bacta", "Use bacta item", IN_GenCMD15, nullptr},
	{"use_electrobinoculars", "Use electro binoculars item", IN_GenCMD16, nullptr},
	{"zoom", "Use binoculars item", IN_GenCMD17, nullptr},
	{"use_sentry", "Use sentry gun item", IN_GenCMD18, nullptr},
	{"saberAttackCycle", "Switch lightsaber attack styles", IN_GenCMD19, nullptr},
	{"force_throw", "Use push force power", IN_GenCMD20, nullptr},
	{"use_jetpack", "Use jetpack item", IN_GenCMD21, nullptr},
	{"use_bactabig", "Use big bacta item", IN_GenCMD22, nullptr},
	{"use_healthdisp", "Use health dispenser item", IN_GenCMD23, nullptr},
	{"use_ammodisp", "Use ammo dispenser item", IN_GenCMD24, nullptr},
	{"use_eweb", "Use e-web item", IN_GenCMD25, nullptr},
	{"use_cloak", "Use cloaking item", IN_GenCMD26, nullptr},
	{"taunt", "Taunt", IN_GenCMD27, nullptr},
	{"bow", "Bow", IN_GenCMD28, nullptr},
	{"meditate", "Meditate", IN_GenCMD29, nullptr},
	{"flourish", "Flourish", IN_GenCMD30, nullptr},
	{"gloat", "Gloat", IN_GenCMD31, nullptr},
	{"surrender", "Surrender", IN_GenCMD32, nullptr},
	{"useGivenForce", "Use specified force power", IN_UseGivenForce, nullptr},
	{"automap_button", "Show/hide automap", IN_AutoMapButton, nullptr},
	{"automap_toggle", "Show/hide radar", IN_AutoMapToggle, nullptr},
	{"voicechat", "Open voice chat menu", IN_VoiceChatButton, nullptr},
	{nullptr, nullptr, nullptr, nullptr}
};

/*
============
CL_InitInput
============
*/
void CL_InitInput(void)
{
	Cmd_AddCommandList(inputCmds);

	cl_nodelta = Cvar_Get("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get("cl_debugMove", "0", 0);
}

/*
============
CL_ShutdownInput
============
*/
void CL_ShutdownInput(void)
{
	Cmd_RemoveCommandList(inputCmds);
}