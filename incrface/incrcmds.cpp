/*   Copyright (c) AT&T Corp.  All rights reserved.

This software may only be used by you under license from 
AT&T Corp. ("AT&T").  A copy of AT&T's Source Code Agreement 
is available at AT&T's Internet website having the URL 

http://www.research.att.com/sw/tools/graphviz/license/

If you received this software without first entering into a license 
with AT&T, you have an infringing copy of this software and cannot 
use it without violating AT&T's intellectual property rights. */

#include "common/Dynagraph.h"
#include "DynaView.h"
#include "common/ag2str.h"
//#include "common/bufferGraphStream.h"
#include "incrxep.h"

using namespace std;

static StrAttrs g_currAttrs;
DinoMachine g_dinoMachine;

// this must be filled 
// e.g. in dynagraph/main.cpp and comdg/Engine.cpp
IncrCallbacks *g_incrCallback;

void incr_set_handler(DString name,IncrLangEvents *handler) {
    DinoMachine::Node *n = g_dinoMachine.get_node(name);
    IncrLangEvents *&h = gd<DinoMachNode>(n).handler;
    if(h!=handler) {
        if(h)
            g_incrCallback->incr_cb_destroy_handler(h);
        h = handler;
    }
}
IncrLangEvents *incr_get_handler(DString name) {
    DinoMachine::Node *n = g_dinoMachine.ndict[name];
    if(!n)
        return 0;
    return gd<DinoMachNode>(n).handler;
}
void incr_open_graph(const char *graph)
{
    // look for one already there - incr_ev_open_graph will decide if this is an error
    IncrLangEvents *h = incr_get_handler(graph);

    if(!h) 
        h = g_incrCallback->incr_cb_create_handler(graph,g_currAttrs);
    if(!h)
        return;
    if(!h->incr_ev_open_graph(graph,g_currAttrs))
        delete h;
}

void incr_close_graph(const char *graph)
{
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    if(h->incr_ev_close_graph()) 
        g_dinoMachine.erase(g_dinoMachine.ndict[graph]); // ~DinoMachNode will g_incrCallback->incr_cb_destroy_handler(h);
}

void incr_mod_graph(const char *graph) {
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_mod_graph(g_currAttrs);
}

void incr_lock(const char *graph) {
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_lock();
}

void incr_unlock(const char *graph) {
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_unlock();
}

extern FILE *incr_yyin;
void incr_segue(const char *graph) {
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    //bufferGraphStream fix(incr_yyin);
    //assert(!feof(fix.fin));
    StrGraph *sg = readStrGraph(incr_yyin);
    /*
    assert(!feof(fix.fin));
    char buf[200];
    fgets(buf,200,fix.fin);
    */
    if(!sg) 
        fprintf(stderr,"graph read error\n");
    else
        h->incr_ev_load_strgraph(sg,true,true);
}

void incr_ins_node(const char *graph,const char *id)
{
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_ins_node(id,g_currAttrs,true);
}

void incr_ins_edge(const char *graph,const char *id, const char *tail, const char *head)
{
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_ins_edge(id,tail,head,g_currAttrs);
}

void incr_mod_node(const char *graph,const char *id)
{
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_mod_node(id,g_currAttrs);
}

void incr_mod_edge(const char *graph,const char *id)
{
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_mod_edge(id,g_currAttrs);
}

void incr_del_node(const char *graph,const char *id)
{
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_del_node(id);
}

void incr_del_edge(const char *graph,const char *id)
{
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_del_edge(id);
}
void incr_req_graph(const char *graph) {
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_req_graph();
}
void incr_req_node(const char *graph,const char *id) {
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_req_node(id);
}
void incr_req_edge(const char *graph,const char *id) {
    IncrLangEvents *h = incr_get_handler(graph);
    if(!h) 
		throw IncrGraphNotOpen(graph);
    h->incr_ev_req_edge(id);
}
void incr_ful_graph(const char *graph) {
    StrGraph *sg = readStrGraph(incr_yyin);
    g_incrCallback->incr_cb_fulfil_graph(graph,sg);
    delete sg;
}
void incr_ful_node(const char *graph,const char *id) {
    g_incrCallback->incr_cb_fulfil_node(graph,id,g_currAttrs);
}
void incr_ful_edge(const char *graph,const char *id) {
    g_incrCallback->incr_cb_fulfil_edge(graph,id,g_currAttrs);
}
void incr_message(const char *msg) 
{
    g_incrCallback->incr_cb_message(msg);
}

/* attribute scanning */

void incr_reset_attrs()
{
    g_currAttrs.clear();
}

void incr_append_attr(const char *name, const char *value)
{
    g_currAttrs[name] = value;
}


/* error handler */

static const char *ErrMsg[] = {
    "unknown error",
        "graph/view already open",
        "graph/view not open",
        "name mismatch",
        "syntax error",
        "duplicated ID (node/edge)",
        "not implemented",
        "object doesn't exist",
        (char*)0
} ;

void incr_error(int code, const char *str)
{
    if (!str) str = "";
    char buf[300];
    sprintf(buf,"incr interface: %s %s\n",ErrMsg[code],str);
    throw IncrError(buf);
}

void incr_abort(int code)
{
    incr_error(code,0);
}

//}
