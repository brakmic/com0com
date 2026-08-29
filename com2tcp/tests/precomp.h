/*
 * Test-specific precompiled header for com2tcp tests.
 *
 * The production precomp.h is sufficient for us since com2tcp only needs
 * winsock2.h, windows.h, stdio.h, and vector. No WDK headers required.
 * This header exists so tests can override include paths if needed.
 */

#ifndef _TEST_PRECOMP_H_
#define _TEST_PRECOMP_H_

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#pragma warning(disable:4710)
#pragma warning(push, 3)
#include <vector>
#pragma warning(pop)
using namespace std;

#include "../utils.h"

#endif /* _TEST_PRECOMP_H_ */
