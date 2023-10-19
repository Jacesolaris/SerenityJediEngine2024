/*
===========================================================================
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

#ifndef __G_ROFF_H__
#define __G_ROFF_H__

#include "../qcommon/q_shared.h"

// ROFF Defines
//-------------------
constexpr auto ROFF_VERSION = 1; // ver # for the (R)otation (O)bject (F)ile (F)ormat;
constexpr auto ROFF_VERSION2 = 2; // ver # for the (R)otation (O)bject (F)ile (F)ormat;
constexpr auto MAX_ROFFS = 128; // hard coded number of max roffs per level, sigh..;
constexpr auto ROFF_SAMPLE_RATE = 20; // 10hz;

// ROFF Header file definition
//-------------------------------
using roff_hdr_t = struct roff_hdr_s
{
	char mHeader[4]; // should be "ROFF" (Rotation, Origin File Format)
	int mVersion;
	float mCount; // There isn't any reason for this to be anything other than an int, sigh...
	//
	//		Move - Rotate data follows....vec3_t delta_origin, vec3_t delta_rotation
	//
};

// ROFF move rotate data element
//--------------------------------
using move_rotate_t = struct move_rotate_s
{
	vec3_t origin_delta;
	vec3_t rotate_delta;
};

using roff_hdr2_t = struct roff_hdr2_s
//-------------------------------
{
	char mHeader[4]; // should match roff_string defined above
	int mVersion; // version num, supported version defined above
	int mCount; // I think this is a float because of a limitation of the roff exporter
	int mFrameRate; // Frame rate the roff should be played at
	int mNumNotes; // number of notes (null terminated strings) after the roff data
};

using move_rotate2_t = struct move_rotate2_s
//-------------------------------
{
	vec3_t origin_delta;
	vec3_t rotate_delta;
	int mStartNote, mNumNotes; // note track info
};

// a precached ROFF list
//-------------------------
using roff_list_t = struct roff_list_s
{
	int type; // roff type number, 1-old, 2-new
	char* file_name; // roff filename
	int frames; // number of roff entries
	void* data; // delta move and rotate vector list
	int mFrameTime; // frame rate
	int mLerp; // Lerp rate (FPS)
	int mNumNoteTracks;
	char** mNoteTrackIndexes;
};

extern roff_list_t roffs[];
extern int num_roffs;

// Function prototypes
//-------------------------
int G_LoadRoff(const char* file_name);
void G_Roff(gentity_t* ent);
void G_SaveCachedRoffs();
void G_LoadCachedRoffs();

#endif
