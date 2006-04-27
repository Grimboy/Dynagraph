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


#include "DynaDAG.h"

using namespace std;

namespace Dynagraph {
namespace DynaDAG {

const double EPSILON = 1.0e-5; // DBL_EPSILON*10; // 1.0e-36;

Region &getRegion(DDModel::Node *n) {
	DynaDAGLayout::Node *vn = gd<DDNode>(n).multi->layoutN;
	return gd<NodeGeom>(vn).region;
}
double Config::LeftExtent(DDModel::Node *n) {
	if(!n)
		return 0.0;
	if(gd<DDNode>(n).amEdgePart())
		return gd<GraphGeom>(whole).separation.x/2.0;
	if(getRegion(n).boundary.valid)
		return -getRegion(n).boundary.l;
	return EPSILON;
}
double Config::RightExtent(DDModel::Node *n) {
	if(!n)
		return 0.0;
	if(gd<DDNode>(n).amEdgePart())
		return gd<GraphGeom>(whole).separation.x/2.0;
	double r = 0.0;
	if(getRegion(n).boundary.valid)
		r = getRegion(n).boundary.r;
	DynaDAGLayout::Node *ln = gd<DDNode>(n).multi->layoutN;
	if(current->find_edge(ln,ln))
		r += gd<GraphGeom>(whole).separation.x;
	return max(r,EPSILON);
}
double Config::TopExtent(DDModel::Node *n) {
	if(!n)
		return 0.0;
	if(gd<DDNode>(n).amEdgePart())
		return EPSILON;
	if(getRegion(n).boundary.valid)
		return getRegion(n).boundary.t;
	return EPSILON;
}
double Config::BottomExtent(DDModel::Node *n) {
	if(!n)
		return 0.0;
	if(gd<DDNode>(n).amEdgePart())
		return EPSILON;
	if(getRegion(n).boundary.valid)
		return -getRegion(n).boundary.b;
	return EPSILON;
}
/*
Coord Config::NodeSize(DDModel::Node *n) {
	Coord ret;
	if(!n)
		ret.x = ret.y = 0.0;
	else if(gd<DDNode>(n).amEdgePart()) {
		ret.x = gd<GraphGeom>(whole).separation.x; // separation is good enough
		ret.y = EPSILON;
	}
	else {
		DynaDAGLayout::Node *vn = gd<DDNode>(n).multi->layoutN;
		Region *region = gd<NodeGeom>(vn).region;
		ret.x = ret.y = 0;
		if(region) {
			ret.x = region.boundary.Width();
			ret.y = region.boundary.Height();
		}
		if(ret.x<=0)
			ret.x = EPSILON;
		if(ret.y<=0)
			ret.y = EPSILON;
	}
	return ret;
}
*/
double Config::UVSep(DDModel::Node *left,DDModel::Node *right) {
	double s0 = RightExtent(left),
		s1 = LeftExtent(right);
	return s0+s1 + gd<GraphGeom>(whole).separation.x;
}
double Config::CoordBetween(DDModel::Node *L, DDModel::Node *R) {
	double x;
	if(L || R) {
		double lx = L ? gd<DDNode>(L).cur.x : gd<DDNode>(R).cur.x - 2.0 * UVSep(0,R),
			rx = R ? gd<DDNode>(R).cur.x : gd<DDNode>(L).cur.x + 2.0 * UVSep(L,0);
		x = (lx + rx) / 2.0;
	}
	else x = 0.0;
	return x;
}
DDModel::Node *Config::RelNode(DDModel::Node *n, int offset) {
	unsigned pos = gd<DDNode>(n).order + offset;
	if(pos < 0)
		return 0;
	Rank *rank = ranking.GetRank(gd<DDNode>(n).rank);
	assert(rank);
	if(pos >= rank->order.size())
		return 0;
	return rank->order[pos];
}
DDModel::Node *Config::Right(DDModel::Node *n) {
	return RelNode(n,1);
}
DDModel::Node *Config::Left(DDModel::Node *n) {
	return RelNode(n,-1);
}
void Config::InstallAtRight(DDModel::Node *n, int r) {
	Rank *rank = *ranking.EnsureRank(r,gd<GraphGeom>(current).separation.y);
	DDModel::Node *right = rank->order.size()?rank->order.back():0;
	double x = right?gd<DDNode>(right).cur.x + UVSep(right,n):0.0;
	assert(!right||x>gd<DDNode>(right).cur.x);
	rank->order.push_back(n);
	DDNode &ddn = gd<DDNode>(n);
	ddn.rank = r;
	ddn.order = rank->order.size()-1;
	ddn.inConfig = true;
	ddn.cur.valid = true;
	ddn.cur.x = x;
	ddn.cur.y = rank->yBase; // estimate
	model.dirty().insert(n);
}
#ifdef X_STRICTLY_INCREASING
// allow nodes of multinode to co-locate temporarily
inline bool sameNode(DDModel::Node *n1,DDModel::Node *n2) {
	return gd<DDNode>(n1).amNodePart() && gd<DDNode>(n1).multi==gd<DDNode>(n2).multi;
}
#endif
void Config::InstallAtOrder(DDModel::Node *n, int r, unsigned o, double x) {
	Rank *rank = *ranking.EnsureRank(r,gd<GraphGeom>(current).separation.y);
	assert(o<=rank->order.size() && o>=0);
	NodeV::iterator i = rank->order.begin()+o;
	i = rank->order.insert(i,n);
	DDNode &ddn = gd<DDNode>(n);
	ddn.rank = r;
	ddn.order = i - rank->order.begin();
	ddn.inConfig = true;
	InvalidateAdjMVals(n);
	ddn.cur.valid = true;
	ddn.cur.x = x;
	ddn.cur.y = rank->yBase; // estimate

	for(++i; i!=rank->order.end(); ++i) {
		gd<DDNode>(*i).order++;
		InvalidateAdjMVals(*i);
	}
	model.dirty().insert(n);
}
void Config::InstallAtOrder(DDModel::Node *n, int r, unsigned o) {
	Rank *rank = *ranking.EnsureRank(r,gd<GraphGeom>(current).separation.y);
	assert(o<=rank->order.size() && o>=0);
	DDModel::Node *L = o>0?rank->order[o-1]:0,
		*R = o<rank->order.size()?rank->order[o]:0;
	InstallAtOrder(n,r,o,CoordBetween(L,R));
}
struct XLess {
	bool operator()(DDModel::Node *u,double x) {
		return gd<DDNode>(u).cur.x<x;
	}
};
NodePair Config::NodesAround(int r,double x) {
	Rank *rank = *ranking.EnsureRank(r,gd<GraphGeom>(current).separation.y);
	NodeV::iterator i = lower_bound(rank->order.begin(),rank->order.end(),x,XLess());
	DDModel::Node *L = i==rank->order.begin()?0:*(i-1),
		*R = i==rank->order.end()?0:*i;
	return make_pair(L,R);
}
void Config::InstallAtPos(DDModel::Node *n, int r, double x) {
	NodePair lr = NodesAround(r,x);
	InstallAtOrder(n,r,lr.second?gd<DDNode>(lr.second).order:lr.first?gd<DDNode>(lr.first).order+1:0,x);
}
void Config::Exchange(DDModel::Node *u, DDModel::Node *v) {
	DDNode &ddu = gd<DDNode>(u),
		&ddv = gd<DDNode>(v);
	assert(ddu.inConfig);
	assert(ddv.inConfig);
	assert(ddu.rank == ddv.rank);
	report(r_exchange,"exchange %p of %s %p: x %.3f r %d o %d\n\tw/ %p of %s %p: x %.3f r %d o %d\n",
		u,type(u),thing(u),ddu.cur.x,ddu.rank,ddu.order,
		v,type(v),thing(v),ddv.cur.x,ddv.rank,ddv.order);

	Rank *rank = ranking.GetRank(ddu.rank);
	int upos = ddu.order,
		vpos = ddv.order;
	//assert(vpos==upos+1);

	/* delete any LR constraint between these nodes */
	xconOwner->DeleteLRConstraint(u,v);
	/*xconOwner->RemoveNodeConstraints(u);
	xconOwner->RemoveNodeConstraints(v);*/

	rank->order[vpos] = u; gd<DDNode>(u).order = vpos;
	rank->order[upos] = v; gd<DDNode>(v).order = upos;
	//swap(gd<DDNode>(u).cur.x,gd<DDNode>(v).cur.x); // keep x order consistent
	InvalidateAdjMVals(u);
	InvalidateAdjMVals(v);
	model.dirty().insert(u);
	model.dirty().insert(v);
}
void Config::RemoveNode(DDModel::Node *n) {
	xconOwner->RemoveNodeConstraints(n);
	InvalidateAdjMVals(n);
	DDNode &ddn = gd<DDNode>(n);
	if(!ddn.inConfig)
		return;
	Rank *rank = ranking.GetRank(gd<DDNode>(n).rank);
	int pos = gd<DDNode>(n).order;
	assert(rank->order[pos] == n);
	NodeV::iterator i = rank->order.begin()+pos;
	rank->order.erase(i);
	if(i!=rank->order.end())
		model.dirty().insert(*i); // re-constrain right node
	for(;i!=rank->order.end(); ++i)
		gd<DDNode>(*i).order--;
	ddn.inConfig = false;
	ddn.rank = INT_MIN;
    /*
	if(ddn.multi)
		ddn.multi->coordFixed = false;
    */
	// ddn.orderFixed = false;
}
void Config::Restore(Ranks &backup) {
	ranking = backup;
	for(Ranks::iterator ri = ranking.begin(); ri!=ranking.end(); ++ri) {
		Rank *r = *ri;
		for(NodeV::iterator ni = r->order.begin(); ni!=r->order.end(); ++ni) {
			int o = int(ni - r->order.begin());
			if(gd<DDNode>(*ni).order != o) {
				InvalidateAdjMVals(*ni);
				gd<DDNode>(*ni).order = o;
			}
		}
	}
}

} // namespace DynaDAG
} // namespace Dynagraph
