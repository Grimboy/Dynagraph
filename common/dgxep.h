/**********************************************************
*      This software is part of the graphviz toolset      *
*                http://www.graphviz.org/                 *
*                                                         *
*            Copyright (c) 1994-2005 AT&T Corp.           *
*                and is licensed under the                *
*            Common Public License, Version 1.0           *
*                      by AT&T Corp.                      *
*                                                         *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
*                                                         *
*               This version available from               *
*                   http://dynagraph.org                  *
**********************************************************/


#ifndef dgxep_h
#define dgxep_h

#include "StringDict.h"

namespace Dynagraph {

// a base for all exceptions so we can report the basics
struct DGException {
    DString exceptype;
    bool fatal;
    DGException(DString exceptype,bool fatal = false) : exceptype(exceptype),fatal(fatal) {}
};
// pretty dopey - eliminate a few more chars in xep defs
struct DGException2 : DGException {
    DString param;
    DGException2(DString exceptype,DString param,bool fatal = false) : DGException(exceptype,fatal),param(param) {}
};

} // namespace Dynagraph

#endif //dgxep_h
