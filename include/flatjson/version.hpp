
// ----------------------------------------------------------------------------
//                              Apache License
//                        Version 2.0, January 2004
//                     http://www.apache.org/licenses/
//
// This file is part of FlatJSON(https://github.com/niXman/flatjson) project.
//
// Copyright (c) 2019-2022 niXman (github dot nixman dog pm.me). All rights reserved.
// ----------------------------------------------------------------------------

#ifndef __FLATJSON__VERSION_HPP
#define __FLATJSON__VERSION_HPP

/*************************************************************************************************/

#define __FJ__STRINGIZE_I(x) #x
#define __FJ__STRINGIZE(x) __FJ__STRINGIZE_I(x)

/*************************************************************************************************/

// FJ_VERSION >> 24 - is the major version
// FJ_VERSION >> 16 - is the minor version
// FJ_VERSION >> 8  - is the bugfix level

#define FJ_VERSION_MAJOR 0
#define FJ_VERSION_MINOR 0
#define FJ_VERSION_BUGFIX 2

#define FJ_VERSION_HEX \
     ((FJ_VERSION_MAJOR << 24) \
     | (FJ_VERSION_MINOR << 16) \
     | (FJ_VERSION_BUGFIX << 8))

#define FJ_VERSION_STRING \
        __FJ__STRINGIZE(FJ_VERSION_MAJOR) \
    "." __FJ__STRINGIZE(FJ_VERSION_MINOR) \
    "." __FJ__STRINGIZE(FJ_VERSION_BUGFIX)

/*************************************************************************************************/

#endif // __FLATJSON__VERSION_HPP
