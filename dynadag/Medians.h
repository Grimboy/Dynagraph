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

void InvalidateMVal(DDModel::Node *n, UpDown dir);
bool MValExists(DDModel::Node *n, UpDown dir);
double MVal(DDModel::Node *n, UpDown dir);
void SetMVal(DDModel::Node *n, double val, UpDown dir);
void InvalidateAdjMVals(DDModel::Node *n);
