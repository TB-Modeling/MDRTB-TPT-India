//
//  Stats.h
//  src
//
//  Created by Trinity on 8/4/15.
//  Copyright (c) 2015 Trinity. All rights reserved.
//
#ifndef MDRTB_STATS_H
#define MDRTB_STATS_H

#include <iostream>
#include <vector>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;

/* XMACRO Setup TODO Add Text */

//All default values are 0
#define STATS_COUNTERS \
    XM( nIncDS, "nIncDS" ) \
    XM( nIncDR, "nIncDR" ) \
    XM( nIncDS_fastprog, "nIncDS_fastprog" ) \
    XM( nIncDR_fastprog, "nIncDR_fastprog" ) \
    XM( nIncDS_slowprog, "nIncDS_slowprog" ) \
    XM( nIncDR_slowprog, "nIncDR_slowprog" ) \
    XM( nIncDS_relapse, "nIncDS_relapse" ) \
    XM( nIncDR_relapse, "nIncDR_relapse" ) \
    XM( nIncDR_resist, "nIncDR_resist" ) \
    XM( nIncHH, "nIncHH" ) \
    XM( nIncCOM, "nIncCOM" ) \
    XM( nIncDS_HH, "nIncDS_HH" ) \
    XM( nIncDR_HH, "nIncDR_HH" ) \
    XM( nMortalityDS, "nMortalityDS" ) \
    XM( nMortalityDR, " mortality" ) \
    XM( nNaturalDeaths, "nNaturalDeaths" ) \
    XM( nTransDS, "nTransDS" ) \
    XM( nTransDR, "nTransDR" ) \
    XM( nTransDS_HH, "nTransDS_HH" ) \
    XM( nTransDR_HH, "nTransDR_HH" ) \
    XM( nELDS, " number of EL incidences: people entering ELDS" ) \
    XM( nELDR, "nELDR" ) \
    XM( nTrtDS_DS, " DS treatment received by DS patients" ) \
    XM( nTrtDS_DR, " DS treatment received by DR patients" ) \
    XM( nTrtDR_DR, "nTrtDR_DR" ) \
    XM( nFailedDSTrt_DS, "failing a DSTrt by a DS patient" ) \
    XM( nFailedDSTrt_DR, "nFailedDSTrt_DR" ) \
    XM( nFailedDRTrt_DR, "nFailedDRTrt_DR" ) \
    XM( nVisits, "nVisits" ) \
    XM( nDiag_DS, "nDiag_DS" ) \
    XM( nDiag_DR, "nDiag_DR" ) \
    XM( nDST, " number of drug susceptibility tests performed (at the time of DSTB diagnosis)" ) \
    XM( nPretreatmentLTF, "nPretreatmentLTF" ) \
    XM( nNewPatient, " total number of patients with TB who has no previous history of treatment" ) \
    XM( nRetreatedPatient, "nRetreatedPatient" ) \
    XM( nNewDRTB, "nNewDRTB" ) \
    XM( nRetreatedDRTB, "nRetreatedDRTB" ) \
    XM( nHhct_hhScreened, "nHhct_hhScreened" ) \
    XM( nHhct_hhMembersScreened, "nHhct_hhMembersScreened" ) \
    XM( nHhct_hhMembersLt6, "nHhct_hhMembersLt6" ) \
    XM( nHhct_hhMembersLt16, "nHhct_hhMembersLt16" ) \
    XM( nHhct_DS_atbDiagnosed, "nHhct_DS_atbDiagnosed" ) \
    XM( nHhct_DS_ltbDiagnosed, "nHhct_DS_ltbDiagnosed" ) \
    XM( nHhct_DR_atbDiagnosed, "nHhct_DR_atbDiagnosed" ) \
    XM( nHhct_DR_ltbDiagnosed, "nHhct_DR_ltbDiagnosed" ) \
    XM( nPt_started, "nPt_started" ) \
    XM( nPt_completed, "nPt_completed" ) \
    XM( nIncDS_onPt, "nIncDS_onPt" ) \
    XM( nIncDS_postPt, "nIncDS_postPt" ) \
    XM( nIncDR_onPt, "nIncDR_onPt" ) \
    XM( nIncDR_postPt, "nIncDR_postPt" ) \
    
    
//Vectors: storing tally values that are not necessarily reported each year
#define STATS_VECTORS \
    XM( int, vDurDSTB," duration of disease before death, cure or recovery" ) \
    XM( int, vDurDRTB,"vDurDRTB" ) \
    XM( int, vDurDSTB_trt,"duration of disease until treatment" ) \
    XM( int, vDurDRTB_trt,"vDurDRTB_trt" ) \
    XM( int, vNumVisitsToRecovery,"vNumVisitsToRecovery" ) \
    XM( int, vAgeAtDeath,"vAgeAtDeath" ) \
    XM( double, vHhSize_atDiagADS," # of hh memebers at ADS diagnosis" ) \
    XM( double, vHhPrevSUS_atDiagADS," # of hh memebers SUS to DS/DR at ADS diagnosis" ) \
    XM( double, vHhPrev_ADS_atDiagADS," # of hh memebers with ADS infection at ADS diagnosis" ) \
    XM( double, vHhPrev_ADR_atDiagADS," # of hh memebers with ADR infection at ADS diagnosis" ) \
    XM( double, vHhPrev_LADS_atDiagADS," # of hh memebers with any DS infection (not sus or rec) at ADS diagnosis whose infection originated in the household" ) \
    XM( double, vHhPrev_LADR_atDiagADS,"vHhPrev_LADR_atDiagADS" ) \
    XM( double, vHhPrev_LADS_LADR_atDiagADS,"vHhPrev_LADS_LADR_atDiagADS" ) \
    XM( double, vHhPrev_ADS_LADR_atDiagADS,"vHhPrev_ADS_LADR_atDiagADS" ) \
    XM( double, vHhPrev_ADR_LADS_atDiagADS,"vHhPrev_ADR_LADS_atDiagADS" ) \
    XM( double, vHhPrevADS_hhOriginated_atDiagADS," # of hh memebers with ADS infection, originated at HH, at ADS diagnosis whose infection originated in the household" ) \
    XM( double, vHhPrevLADS_hhOriginated_atDiagADS," # of hh memebers with any DS infection, originated at HH, at DS diagnosis whose infection originated in the household" ) \
    XM( double, vHhSize_atDiagADR," # of hh memebers at ADS diagnosis" ) \
    XM( double, vHhPrevSUS_atDiagADR," # of hh memebers SUS to DS/DR at ADR diagnosis" ) \
    XM( double, vHhPrev_ADS_atDiagADR," # of hh memebers with ADS infection at ADR diagnosis" ) \
    XM( double, vHhPrev_ADR_atDiagADR," # of hh memebers with ADR infection at ADR diagnosis" ) \
    XM( double, vHhPrev_LADS_atDiagADR,"vHhPrev_LADS_atDiagADR" ) \
    XM( double, vHhPrev_LADR_atDiagADR,"vHhPrev_LADR_atDiagADR" ) \
    XM( double, vHhPrev_LADS_LADR_atDiagADR,"vHhPrev_LADS_LADR_atDiagADR" ) \
    XM( double, vHhPrev_ADS_LADR_atDiagADR,"vHhPrev_ADS_LADR_atDiagADR" ) \
    XM( double, vHhPrev_ADR_LADS_atDiagADR,"vHhPrev_ADR_LADS_atDiagADR" ) \
    XM( double, vHhPrevADR_hhOriginated_atDiagADR,"vHhPrevADR_hhOriginated_atDiagADR" ) \
    XM( double, vHhPrevLADR_hhOriginated_atDiagADR,"vHhPrevLADR_hhOriginated_atDiagADR" ) \


class Stats {
private:
    // Boost::serialization and Archive
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar &_TNOW;
        ar &_YNOW;
        ar &_LAST_PERSON_ID;
        ar &_LAST_HH_ID;

#define XM( name, desc ) \
        ar & name;
STATS_COUNTERS
#undef XM

#define XM( type, name, desc ) \
        ar & name;
STATS_VECTORS
#undef XM

    }
    //4 Basic variables

    int _TNOW ; //time counter variable (based on months)  TNOW shared among all classes
    int _YNOW; // year counter
    int _LAST_PERSON_ID;//unique id counter for creating the population (starts from 0 and increases with each new birth over time)
    int _LAST_HH_ID;

    //---------------------------------------------------
    //COUNTERS: Counting specific events
    //---------------------------------------------------
#define XM( name, desc ) \
    int name;
STATS_COUNTERS
#undef XM

    ////Vectors: storing tally values that are not necessarily reported each year
#define XM( type, name, desc ) \
    vector<type> name;
STATS_VECTORS
#undef XM

public:
    //---------------------------------------------------
    //CLASS FUNCTIONS
    //---------------------------------------------------
    Stats(); //Constructor
    
    bool compare(std::shared_ptr<Stats> s) const; //comapres this person with s to make sure all attributes are the same
    bool operator != (const std::shared_ptr<Stats> &s) const {
        bool equivalent = compare(s);
        return !equivalent;
    }
    
    bool operator == (const std::shared_ptr<Stats> &s) const {
        bool equivalent = compare(s);
        return equivalent;
    }

    //---------------------------------------------------
    //HELPER FUNCTIONS
    //---------------------------------------------------
    inline void setYear(const int y) noexcept { _YNOW = y; }
    inline int getYear() const noexcept { return _YNOW; }
    inline void incYear() noexcept { _YNOW++; }
    inline void incYear(int t) noexcept { _YNOW+=t; }

    inline void setTime(const int t) noexcept { _TNOW = t; }
    inline int getTime() const noexcept { return _TNOW; }
    inline void incTime() noexcept { _TNOW++; }

    inline int getPersonID() const noexcept { return _LAST_PERSON_ID; }
    inline void incPersonID() noexcept { _LAST_PERSON_ID++; }

    inline int getHouseholdID() const noexcept { return _LAST_HH_ID; }
    inline void incHouseholdID() noexcept { _LAST_HH_ID++; }
    //---------------------------------------------------
    //FUNCTIONS TO READ/RETURN COUNTERS
    //---------------------------------------------------

#define XM( name, desc ) \
    inline int return_##name() const { return name; } \
    inline void record_##name(const int increment = 1) { name+=increment; } \
    inline void info_##name() const { cout << #name << ": " << desc << endl; }
STATS_COUNTERS
#undef XM

#define XM( type, name, desc ) \
    inline void record_##name(const type d) { name.push_back(d); } \
    inline vector<type> return_##name() const { return name; } \
    inline void info_##name() const { cout << #name << ": " << desc << endl; }
STATS_VECTORS
#undef XM

    //---------------------------------------------------
    //RESET: cleaning the stats
    //---------------------------------------------------
    void resetAnnualOutputs(); //resets all counters
    void resetAllOutputs(); //resets counters and tallys
};
#endif /* defined(MDRTB_STATS_H) */
