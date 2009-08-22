/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file osx_stdafx.h OSX is different on some places. */

#ifndef MACOS_STDAFX_H
#define MACOS_STDAFX_H

#define __STDC_LIMIT_MACROS
#include <stdint.h>

/* We need to include this first as that "depends" on the compiler's setting
 * of __LP64__. So before we define __LP64__ so it can be used. */
#include <sys/cdefs.h>
#include <unistd.h>

/* __LP64__ only exists in 10.5 and higher */
#if defined(__APPLE__) && !defined(__LP64__)
#	define __LP64__ 0
#endif

/* Check for mismatching 'architectures' */
#if (__LP64__ && !defined(_SQ64)) || (!__LP64__ && defined(_SQ64))
#	error "Compiling 64 bits without _SQ64 set! (or vice versa)"
#endif

#define MAC_OS_X_VERSION_MIN_REQUIRED MAC_OS_X_VERSION_10_3
#include <AvailabilityMacros.h>

/* Name conflict */
#define Rect        OTTDRect
#define Point       OTTDPoint
#define WindowClass OTTDWindowClass

#include <CoreServices/CoreServices.h>

#undef Rect
#undef Point
#undef WindowClass

/* remove the variables that CoreServices defines, but we define ourselves too */
#undef bool
#undef false
#undef true

/* Name conflict */
#define GetTime OTTD_GetTime

#define SL_ERROR OSX_SL_ERROR

/* NSInteger and NSUInteger are part of 10.5 and higher. */
#ifndef NSInteger
#if __LP64__
typedef long NSInteger;
typedef unsigned long NSUInteger;
#else
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif /* __LP64__ */
#endif /* NSInteger */

#endif /* MACOS_STDAFX_H */
