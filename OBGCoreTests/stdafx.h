// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include <string>
// Headers for CppUnitTest
#include "CppUnitTest.h"
#include "LinearMath\btVector3.h"

// TODO: reference additional headers your program requires here
std::wstring ToString(const btVector3 &vec);