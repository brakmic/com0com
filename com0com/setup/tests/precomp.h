/*
 * Test-specific precompiled header replacement.
 *
 * The production precomp.h includes <newdev.h> which is only in the WDK.
 * This minimal header includes only the Windows SDK headers needed to
 * compile params.cpp and comdb.cpp in the test binary.
 */

#ifndef _TEST_PRECOMP_H_
#define _TEST_PRECOMP_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cfgmgr32.h>
#include <regstr.h>

// Stub the WDK-only newdev.h types needed by params.cpp
// UpdateDriverForPlugAndPlayDevices is declared in newdev.h
// but params.cpp only uses its type in a function we don't call from tests.

// Stub INSTALLFLAG_* constants needed by setup code
#define INSTALLFLAG_FORCE   0x00000001
#define INSTALLFLAG_READONLY 0x00000002
#define INSTALLFLAG_NONINTERACTIVE 0x00000004

// Suppress newdev.h — provide minimal type stubs
typedef void *HANDLE;  // already in windows.h

#include <setupapi.h>
#include <shlwapi.h>

#endif /* _TEST_PRECOMP_H_ */
