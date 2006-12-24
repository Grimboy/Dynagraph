/**********************************************************
*            This software is part of Dynagraph           *
*                http://www.dynagraph.org/                *
*                                                         *
*            Copyright (c) 2006 Gordon Woodhull           *
*        Portions Copyright (c) 1994-2005 AT&T Corp.      *
*  Licensed under the Common Public License, Version 1.0  *
*                                                         *
*      Dynagraph originates in the Graphviz toolset       *
*                   http://graphviz.org                   *
*        Information and Software Systems Research        *
*              AT&T Research, Florham Park NJ             *
*                       Many thanks.                      *
**********************************************************/


#ifndef NSRankerModel_h
#define NSRankerModel_h

#include "ConstraintGraph.h"

namespace Dynagraph {
namespace DynaDAG {

struct NSRankerModelNode {
	LlelConstraintGraph::NodeConstraints topC,bottomC;
	bool hit, // for rank dfs
		rankFixed; // whether nailed in Y

	NSRankerModelNode() : hit(false),rankFixed(false) {}
	// do not copy constraints or hit-flag
	NSRankerModelNode(const NSRankerModelNode &other) :
		hit(false),
		rankFixed(other.rankFixed) {}
	NSRankerModelNode &operator=(NSRankerModelNode &other) {
		hit = false;
		rankFixed = other.rankFixed;
		return *this;
	}
};
struct NSRankerModelEdge {
	int weight,backweight;
	double length,backlength;
	LlelConstraintGraph::Node *weak;
	LlelConstraintGraph::Edge *strong;
	NSRankerModelEdge() : weight(1),backweight(1),length(0.),backlength(.0),weak(0),strong(0) {}
};
struct NSRankerModelGraph {
	LlelConstraintGraph cg;
	LlelConstraintGraph::Node *top_; // to pull loose nodes upward
	NSRankerModelGraph() : top_(cg_.create_node()) {}
};

struct NSRankerModelNodeData : Name,NSRankerModelNode {};
struct NSRankerModelEdgeData : Name,NSRankerModelEdge {};

typedef LGraph<ADTisCDT,false,NSRankerModelGraph,NSRankerModelNodeData,NSRankerModelEdgeData> NSRankerModel;

} // namespace DynaDAG
} // namespace Dynagraph

#endif // NSRankerModel_h
