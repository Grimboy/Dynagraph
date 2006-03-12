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

#ifndef DynaDAGLayout_h
#define DynaDAGLayout_h

#include "common/LayoutAttrs.h"

namespace Dynagraph {
namespace DynaDAG {

typedef NamedGraph<ADTisCDT,GraphAttrs,NodeAttrs,EdgeAttrs,LayoutUpdates,LayoutUpdates,LayoutUpdates> DynaDAGLayout;

} // namespace DynaDAG
} // namespace Dynagraph


#endif // DynaDAGLayout_h
