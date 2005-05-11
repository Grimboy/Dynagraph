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


#ifndef StringDict_h
#define StringDict_h

#include "dgxep.h"
#include "useful.h"
#include <string>
#include <algorithm>
#include <iostream>

// adaptation of agraph's refstr

// I know, there's a more modern way to do this....
//#define STRINGDICT_USE_STL
#ifndef STRINGDICT_USE_STL
#include "cdt.h"
struct StringDict {
	StringDict();
	void init();
	const char *enter(const char *val);
	void release(const char *val);
	void ref(const char *val); // MUST have come from here!
private:
	Dict_t *dict;
};
#else
#include <map>
#include <string>
struct StringDict {
	typedef std::map<std::string,int> mapstrs;
	StringDict() { init(); }
	void init() { strs = new mapstrs; }
	const char *enter(const char *val) {
		if(!val)
			return 0;
		if(!strs)
			init();
		mapstrs::iterator mi = strs->insert(mapstrs::value_type(val,0)).first;
		mi->second++;
		return mi->first.c_str();
	}
	void release(const char *val) {
		if(!val)
			return;
		mapstrs::iterator mi = strs->find(val);
		if(mi==strs->end())
			return;
		assert(mi->second>0);
		if(!--mi->second)
			strs->erase(mi);
	}
	void ref(const char *val) {
		if(!val)
			return;
		mapstrs::iterator mi = strs->find(val);
		if(mi==strs->end())
			return;
		assert(mi->second>0);
		++mi->second;
	}

private:
	mapstrs *strs;
};
#endif
// in StringDict.cpp, or define your own if DSTRING_USE_STL
extern StringDict g_stringDict;

struct DString { // imitation of std::string
private:
	const char *val;
public:
	typedef const char *iterator;
	typedef const char *const_iterator;
	typedef char value_type;
	typedef size_t size_type;
	static const size_type npos;

	DString() : val(0) {}
	DString(const char *v) : val(g_stringDict.enter(v)) {}
	DString(const DString &ds) : val(ds.val) {
		g_stringDict.ref(val);
	}
	DString(const std::string &s) : val(g_stringDict.enter(s.c_str())) {}
	~DString() {
		g_stringDict.release(val);
	}
	DString &operator =(const DString &ds) {
		const char *old = val; // do ref first for unlikely s = s
		g_stringDict.ref(val = ds.val);
		g_stringDict.release(old);
		return *this;
	}
    operator std::string() const {
        return val?std::string(val):std::string();
	}
	// these are what make this super-cool: single-word compare!
    // (obviously this is not a typological sort, but no one wants
    // attributes in alphabetical order just to look them up quickly)
	bool operator <(const DString &ds) const {
		return val<ds.val;
	}
	bool operator ==(const DString &ds) const {
		return val==ds.val;
	}
	bool operator !=(const DString &ds) const {
		return val!=ds.val;
	}
	const char *c_str() const {
		return val;
	}
	size_type size() const {
		if(!val)
			return 0;
		return strlen(val);
	}
	size_type length() const {
		return size();
	}
	bool empty() const {
		return length()==0;
	}
	void clear() {
		*this = 0;
	}
	char operator[](size_t pos) const {
		return val[pos];
	}
	const char &operator[](size_t pos) {
		return val[pos];
	}
	iterator begin() {
		return val;
	}
	iterator end() {
		return val+size();
	}
	const_iterator begin() const {
		return val;
	}
	const_iterator end() const {
		return val+size();
	}
	size_type find(char c,size_type pos) const {
		if(pos>=size())
			return npos;
		const_iterator i = std::find(begin()+pos,end(),c);
		if(i==end())
			return npos;
		else
			return i-begin();
	}
	DString substr(size_type pos,size_type len=npos) const {
		DString ret;
		if(pos>=size())
			return ret;
		ret.assign(begin()+pos,len);
		return ret;
	}
	DString &assign(const char *v,size_type len) {
	  if(!v) {
	    return *this = 0;
	  }
		if(len>=strlen(v))
			return *this = v;
		// this does not work because if v is a DString, this changes the dictionary entry itself.
		/*
		char *sneaky = const_cast<char*>(v),
			c = sneaky[len];
		sneaky[len] = 0;
		*this = sneaky;
		sneaky[len] = c;
		*/
		char *copy = new char[len+1];
		strncpy(copy,v,len);
		copy[len] = 0;
		*this = copy;
		delete [] copy;
		return *this;
	}
	int compare(const DString&s) const {
		return strcmp(c_str(),s.c_str());
	}
	int compare(size_type _Pos1,size_type _Num1,const DString& _Str) {
		return strncmp(c_str()+_Pos1,_Str.c_str(),_Num1);
	}
	int compare(size_type _Pos1,size_type _Num1,const DString& _Str,size_type _Off,size_type _Count) {
		int ret = strncmp(c_str()+_Pos1,_Str.c_str()+_Off,std::min(_Num1,_Count));
		if(ret==0 &&_Num1!=_Count) // comparing strings of different sizes
			return int(_Num1)-int(_Count);
		else
			return ret;
	}
	int compare(const value_type* _Ptr) const {
		return strcmp(c_str(),_Ptr);
	}
	int compare(size_type _Pos1,size_type _Num1,const value_type* _Ptr,size_type _Num2 = npos) const {
		int ret = strncmp(c_str()+_Pos1,_Ptr,std::min(_Num1,_Num2));
		if(ret==0 && _Num1!=_Num2)
			return int(_Num1)-int(_Num2);
		else
			return ret;
	}

};
inline std::ostream &operator <<(std::ostream &os,const DString &s) {
  if(s.length())
    os << s.c_str();
  return os;
}
struct DictStringLost : DGException {
	const char *s;
	DictStringLost(const char *s) :
	  DGException("StringDict internal exception: string lost"),
	  s(s)
  {}
};

int ds2int(const DString &s);

#endif // StringDict_h
