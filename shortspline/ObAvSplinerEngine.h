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
**********************************************************/

#include "common/Dynagraph.h"
struct ObAvSplinerEngine : Server {
	ObAvSplinerEngine(Layout *client,Layout *current) : Server(client,current) {}
	// Server
	void Process(ChangeQueue &changeQ);
};
