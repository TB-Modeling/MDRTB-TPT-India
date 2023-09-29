/*
 *   Stats.cpp
 *   src
 *
 *   Created by Trinity on 8/4/15.
 *   Copyright (c) 2015 Trinity. All rights reserved.
 */
#include "Stats.h"

Stats::Stats() {//constructor
    _TNOW = 1;
    _YNOW=0;
    _LAST_PERSON_ID = 0;
    _LAST_HH_ID=0;

    resetAllOutputs();
}


bool Stats::compare(std::shared_ptr<Stats> s) const {
    if (_TNOW != s->_TNOW ) return false;
    if (_YNOW!= s->_YNOW ) return false;
    if (_LAST_PERSON_ID != s->_LAST_PERSON_ID ) return false;
    if (_LAST_HH_ID != s->_LAST_HH_ID ) return false;

#define XM( name, desc ) \
    if ( name != s->name ) return false;
STATS_COUNTERS
#undef XM

#define XM( type, name, desc ) \
    if ( name != s->name ) return false;
STATS_VECTORS
#undef XM

    return true;
}


void Stats::resetAnnualOutputs(){

#define XM( name, desc ) \
    name = 0;
STATS_COUNTERS
#undef XM
}


void Stats::resetAllOutputs(){
    resetAnnualOutputs();

#define XM( type, name, desc ) \
    name.clear();
STATS_VECTORS
#undef XM

}
