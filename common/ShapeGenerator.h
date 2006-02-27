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

#ifndef ShapeGenerator_h
#define ShapeGenerator_h

#include "reorient.h"

namespace Dynagraph {

template<typename Layout>
struct ShapeGenerator : Server<Layout> {
	void Process(ChangeQueue<Layout> &Q);
	ShapeGenerator(Layout *client,Layout *currentLayout) : Server<Layout>(client,currentLayout) {}
	~ShapeGenerator() {}
};
template<typename Layout>
void ShapeGenerator<Layout>::Process(ChangeQueue<Layout> &Q) {
  Layout *subs[2] = {&Q.insN,&Q.modN};
  for(int i=0; i<2; ++i)
    for(typename Layout::node_iter ni = subs[i]->nodes().begin(); ni !=subs[i]->nodes().end(); ++ni) {
      typename Layout::Node *n = *ni;
      if((i==0 || igd<Update>(n).flags&DG_UPD_POLYDEF) && gd<IfPolyDef>(n).whether) {
		gd<Drawn>(n).clear();
        try {
		    genpoly(gd<PolyDef>(n),gd<Drawn>(n));
        }
        catch(GenPolyXep xep) {
			std::cout << "message \"" << xep.exceptype << '"' << std::endl;
			// bad or incomplete definitions: just leave blank
        }
		NodeGeom &ng = gd<NodeGeom>(n);
		ng.region.shape.Clear();
		if(gd<Drawn>(n).size()) {
			// it would be nice to make this section an update for a DG_UPD_DRAWN
			// flag, so that the user could specify gd<Drawn> instead of this shapegen...
			Line &biggest = gd<Drawn>(n).front(); // first one is biggest
			ng.region.shape.resize(biggest.size());
			for(size_t i = 0; i<biggest.size(); ++i)
				ng.region.shape[i] = reorient(biggest[i],true,gd<Translation>(Q.current).orientation);
			ng.region.shape.degree = biggest.degree;
		}
		ng.region.updateBounds();
		ModifyNode(Q,n,DG_UPD_REGION|DG_UPD_DRAWN);
      }
    }
}

} // namespace Dynagraph

#endif //ShapeGenerator_h
