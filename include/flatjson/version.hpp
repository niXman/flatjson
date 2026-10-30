
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2026 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__VERSION_HPP
#define __FLATJSON__VERSION_HPP

/*************************************************************************************************/

#define __FJ__STRINGIZE_I(x) #x
#define __FJ__STRINGIZE(x) __FJ__STRINGIZE_I(x)

#define FJ_VERSION_MAJOR 1
#define FJ_VERSION_MINOR 0
#define FJ_VERSION_BUGFIX 0

// FJ_VERSION % 100 is the patch level
// FJ_VERSION / 100 % 1000 is the minor version
// FJ_VERSION / 100000 is the major version

#define FJ_VERSION \
    (FJ_VERSION_MAJOR * 100000 \
        + FJ_VERSION_MINOR * 100 \
        + FJ_VERSION_BUGFIX)

#define FJ_VERSION_GET_MAJOR(x)  ((x) / 100000)
#define FJ_VERSION_GET_MINOR(x)  (((x) / 100) % 1000)
#define FJ_VERSION_GET_BUGFIX(x) ((x) % 100)

#define FJ_VERSION_STRING \
    __FJ__STRINGIZE(FJ_VERSION_MAJOR) \
    "." __FJ__STRINGIZE(FJ_VERSION_MINOR) \
    "." __FJ__STRINGIZE(FJ_VERSION_BUGFIX)

/*************************************************************************************************/

#endif // __FLATJSON__VERSION_HPP
