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

// Filename:-	tr_font.h
//
// font support

// This file is shared in the single and multiplayer codebases, so be CAREFUL WHAT YOU ADD/CHANGE!!!!!

#pragma once

void R_ShutdownFonts();
void R_InitFonts();
int RE_RegisterFont(const char* psName);
int RE_Font_StrLenPixels(const char* psText, int iFontHandle, float fScale = 1.0f);
int RE_Font_StrLenChars(const char* psText);
int RE_Font_HeightPixels(int iFontHandle, float fScale = 1.0f);
void RE_Font_DrawString(int ox, int oy, const char* psText, const float* rgba, int iFontHandle, int iMaxPixelWidth, float fScale = 1.0f);

// Dammit, I can't use this more elegant form because of !@#@!$%% VM code... (can't alter passed in ptrs, only contents of)
//
//unsigned int AnyLanguage_ReadCharFromString( const char **ppsText, qboolean *pbIsTrailingPunctuation = NULL);
//
// so instead we have to use this messier method...
//
unsigned int AnyLanguage_ReadCharFromString(char* psText, int* piAdvanceCount, qboolean* pbIsTrailingPunctuation = nullptr);
unsigned int AnyLanguage_ReadCharFromString(char** psText, qboolean* pbIsTrailingPunctuation = nullptr);

qboolean Language_IsAsian();
qboolean Language_UsesSpaces();
