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

// Current version of the single player game
#ifndef STV_VERSION_H
#define STV_VERSION_H
#include "../win32/AutoVersion.h"

#ifdef _DEBUG
#define	Q3_VERSION		"(debug)SerenityJediEngine2024-SP: " VERSION_STRING_DOTTED
#elif defined FINAL_BUILD
#define	Q3_VERSION		"SerenityJediEngine2024-SP: " VERSION_STRING_DOTTED
#else
#define	Q3_VERSION		"(internal)SerenityJediEngine2024-SP: " VERSION_STRING_DOTTED
#endif
// end
#endif // STV_VERSION_H
