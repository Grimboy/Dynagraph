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


#ifndef stringsIn_h
#define stringsIn_h

#include <sstream>
#include "Transform.h"
#include "AttrValues.h"

namespace Dynagraph {

struct NonSizeableShape {};
struct UnknownShape {};

void clearRemoved(const StrAttrs &current,const StrAttrs &apply,StrAttrs &ret);
void ensureAttr(const StrAttrs &att,StrAttrs &A, DString name);
bool shapeChanged(const StrAttrs &oldAttrs,const StrAttrs &newAttrs);
PolyDef readPolyDef(Transform *trans,StrAttrs &attrs);
	
template<typename Layout>
Update stringsIn(Transform *trans,bool useDotDefaults,Layout *l,const StrAttrs &attrs,bool clearOld) {
	using std::istringstream;
	StrAttrs allChanges = attrs;
	if(clearOld)
		clearRemoved(gd<StrAttrs>(l),attrs,allChanges);
    StrAttrs2 &att = gd<StrAttrs2>(l);
    ensureAttr(att,allChanges,"resolution");
    ensureAttr(att,allChanges,"separation");
    ensureAttr(att,allChanges,"defaultsize");
	for(StrAttrs::const_iterator ai = allChanges.begin(); ai!=allChanges.end(); ++ai) {
        DString name = ai->first,
            value = ai->second;
		if(name=="resolution") {
			if(value.empty())
                value = useDotDefaults?"1,5":"0.1,0.1";
			istringstream s(value);
			s >> gd<GraphGeom>(l).resolution;

		}
		else if(name=="separation") {
			if(value.empty())
                value = useDotDefaults?"24,24":"0.5,0.5";
			istringstream s(value);
			s >> gd<GraphGeom>(l).separation;
		}
		else if(name=="defaultsize") {
			if(value.empty())
                value = useDotDefaults?"54,36":"1.5,1";
			istringstream s(value);
			s >> gd<GraphGeom>(l).defaultSize;
		}
		else if(name=="ticks") {
			if(value.empty())
                value = "0";
			istringstream s(value);
			s >> gd<GraphGeom>(l).ticks;
		}
		att.put(ai->first,value);
	}
	return 0;
}
template<typename Layout>
Update assureAttrs(Transform *trans,typename Layout::Node *n) {
	Update ret;
	StrAttrs &att = gd<StrAttrs>(n);
	StrAttrs2 &att2 = gd<StrAttrs2>(n);
	if(att["shape"].empty()) {
        DString value = (att.find("sides")!=att.end())?"polygon":"ellipse";
        if(att2.put("shape",value))
		    ret.flags |= DG_UPD_REGION;
	}
	StrAttrs::iterator ai;
	Coord size(.0,0.);
    if((ai=att.find("width"))!=att.end() && !ai->second.empty())
		sscanf(ai->second.c_str(),"%lf",&size.x);
	if((ai=att.find("height"))!=att.end() && !ai->second.empty())
		sscanf(ai->second.c_str(),"%lf",&size.y);
	if(size.x&&size.y)
		return ret;
	if(size.x||size.y) {
		if(!size.y)
			size.y = size.x;
		if(!size.x)
			size.x = size.y;
	}
	else
		size = trans->outSize(gd<GraphGeom>(n->g).defaultSize);
	char buf[20];
	sprintf(buf,"%f",size.x);
	bool ch = att2.put("width",buf);
	sprintf(buf,"%f",size.y);
	ch |= att2.put("height",buf);
    if(ch)
	    ret.flags |= DG_UPD_REGION;
	return ret;
}
template<typename Layout>
Update stringsIn(Transform *trans,typename Layout::Node *n,const StrAttrs &attrs,bool clearOld) {
	using std::istringstream;
	StrAttrs allChanges;
	if(clearOld)
		clearRemoved(gd<StrAttrs>(n),attrs,allChanges);
	const StrAttrs &A = clearOld?allChanges:attrs;
	Update ret;
	NodeGeom &ng = gd<NodeGeom>(n);
	StrAttrs &att = gd<StrAttrs>(n);
	StrAttrs::const_iterator ai;
	bool chshape = shapeChanged(att,A);
	if(chshape)
		ret.flags |= DG_UPD_POLYDEF;
	for(ai = A.begin(); ai!=A.end(); ++ai) {
		if(ai->first=="pos") {
            if(ai->second.empty())  // position intentionally removed
                ng.pos.valid = false;
            else {
			    ng.pos.valid = true;
			    istringstream stream(ai->second);
			    stream >> ng.pos;
			    ng.pos = trans->in(ng.pos);
            }
			ret.flags |= DG_UPD_MOVE;
		}
		else if(ai->first.compare(0,9,"labelsize")==0) {
			int i=ds2int(ai->first.substr(9));
			if(i>=0) {
				istringstream stream(ai->second);
				stream >> gd<NodeLabels>(n)[i].size;
				ret.flags |= DG_UPD_LABEL;
			}
		}
		else if(ai->first.compare(0,14,"labelalign")==0) {
			int i=ds2int(ai->first.substr(14));
			if(i>=0) {
				gd<NodeLabels>(n)[i].align = string2nlp(ai->second);
				ret.flags |= DG_UPD_LABEL;
			}
		}
		else if(ai->first.compare(0,2,"labelbounds")==0) {
			int i=ds2int(ai->first.substr(9));
			if(i>=0) {
				istringstream stream(ai->second);
				stream >> gd<NodeLabels>(n)[i].bounds;
				ret.flags |= DG_UPD_LABEL;
			}
		}
		gd<StrAttrs2>(n).put(ai->first,ai->second);
	}
	ret.flags |= assureAttrs<Layout>(trans,n).flags;
	if(ret.flags&(DG_UPD_REGION|DG_UPD_POLYDEF)) {
		if((ai = att.find("boundary"))!=att.end() && !ai->second.empty()) {
			const DString &s = ai->second;
			istringstream stream(s);
			ng.region.shape.Clear();
			stream >> ng.region.shape;
			ng.region.updateBounds();
			gd<Drawn>(n).clear();
			gd<Drawn>(n).push_back(ng.region.shape);
			gd<IfPolyDef>(n).whether = false;
			ret.flags |= DG_UPD_REGION|DG_UPD_DRAWN;
		}
		else if((ai = att.find("shape"))!=att.end() && (ai->second=="none" || ai->second=="plaintext")) {
			Coord size;
			ai = att.find("labelsize");
			if(ai != att.end()) {
				istringstream stream(ai->second);
				stream >> size;
			}
			else
				size = Coord(0.,.0);
			size = trans->inSize(size);
			gd<Drawn>(n).clear();
			ng.region.shape.Clear();
			ng.region.boundary = Rect(-size.x/2,size.y/2,size.x/2,-size.y/2);
			gd<IfPolyDef>(n).whether = false;
			ret.flags |= DG_UPD_REGION|DG_UPD_DRAWN;
		}
		else {
			gd<IfPolyDef>(n).whether = true;
			gd<PolyDef>(n) = readPolyDef(trans,att);
			ret.flags |= DG_UPD_POLYDEF;
		}
	}
	return ret.flags;
}
template<typename Layout>
Update stringsIn(Transform *trans,typename Layout::Edge *e,const StrAttrs &attrs,bool clearOld) {
	using std::istringstream;
	StrAttrs allChanges;
	if(clearOld)
		clearRemoved(gd<StrAttrs>(e),attrs,allChanges);
	const StrAttrs &A = clearOld?allChanges:attrs;
	Update ret;
	EdgeGeom &eg = gd<EdgeGeom>(e);
	for(StrAttrs::const_iterator ai = A.begin(); ai!=A.end(); ++ai) {
		bool skip = false;
		if(ai->first=="pos") {
		    const DString &s = ai->second;
            if(s.empty()) {
                eg.pos.Clear();
            }
            else {
			    assert(s.length());
			    DString::size_type begin = 0,end=s.size();
                // grr i know i'll get this wrong in some obscure way...
                // just sloppily trying to throw away the [e,x,y ][s,x,y ]
                for(int j = 0; j<2; ++j) {
					if(s[begin]=='e'||s[begin]=='s') {
						++begin;
                        for(int i = 0; i<2; ++i) {
                            assert(s[begin]==',');
							++begin;
							// much too expansive: -6.2.-9.4 would be one number...
                            while(s[begin]=='-'||isdigit(s[begin])||s[begin]=='.')
								++begin;
                        }
					}
                    while(s[begin]==' ') ++begin;
                }
			    DString::size_type i = s.find(';',begin);
			    if(i!=DString::npos)
			        end = i;
			    Line newline;
			    istringstream stream(s.substr(begin,end-begin));
			    stream >> newline;
			    if(transformShape(trans,newline)) {
				    eg.pos = newline;
				    ret.flags |= DG_UPD_MOVE;
			    }
			    else
				    skip = true;
            }
		}
		else if(ai->first=="minlen") {
			double len = atof(ai->second.c_str());
			if(len>=.0)
				eg.minLength = len;
			else
				skip = true;
		}
		if(!skip)
			gd<StrAttrs2>(e).put(ai->first,ai->second);
	}
	return ret;
}
template<typename Layout>
void applyStrGraph(Transform *trans,bool useDotDefaults,StrGraph *g,Layout *out, Layout *subg) {
  stringsIn(trans,useDotDefaults,out,gd<StrAttrs>(g),false);
  gd<Name>(out) = gd<Name>(g);
  std::map<DString,typename Layout::Node*> renode;
  for(StrGraph::node_iter ni = g->nodes().begin(); ni!=g->nodes().end(); ++ni) {
    typename Layout::Node *n = out->create_node();
    subg->insert(n);
    renode[gd<Name>(n) = gd<Name>(*ni)] = n;
    StrAttrs &attrs = gd<StrAttrs>(*ni);
	stringsIn<Layout>(trans,n,attrs,false);
  }
  for(StrGraph::graphedge_iter ei = g->edges().begin(); ei!=g->edges().end(); ++ei) {
    typename Layout::Edge *e = out->create_edge(renode[gd<Name>((*ei)->tail)],
				     renode[gd<Name>((*ei)->head)]).first;
    gd<Name>(e) = gd<Name>(*ei);
    subg->insert(e);
    StrAttrs &attrs = gd<StrAttrs>(*ei);
	stringsIn<Layout>(trans,e,attrs,false);
  }
}

} // namespace Dynagraph

#endif // stringsIn_h
