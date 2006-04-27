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

#ifndef FlexiRanks_h
#define FlexiRanks_h

#include "Rank.h"

namespace Dynagraph {
namespace DynaDAG {

struct FlexiRankXlator {
	typedef int index;
	bool Above(index a,index b) {
		return a<b;
	}
	bool Below(index a,index b) {
		return a>b;
	}
	index CoordToRank(double y) {
#ifndef DOWN_GREATER
		return -ROUND(y/div_);
#else
		return ROUND(y/div_);
#endif
	}
	double RankToCoord(index r) {
#ifndef DOWN_GREATER
		return -r*div_;
#else
		return r*div_;
#endif
	}
	index HeightToDRank(double dy) {
		return ROUND(dy/div_);	
	}
	explicit FlexiRankXlator(double div) : div_(div) {}
	double div_;
};

struct CompRank {
	bool operator()(Rank *r1,Rank *r2) const {
#ifndef DOWN_GREATER
		return r1->yBase>r2->yBase;
#else
		return r1->yBase<r2->yBase;
#endif
	}
};
struct FlexiRanks : std::set<Rank*,CompRank> {
	typedef FlexiRankXlator::index index;
	FlexiRankXlator rankXlate_;
	FlexiRanks(FlexiRankXlator rankXlate) : rankXlate_(rankXlate) {}
	FlexiRanks(FlexiRanks &o) : rankXlate_(o.rankXlate_) {
		*this = o;
	}
	~FlexiRanks() {
		reset();
	}
	void reset() {
		iterator ri = begin();
		while(ri!=end()) {
			Rank *r = *ri;
			iterator er = ri++;
			erase(er);
			delete r;
		}
	}
	FlexiRanks &operator =(FlexiRanks &o) {
		reset();
		oldRanks = o.oldRanks;
		newRanks = o.newRanks;
		rankXlate_ = o.rankXlate_;
		for(iterator ri = o.begin(); ri!=o.end(); ++ri) {
			Rank *nr = new Rank(**ri);
			insert(nr);
		}
		return *this;
	}
	Rank *front() { return *begin(); }
	Rank *back() { return *rbegin(); }
	bool Above(index a,index b) {
		return rankXlate_.Above(a,b);
	}
	bool Below(index a,index b) {
		return rankXlate_.Below(a,b);
	}
	index CoordToRank(double y) {
		return rankXlate_.CoordToRank(y);
	}
	double RankToCoord(index r) {
		return rankXlate_.RankToCoord(r);
	}
	index HeightToDRank(double dy) {
		return rankXlate_.HeightToDRank(dy);
	}
	index Low() { 
		if(empty()) 
			return 0; 
		else return rankXlate_.CoordToRank(front()->yBase); 
	}
	index High() { 
		if(empty()) return 0; 
		else return rankXlate_.CoordToRank(back()->yBase); 
	}
	index Up(index r) {
		if(r==INT_MIN)
			return r;
		iterator ri = GetIter(r);
		if(ri==begin())
			return INT_MIN;
		return IndexOfIter(--ri);
	}
	index Down(index r) {
		if(r==INT_MAX)
			return r;
		iterator ri = GetIter(r);
		if(++ri==end())
			return INT_MAX;
		return IndexOfIter(ri);
	}
	iterator GetIter(index r) {
		Rank q(8.); // stupid set lookup deserves rewrite
		q.yBase = rankXlate_.RankToCoord(r);
		return find(&q);
	}
	Rank *GetRank(index r)	{
		iterator ri = GetIter(r);
		if(ri==end())
			return 0;
		else
			return *ri;
	}
	iterator EnsureRank(index r,double sep) {
		assert(r!=INT_MAX && r!=INT_MIN); // off bottom or top
		iterator ri = GetIter(r);
		if(ri==end()) {
			Rank *rank = new Rank(sep);
			rank->yBase = rankXlate_.RankToCoord(r);
			ri = insert(rank).first;
		}
		return ri;
	}
	void RemoveRank(iterator ri) {
		Rank *del = *ri;
		erase(ri);
		delete del;
	}
	index IndexOfIter(iterator ri) {
		return rankXlate_.CoordToRank((*ri)->yBase);
	}
	void Check();
	typedef std::vector<index> IndexV;
	IndexV newRanks,oldRanks;
};

} // namespace DynaDAG
} // namespace Dynagraph

#endif // FlexiRanks_h