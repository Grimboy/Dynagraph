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
#include "Measurements.h"

using namespace std;

namespace Dynagraph {
namespace DynaDAG {

DynaDAGServer::~DynaDAGServer() {
	for(DynaDAGLayout::graphedge_iter i = current->edges().begin();i!=current->edges().end();++i)
		closeLayoutEdge(*i);
	for(DynaDAGLayout::node_iter j(current->nodes().begin()); j!=current->nodes().end(); ++j)
		closeLayoutNode(*j);
	assert(model.empty());
}
// DynaDAGServices
pair<DDMultiNode*,DDModel::Node*> DynaDAGServer::OpenModelNode(DynaDAGLayout::Node *layoutN) {
	DDMultiNode *m = 0;
	DDModel::Node *mn = model.create_node();
	if(layoutN) { // part of multinode: attach multi to model & view nodes
		assert(layoutN->g==client); // don't store the wrong subnode
		m = DDp(layoutN); // see if view node's already been modeled
		if(!m) {
			(m = new DDMultiNode)->layoutN = layoutN;
			m->node = mn;
			DDp(layoutN) = m;
		}
		DDd(mn).multi = m;
	}
	return make_pair(m,mn);
}
void DynaDAGServer::CloseModelNode(DDModel::Node *n) {
	/* must already be isolated in graph */
	assert(n->ins().size()==0 && n->outs().size()==0);
	xsolver.RemoveNodeConstraints(n);
	config.RemoveNode(n);
	model.erase_node(n);
}
pair<DDPath*,DDModel::Edge*> DynaDAGServer::OpenModelEdge(DDModel::Node *u, DDModel::Node *v, DynaDAGLayout::Edge *layoutE) {
	DDPath *p = 0;
	DDModel::Edge *me = 0;
	if(u&&v)
		me = model.create_edge(u,v).first;
	if(layoutE) { // part of path: attach path to model & view edges
		assert(layoutE->g==client); // don't store the wrong subedge
		p = DDp(layoutE); // see if view edge's already been modeled
		if(!p) {
			(p = new DDPath)->layoutE = layoutE;
			DDp(layoutE) = p;
		}
		if(me)
			DDd(me).path = p;
	}
	return make_pair(p,me);
}
void DynaDAGServer::CloseModelEdge(DDModel::Edge *e) {
	xsolver.RemoveEdgeConstraints(e);
	model.erase_edge(e);
}
void DynaDAGServer::CloseChain(DDChain *chain,bool killEndNodes) {
	if(!chain->first)
		return;
	DDModel::Edge *ei = chain->first;
	DDModel::Node *ni;
	while(1) {
		ni = ei->head;
		DDModel::Node *killn = 0;
		if(killEndNodes || ei!=chain->first)
			killn = ei->tail;
		CloseModelEdge(ei);
		if(killn)
			CloseModelNode(killn);
		if(ei==chain->last)
			break;
		assert(ni->outs().size()==1);
		ei = *ni->outs().begin();
	}
	if(killEndNodes)
		CloseModelNode(ni);
	chain->first = chain->last = 0;
}
Optimizer *DynaDAGServer::GetOptimizer() {
	return optimizer;
}
// private methods
void DynaDAGServer::closeLayoutNode(DynaDAGLayout::Node *n) {
	DDMultiNode *m = DDp(n);
	ranker.RemoveLayoutNodeConstraints(m);
	//xsolver.RemoveLayoutNodeConstraints(m);
	CloseChain(m,true);
	if(m->node)
		CloseModelNode(m->node);
	delete m;
	DDp(n) = 0;
}
void DynaDAGServer::closeLayoutEdge(DynaDAGLayout::Edge *e) {
	DDPath *p = DDp(e);
	report(r_bug,"e %p p %p\n",e,p);
	assert(p);
	ranker.RemovePathConstraints(p);
	if(p->first) {
		InvalidateMVal(p->first->tail,DOWN);
		InvalidateMVal(p->last->head,UP);
	}
	CloseChain(p,false);
	delete p;
	DDp(e) = 0;
}
void DynaDAGServer::executeDeletions(DDChangeQueue &changeQ,DynaDAGLayout &extraI) {
	for(DynaDAGLayout::graphedge_iter j = changeQ.delE.edges().begin(); j!=changeQ.delE.edges().end();) {
		// if part of 2-cycle, enable the other edge
		if(DynaDAGLayout::Edge *e1 = current->find_edge((*j)->head,(*j)->tail))
			if(assign(DDp(e1)->secondOfTwo,false))
				extraI.insert(e1);
		DynaDAGLayout::Edge *e = *j++;
		closeLayoutEdge(e);
	}
	for(DynaDAGLayout::node_iter i = changeQ.delN.nodes().begin(); i!=changeQ.delN.nodes().end(); ++i)
		closeLayoutNode(*i);
}
// pretty aggressive (not ENOUGH)
void DynaDAGServer::findOrdererSubgraph(DDChangeQueue &changeQ,DynaDAGLayout &outN,DynaDAGLayout &outE) {
	// do crossing optimization on all inserted nodes & edges...
	outN = changeQ.insN;
	outE = changeQ.insE;
	// all moved...
	DynaDAGLayout::node_iter ni;
	for(ni = changeQ.modN.nodes().begin(); ni!=changeQ.modN.nodes().end(); ++ni)
		if(igd<Update>(*ni).flags&DG_UPD_MOVE)
			outN.insert(*ni);
	for(DynaDAGLayout::graphedge_iter ei=changeQ.modE.edges().begin(); ei!=changeQ.modE.edges().end(); ++ei)
		if((igd<Update>(*ei).flags&DG_UPD_MOVE) && !userDefinedMove(*ei))
			outE.insert(*ei);
	// and all adjacent (this will add the edges off of a node that has a new or changed edge, but not the other ends of those edges)
	outN |= outE; // nodes adjacent to edges
	for(ni = outN.nodes().begin(); ni!=outN.nodes().end(); ++ni) // edges adjacent to nodes
		for(DynaDAGLayout::nodeedge_iter ei = (*ni)->alledges().begin(); ei!=(*ni)->alledges().end(); ++ei)
			outE.insert(*ei);
	if(reportEnabled(r_dynadag)) {
		loops.Field(r_dynadag,"number of layout nodes",current->nodes().size());
		loops.Field(r_dynadag,"layout nodes for crossopt",outN.nodes().size());
		loops.Field(r_dynadag,"layout edges for crossopt",outE.edges().size());
	}
}
void DynaDAGServer::updateBounds(DDChangeQueue &changeQ) {
  bool got = false;
  double glb=0.0,grb=0.0;  // init for gcc's sake argh
  for(Config::Ranks::iterator ri = config.ranking.begin(); ri!=config.ranking.end(); ++ri)
    if((*ri)->order.size()) {
      DDModel::Node *left = (*ri)->order.front();
      double lb = DDd(left).cur.x - config.LeftExtent(left);
      if(!got || glb > lb)
	glb = lb;
      DDModel::Node *right = (*ri)->order.back();
      double rb = DDd(right).cur.x + config.RightExtent(right);
      if(!got || grb < rb)
	grb = rb;
      got = true;
    }
  Bounds bb;
  if(got) {
    bb.valid = true;
    bb.l = glb;
    bb.t = config.ranking.front()->yAbove(0);
    bb.r = grb;
    bb.b = config.ranking.back()->yBelow(0);
  }
  if(gd<GraphGeom>(current).bounds != bb) {
    gd<GraphGeom>(current).bounds = bb;
    igd<Update>(changeQ.client) |= DG_UPD_BOUNDS;
  }
}
void DynaDAGServer::findChangedNodes(DDChangeQueue &changeQ) {
	// calculate how much nodes moved
	Coord moved(0,0);
	int nmoves=0;
	for(DynaDAGLayout::node_iter ni = current->nodes().begin(); ni!=current->nodes().end(); ++ni) {
		if(!changeQ.delN.find(*ni)) {
			Position pos = DDp(*ni)->pos();
			assert(pos.valid);
			Position &p = gd<NodeGeom>(*ni).pos;
			if(p != pos)
			{
				if(p.valid) {
					moved += (p-pos).Abs();
					++nmoves;
				}
				p = pos;
				ModifyNode(changeQ,*ni,DG_UPD_MOVE);
			}
		}
	}
	loops.Field(r_stability,"layout nodes moved",nmoves);
	loops.Field(r_stability,"node x movement",moved.x);
	loops.Field(r_stability,"node y movement",moved.y);
}
void DynaDAGServer::findFlowSlope(DDMultiNode *mn) {
	if(!gd<NodeGeom>(mn->layoutN).flow) {
		mn->flowSlope = Coord(0,0);
		return;
	}
	Coord avgIn(0,0),avgOut(0,0);
	int nIns=0,nOuts=0;
	for(DDModel::inedge_iter ei = mn->top()->ins().begin(); ei!=mn->top()->ins().end(); ++ei) {
		Coord vec = (DDd((*ei)->head).cur-DDd((*ei)->tail).cur).Norm();
		if(DDd(*ei).path->direction==DDPath::forward)
			++nIns, avgIn += vec;
		else
			++nOuts, avgOut -= vec;
	}
	for(DDModel::outedge_iter ei = mn->bottom()->outs().begin(); ei!=mn->bottom()->outs().end(); ++ei) {
		Coord vec = (DDd((*ei)->head).cur-DDd((*ei)->tail).cur).Norm();
		if(DDd(*ei).path->direction==DDPath::forward)
			++nOuts, avgOut += vec;
		else
			++nIns, avgIn -= vec;
	}
	// special case flat edges (they don't have model edges)
	for(DynaDAGLayout::inedge_iter ei = mn->layoutN->ins().begin(); ei!=mn->layoutN->ins().end(); ++ei)
		if(DDp(*ei)->direction==DDPath::flat)
			++nIns, avgIn += (DDp((*ei)->head)->pos() - DDp((*ei)->tail)->pos()).Norm();
	for(DynaDAGLayout::outedge_iter ei = mn->layoutN->outs().begin(); ei!=mn->layoutN->outs().end(); ++ei)
		if(DDp(*ei)->direction==DDPath::flat)
			++nOuts, avgOut += (DDp((*ei)->head)->pos() - DDp((*ei)->tail)->pos()).Norm();
	if(nIns)
		avgIn /= nIns;
	if(nOuts)
		avgOut /= nOuts;
	mn->flowSlope = (avgIn+avgOut)/2*gd<NodeGeom>(mn->layoutN).flow;
}
void DynaDAGServer::findFlowSlopes(DDChangeQueue &changeQ) {
	for(DynaDAGLayout::graphedge_iter ei = changeQ.insE.edges().begin(); ei!=changeQ.insE.edges().end(); ++ei) {
		findFlowSlope(DDp((*ei)->tail));
		findFlowSlope(DDp((*ei)->head));
	}
	for(DynaDAGLayout::graphedge_iter ei = changeQ.modE.edges().begin(); ei!=changeQ.modE.edges().end(); ++ei) {
		findFlowSlope(DDp((*ei)->tail));
		findFlowSlope(DDp((*ei)->head));
	}
}
bool DynaDAGServer::edgeNeedsRedraw(DDPath *path,DDChangeQueue &changeQ) {
	if(path->unclippedPath.Empty()) // new edge
		return true;
	DynaDAGLayout::Node *tailMod = changeQ.modN.find(path->layoutE->tail),
		*headMod = changeQ.modN.find(path->layoutE->head);
	// check for endpoint resize or move
	if(tailMod && igd<Update>(tailMod).flags & (DG_UPD_REGION|DG_UPD_MOVE) ||
		headMod && igd<Update>(headMod).flags & (DG_UPD_REGION|DG_UPD_MOVE))
		return true;
	if(!path->first) // flat or self edge
		return false;
	double sep = gd<GraphGeom>(client).separation.x;
	for(DDPath::node_iter ni = path->nBegin(); ni!=path->nEnd(); ++ni) {
		DDModel::Node *n = *ni;
		if(!DDd(n).actualXValid)
			return true;
		double x = DDd(n).actualX;
		if(DDModel::Node *left = config.Left(n)) {
			if(DDd(left).amEdgePart()) {
				if(DDd(left).actualXValid && DDd(left).actualX + sep > x)
					return true;
			}
			else {
				if(DDd(left).cur.x + config.RightExtent(left) + sep > x)
					return true;
			}
		}
		if(DDModel::Node *right = config.Right(n)) {
			if(DDd(right).amEdgePart()) {
				if(DDd(right).actualXValid && DDd(right).actualX - sep < x)
					return true;
			}
			else {
				if(DDd(right).cur.x - config.LeftExtent(right) - sep < x)
					return true;
			}
		}
	}
	return false;
}
void DynaDAGServer::sketchEdge(DDPath *path) {
	// draw an edge just based on vnodes
	EdgeGeom &eg = gd<EdgeGeom>(path->layoutE);
	path->unclippedPath.degree = 1;
	DynaDAGLayout::Node *head = path->layoutE->head,
		*tail=path->layoutE->tail;
	assert(head!=tail); // self-edges handled in redrawEdges
	// if a backedge (head is lower rank than tail), path->first->tail is head
	// so we have to clip accordingly and then reverse the result (for arrowheads etc.)
	bool reversed = DDd(DDp(head)->top()).rank<DDd(DDp(tail)->bottom()).rank;
	if(reversed)
		swap(head,tail);
	if(!path->first) {
		// there are three possible reasons why there's no path.
		// redrawEdges handles self-edges and second edges of 2-cycles
		// so we assume it's
		path->unclippedPath.push_back(DDp(tail)->pos()+eg.tailPort.pos);
		path->unclippedPath.push_back(DDp(head)->pos()+eg.headPort.pos);
	}
	else {
		path->unclippedPath.push_back(DDd(path->first->tail).cur+eg.tailPort.pos);
		for(DDPath::node_iter ni = path->nBegin(); ni!=path->nEnd(); ++ni)
			path->unclippedPath.push_back(DDd(*ni).cur);
		path->unclippedPath.push_back(DDd(path->last->head).cur+eg.headPort.pos);
	}
	bool clipFirst = eg.headClipped,
		clipLast = eg.tailClipped;
	if(reversed)
		swap(clipFirst,clipLast);
	eg.pos.ClipEndpoints(path->unclippedPath,
		gd<NodeGeom>(tail).pos,clipFirst?&gd<NodeGeom>(tail).region:0,
		gd<NodeGeom>(head).pos,clipLast?&gd<NodeGeom>(head).region:0);
	if(reversed)
		reverse(eg.pos.begin(),eg.pos.end());
}
void DynaDAGServer::redrawEdges(DDChangeQueue &changeQ,bool force) {
	//ObstacleAvoiderSpliner<DynaDAGLayout> obav(current);
	DynaDAGLayout::graphedge_iter ei;
	for(ei = current->edges().begin(); ei!=current->edges().end(); ++ei) {
		Line before = gd<EdgeGeom>(*ei).pos;
		if(force || !changeQ.delE.find(*ei) && (gd<EdgeGeom>(*ei).pos.Empty() || edgeNeedsRedraw(DDp(*ei),changeQ))) {
			DDp(*ei)->unclippedPath.Clear();
		}
		if((*ei)->tail==(*ei)->head) { // self-edge
			Line &unclipped = DDp(*ei)->unclippedPath;
			Coord sep = gd<GraphGeom>(client).separation;
			unclipped.degree = 3;
			DDModel::Node *tl = DDp((*ei)->tail)->bottom();
			Coord tailpt = gd<EdgeGeom>(*ei).tailPort.pos + DDd(tl).multi->pos();
			Coord right = Coord(tailpt.x+config.RightExtent(tl),tailpt.y),
				left = right - Coord(1.5*sep.x,0.0);
#ifndef DOWN_GREATER
			Coord dy(0.0,-sep.y/2.0);
#else
			Coord dy(0.0,sep.y/2.0);
#endif
			unclipped.push_back(left);
			unclipped.push_back(left+dy);
			unclipped.push_back(right+dy);
			unclipped.push_back(right);
			unclipped.push_back(right-dy);
			unclipped.push_back(left-dy);
			unclipped.push_back(left);
		}
		if(DDp(*ei)->secondOfTwo)
			continue; // handled below
		if(!changeQ.delE.find(*ei) && DDp(*ei)->unclippedPath.Empty()) {
			if(gd<GraphGeom>(current).splineLevel==DG_SPLINELEVEL_VNODE ||
					!spliner.MakeEdgeSpline(DDp(*ei),gd<GraphGeom>(current).splineLevel /*,obav */ ))
				sketchEdge(DDp(*ei));
		}
		if(before!=gd<EdgeGeom>(*ei).pos)
			ModifyEdge(changeQ,*ei,DG_UPD_MOVE);
	}
	for(ei = current->edges().begin(); ei!=current->edges().end(); ++ei)
		if(DDp(*ei)->secondOfTwo) {
			Line before = gd<EdgeGeom>(*ei).pos;
			Line &otherSide = gd<EdgeGeom>(current->find_edge((*ei)->head,(*ei)->tail)).pos;
			gd<EdgeGeom>(*ei).pos.assign(otherSide.rbegin(),otherSide.rend());
			gd<EdgeGeom>(*ei).pos.degree = otherSide.degree;
			if(before!=gd<EdgeGeom>(*ei).pos)
				ModifyEdge(changeQ,*ei,DG_UPD_MOVE);
		}
}
void DynaDAGServer::cleanUp() { // dd_postprocess
	for(DDModel::node_iter ni = model.nodes().begin(); ni!=model.nodes().end(); ++ni) {
		DDNode &ddn = DDd(*ni);
		ddn.prev = ddn.cur;
		// ddn.orderFixed = true;
	}
	for(DynaDAGLayout::node_iter vni = current->nodes().begin(); vni!=current->nodes().end(); ++vni) {
		DDMultiNode *n = DDp(*vni);
		if(!n)
			continue; // deleted
		//n->coordFixed = true;
		n->oldTopRank = DDd(n->top()).rank;
		n->oldBottomRank = DDd(n->bottom()).rank;
	}
}
void DynaDAGServer::dumpModel() {
	if(!reportEnabled(r_modelDump))
		return;
	report(r_modelDump,"digraph dynagraphModel {\n");
	for(DDModel::node_iter ni = model.nodes().begin(); ni!=model.nodes().end(); ++ni)
		report(r_modelDump,"\t\"%p\" [label=\"%p\\n%p\"];\n",*ni,*ni,DDd(*ni).multi);
	for(DDModel::graphedge_iter ei = model.edges().begin(); ei!=model.edges().end(); ++ei)
		report(r_modelDump,"\t\"%p\"->\"%p\" [label=\"%p\\n%p\"];\n",(*ei)->tail,(*ei)->head,*ei,DDd(*ei).path);
	report(r_modelDump,"}\n");
}
void DynaDAGServer::Process(DDChangeQueue &changeQ) {
	loops.Field(r_dynadag,"nodes inserted - input",changeQ.insN.nodes().size());
	loops.Field(r_dynadag,"edges inserted - input",changeQ.insE.edges().size());
	loops.Field(r_dynadag,"nodes modified - input",changeQ.modN.nodes().size());
	loops.Field(r_dynadag,"edges modified - input",changeQ.modE.edges().size());
	loops.Field(r_dynadag,"nodes deleted - input",changeQ.delN.nodes().size());
	loops.Field(r_dynadag,"edges deleted - input",changeQ.delE.nodes().size());
	if(changeQ.Empty())
		return;

	// erase model objects for everything that's getting deleted
	DynaDAGLayout extraI(client);
	executeDeletions(changeQ,extraI);
	// add the former secondOfTwos to insE, temporarily
	changeQ.insE |= extraI;
	timer.LoopPoint(r_timing,"preliminary");

	// find ranks for nodes
	ranker.Rerank(changeQ);
	timer.LoopPoint(r_timing,"re-rank nodes");

	// figure out subgraph for crossing optimization
	DynaDAGLayout crossN(config.current),crossE(config.current);
	//findOrdererSubgraph(changeQ,crossN,crossE);
	//optimizer = optChooser.choose(crossN.nodes().size()); // should prob. be no. of nodes in corresponding model subgraph

	// synch model graph with changes
	config.Update(changeQ);
	loops.Field(r_dynadag,"model nodes",model.nodes().size());
	timer.LoopPoint(r_timing,"update model graph");

	// crossing optimization
	optimizer->Reorder(*config.current,*config.current);//crossN,crossE);
	timer.LoopPoint(r_timing,"crossing optimization");

	// find X coords
	xsolver.Place(changeQ);

	// (with ConseqRanks, find rank Ys from node heights.)  find node Y coords from rank Ys.
	config.SetYs();
	timer.LoopPoint(r_timing,"optimize x coordinates");

	// calculate bounding rectangle
	updateBounds(changeQ);

	// find node & edge moves
	findChangedNodes(changeQ);
	
	findFlowSlopes(changeQ);

	redrawEdges(changeQ,igd<Update>(changeQ.client).flags&DG_UPD_EDGESTYLE);
	timer.LoopPoint(r_timing,"draw splines");

	// restore the edges that were re-inserted because secondOfTwo was reset
	for(DynaDAGLayout::graphedge_iter ei = extraI.edges().begin(); ei!=extraI.edges().end(); ++ei) {
		assert(changeQ.insE.erase(*ei));
		ModifyEdge(changeQ,*ei,DG_UPD_MOVE);
	}


	// reset flags
	cleanUp();

	dumpModel();

	loops.Field(r_dynadag,"nodes inserted - output",changeQ.insN.nodes().size());
	loops.Field(r_dynadag,"edges inserted - output",changeQ.insE.edges().size());
	loops.Field(r_dynadag,"nodes modified - output",changeQ.modN.nodes().size());
	loops.Field(r_dynadag,"edges modified - output",changeQ.modE.edges().size());
	loops.Field(r_dynadag,"nodes deleted - output",changeQ.delN.nodes().size());
	loops.Field(r_dynadag,"edges deleted - output",changeQ.delE.nodes().size());
	if(reportEnabled(r_readability)) {
		Crossings cc = calculateCrossings(config);
		loops.Field(r_readability,"edge-edge crossings",cc.edgeEdgeCross);
		loops.Field(r_readability,"edge-node crossings",cc.nodeEdgeCross);
		loops.Field(r_readability,"node-node crossings",cc.nodeNodeCross);

		pair<int,Coord> elen = calculateTotalEdgeLength(config);
		Coord avg = elen.second/elen.first;
		loops.Field(r_readability,"average edge x-length",avg.x);
		loops.Field(r_readability,"average edge y-length",avg.y);
	}
}

} // namespace DynaDAG
} // namespace Dynagraph
