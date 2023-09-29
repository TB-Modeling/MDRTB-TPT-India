//
//  Person.h
//  src
//
//  Created by Trinity on 8/4/15.
//  Copyright (c) 2015 Trinity. All rights reserved.
//
#ifndef __MDRTB__PERSON_H
#define __MDRTB__PERSON_H

#include "Household.h"
#include "Stats.h"
#include "Params.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <random>
#include <memory>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

class Household;
using namespace std;

/* XMACRO Pattern: TODO: Add Text */

#define PERSON_VARS_SIMPLE \
    XM(int, _death_time, -1,"time of death") \
    XM(int, _death_route, -1,"0 natural mortality 1:TB death") \
    XM(int, _death_dsState, -1,"DS State at death") \
    XM(int, _death_drState, -1,"DR State at death") \
    XM(int, _DSstate, DS_SUS,"DSTB state (default=SUS)") \
    XM(int, _DRstate, DR_SUS,"DRTB state") \
    XM(int, _lastDSstate,DS_SUS ,"previous DSTB state (default=-1)") \
    XM(int, _lastDRstate,DR_SUS ,"previous DRTB state") \
    XM(int, _nTransRouteDS, -1,"Different transmission type codes: -2 initial infection; -1 no infection (default), 0 household, 1 community") \
    XM(int, _nTransRouteDR, -1,"Different transmission type codes: -2 initial infection; -1 no infection (default), 0 household, 1 community") \
    XM(int, _tTransDS, -2," time of most recent transmission") \
    XM(int, _tTransDR, -2," time of most recent transmission") \
    XM(int, _nReinfToActiveDisease, 0,"number of reinfections from last disease episode to now (will reset at development of active disease)") \
    XM(int, _tActiveInfection, -1,"time of active infection development (for those cases who fail the treatment, this differs with the state time)") \
    XM(int, _tSeekCare, -1,"time of seeking care") \
    XM(int, _tDiagnosis_DS, -1,"time of DS diagnosis") \
    XM(int, _tDiagnosis_DR, -1,"time of DR diagnosis") \
    XM(int, _nVisits, 0,"number of visit to TB care") \
    XM(int, _nTrtFailure_recent, 0,"number of recent TB trt failure (will reset at development of active disease)") \
    XM(int,  _tDS_EL, -1,"time of entry to early latency for DSTB") \
    XM(int,  _tDS_LL, -1,"time of entry to late latency for DSTB ") \
    XM(int,  _tDS_A, -1,"time of entry to active DSTB;") \
    XM(int,  _tDS_REC, -1,"time of entry to recovery DSTB") \
    XM(int,  _tDR_EL, -1,"time of entry to early latency for DSTB") \
    XM(int,  _tDR_LL, -1,"time of entry to late latency for DSTB;") \
    XM(int,  _tDR_A, -1,"time of entry to active DSTB;") \
    XM(int,  _tDR_REC, -1,"time of entry to recovery DSTB") \
    XM(int, _tDS_TRDS, -1,"t of starting Treatment for DS") \
    XM(int, _tDR_TRDS, -1,"t of starting Treatment for DS") \
    XM(int, _tDR_TRDR, -1,"t of starting Treatment for DR") \
    XM(int, _tDS_TRDR, -1,"t of starting Treatment for DR") \
    XM(int, _tResistance, -1,"t of developing resistance") \
    XM(int, _tLTFU, -1,"time lost to follow-up") \
    XM(int, _tProgressToLLDS_ELDS, -1,"progression to late latency from early latency") \
    XM(int, _tProgressToADS_ELDS, -1,"fast prog") \
    XM(int, _tProgressToADS_LLDS, -1,"slow prog") \
    XM(int, _tProgressToADS_RECDS, -1,"relapse") \
    XM(int, _tProgressToADS_TRDS, -1,"trt failure") \
    XM(int, _tProgressToRECDS_ADS, -1,"spont cure") \
    XM(int, _tProgressToLLDR_ELDR, -1,"prog") \
    XM(int, _tProgressToADR_ELDR, -1,"fast prog") \
    XM(int, _tProgressToADR_LLDR, -1,"slow prog") \
    XM(int, _tProgressToADR_RECDR, -1,"relapse") \
    XM(int, _tProgressToRECDR_ADR, -1,"spont cure") \
    XM(int, _tProgressToADR_TRDS, -1,"resistance or failure") \
    XM(int, _tProgressToRECDS_TRDS, -1,"recovery") \
    XM(int, _tProgressToADR_TRDR, -1,"trt failure") \
    XM(int, _tProgressToRECDR_TRDR, -1,"recovery") \
    XM(int, _tProgressToRECDS_TRDR, -1,"recovery") \
    XM(int, _tProgressToRECDR_TRDS, -1,"recovery due dstb") \
    XM(bool, _bTreated_ever, false,"true if has received any TB treatment throught lifetime") \
    XM(bool, _bIsGoingToFailTrt, false,"true if chosen to fail the TB treatment") \
    XM(bool, _markedDead, false,"true if person is designated to die") \
    XM(bool, _bMarkedForTrans_DS, false,"true if person is chosen for DSTB transmission") \
    XM(bool, _bMarkedForTrans_DR, false,"true if person is chosen for DRTB transmission") \
    XM(double, _immDS, 0,"immunity toward DSTB transmission (0 to 1); 0 for SUS; 1 for ADS and ADR;") \
    XM(double, _immDR, 0,"immunity toward DRTB transmission") \
    XM(bool, _hhct_indexCase, false,"true if this person (index case) is chosed for HHCT") \
    XM(bool, _hhct_hhMember, false,"true if my hh is chosen for hhct (for hh members of the index case)") \
    XM(int, _hhct_indexCase_timeMarked, -1,"?") \
    XM(int, _hhct_hhMember_timeMarked, -1,"?") \
    XM(int, _hhct_time, -1,"time of screening through HHCT") \
    XM(int, _hhct_year, -1,"year of screening through HHCT") \
    XM(int, _hhct_age, -1,"age at hhct") \
    XM(int, _hhct_dsState, -1,"ds state at hhct") \
    XM(int, _hhct_drState, -1,"dr state at hhct") \
    XM(bool, _hhct_diagnosedWithADS, false,"true if diagnosed with ADS at hhct") \
    XM(bool, _hhct_diagnosedWithADR, false,"true if diagnosed with ADR at hhct") \
    XM(bool, _hhct_diagnosedWithLRDS, false,"true if diagnosed with LRDS at hhct") \
    XM(bool, _hhct_diagnosedWithLRDR, false,"true if diagnosed with LRDR at hhct") \
    XM(int, _hhct_dsTransTime, -1,"hhct ds transmission time") \
    XM(int, _hhct_drTransTime, -1,"hhct dr transmission time") \
    XM(int, _hhct_dsTransRoute, -1,"hhct ds transmission route") \
    XM(int, _hhct_drTransRoute, -1,"hhct dr transmission route") \
    XM(int, _hhct_hhID, -1,"hhID at hhct") \
    XM(int, _hhct_hhSize, -1,"hh size at hhct") \
    XM(int, _hhct_nkidsLt6InHH, -1,"number of kids in the hh at contact tracing") \
    XM(int, _hhct_nkidsLt16InHH, -1,"number of kids less than 16 years old in HH") \
    XM(int, _hhct_hhPrevSUS_DSDR, -1,"?") \
    XM(int, _hhct_hhPrevSUS_DS, -1,"?") \
    XM(int, _hhct_hhPrevSUS_DR, -1,"?") \
    XM(int, _hhct_hhPrevADS , -1,"prevalence of active DS at hhct among hh members (excludig the index case)") \
    XM(int, _hhct_hhPrevADR , -1,"prevalence of active DR at hhct among hh members (excludig the index case)") \
    XM(int, _hhct_hhPrevLDS, -1,"prev of latent DS among hh members") \
    XM(int, _hhct_hhPrevLDR, -1,"prev of latent DR among hh members") \
    XM(int, _pt_regimen, -1,"type of PT regimen -1:default (no pt); 1: isonizid;  2:delamanid") \
    XM(int, _pt_start_time, -1,"start date of PT") \
    XM(int, _pt_start_year, -1,"start year of PT") \
    XM(int, _pt_end_time, -1,"completion date of PT") \
    XM(int, _pt_successCode_latent, -1,"0=unsuccessful among LTB cases; 1=successful among LTB cases;  2= unsuccessful among ATB cases") \
    XM(int, _pt_successCode_rec, -1,"0=unsuccessful among LTB cases; 1=successful among LTB cases;  2= unsuccessful among ATB cases") \
    XM(bool, _bIsOnPt, false,"1 if currently on PT") \
    XM(bool, _bHasBeenOnPt, false,"1 if ever received PT") \
    XM(int, _postPt_first_dsInc_time, -1,"first incidence of DSTB") \
    XM(int, _postPt_first_dsInc_lastDsState, -1,"1: fastprog; 2:reactivation; 3:relapse") \
    XM(int, _postPt_first_dsInc_lastDrState, -1,"1: fastprog; 2:reactivation; 3:relapse") \
    XM(int, _postPt_first_dsInc_transRoute, -1,"0 hh; 1 com") \
    XM(int, _postPt_first_dsInc_transTime, -1,"Post pt first DS incidence transmission time") \
    XM(int, _postPt_first_drInc_time, -1,"first incidence of DRTB") \
    XM(int, _postPt_first_drInc_lastDrState, -1,"1: fastprog; 2:reactivation; 3:relapse") \
    XM(int, _postPt_first_drInc_lastDsState, -1,"1: fastprog; 2:reactivation; 3:relapse") \
    XM(int, _postPt_first_drInc_transRoute, -1,"0 hh; 1 com") \
    XM(int, _postPt_first_drInc_transTime, -1,"Post pt first DS incidence transmission time") \

#define PERSON_MODPARAM \
    XM(durToMaxLevel," months to reach max value for some functions (TB mortality, infectiousness, seeking care)") \
    XM(probMaxSeekCare," maximum prob of seeking care each month") \
    XM(probPretreatmentLTF_DSTB," prob of ltf after diagnosis and before treatment initiation DS") \
    XM(probPretreatmentLTF_DRTB,"prob of ltf after diagnosis and before treatment initiation DS") \
    XM(durTrt_DS,"Duration of DS Treatment") \
    XM(durTrt_DR,"Duration of DR Treatment") \
    XM(probDRTrtFailure,"Probability of DR Treatement failure") \
    XM(probDSTrtFailure,"Probability of DR Treatement failure") \
    XM(probRelapse_DS,"Probability of DS relapse") \
    XM(probRelapse_DR,"Probability of DS relapse") \
    XM(nMaxTimeToRelapse,"Max time to relapse") \
    XM(coefTrans," coef of TB transmission for all contacts/strains (range=0 to 1)") \
    XM(coefMaxInfect_DS," coef of maximum infectiousness for DSTB (range=0 to 1)") \
    XM(coefMaxInfect_DR_RelTo_DS,"  max infectiousness of DRTB relative to DSTB") \
    XM(coefInfect_trtFailure," coef of infectioussness for those receiving but failing treatment") \
    XM(coefInfec_0to10," coef of infectiousness for kids") \
    XM(coefFastProg," coef for prob of fast progression") \
    XM(probFastProgAge_0To2," coef for prob of fast progression as a function of age") \
    XM(probFastProgAge_2To10," Probability of fast progress between age of 2-10") \
    XM(probFastProgAge_10To15,"Probability of fast progress between age of 10-15") \
    XM(probFastProgAge_adult,"Probability of fast progress for adults ") \
    XM(probSlowProg," Probability of slow progress") \
    XM(probResistance," Probability of TB resistance") \
    XM(probMaxMortalityTB," Probability of Max Mortality of TB") \
    XM(coefImmunityLatent," Latent Immunity") \
    XM(probSpontResolution," Probability of spontaneous resolution") \
    XM(probSuccessDStrt_DRTB,"prob of succes for DSTrt to cure DRTB") \
    XM(hhct_probADRcase_followup," Probablility active DR case follow-up HHCT") \
    XM(hhct_testSens_atb," HHCT test sensitivity for active tb") \
    XM(hhct_testSens_ltb,"HHCT test sensitivity for latent tb") \
    XM(hhct_ptCoverage_0To5," HHCT Preventative Therapy Coverage age 0-5") \
    XM(hhct_ptCoverage_0To15,"HHCT Preventative Therapy Coverage age 0-15") \
    XM(pt_regimen," PT Regiment") \
    XM(pt_probSuccess_latent," ?") \
    XM(pt_probSuccess_rec," ?") \
    XM(pt_duration," Duration of Preventative Therapy") \
    XM(pt_protAgainstReinfection," Protection against reinfection after PT") \

class Person {
private:
    friend class boost::serialization::access;
    template<typename Archive>
    void serialize(Archive &ar, const unsigned int version) {
        //-------------------------------------------------------------------------------
        // Members
        //-------------------------------------------------------------------------------
        //Pointers
        ar & _mp;
        ar & _stats;
        ar & _hh;
        //Integers?
        ar & _id;      //unique identifier to distinguish people
        ar & _tBorn; //time of birth
        ar & _age;     //age in years
        ar & _gender; //gender (0 male, 1 female)

#define XM(type, name, default_value, desc) \
    ar & name;
PERSON_VARS_SIMPLE
#undef XM

#define XM(name, desc) \
    ar & __##name;
PERSON_MODPARAM
#undef XM
        
    }
    
    //-------------------------------------------------------------------------------
    // Members
    //-------------------------------------------------------------------------------
    std::shared_ptr<Stats> _stats;
    std::shared_ptr<Household> _hh;
    std::shared_ptr<Params> _mp;
    ////-------------------------------------------------------------------------------
    //// LOCAL VARIABLES
    ////-------------------------------------------------------------------------------
    int _id;      //unique identifier to distinguish people
    int _tBorn; //time of birth
    int _age;     //age in years
    int _gender; //gender (0 male, 1 female)

#define XM(type, name, default_value, desc) \
    type name;
PERSON_VARS_SIMPLE
#undef XM

#define XM(name, desc) \
    double __##name;
PERSON_MODPARAM
#undef XM

public:
    // CLASS FUNCTIONS-------------------------------------------------------------------------------
    Person();
    Person(mt19937 &rng, std::shared_ptr<Params> mp, const std::shared_ptr<Stats> &s, const std::shared_ptr<Household> &hh, bool bNewBorn);
    ~Person();
    //Serialization:
    bool compare(std::shared_ptr<Person> q); //comapres this person with q to make sure all attributes are the same
    bool operator != (std::shared_ptr<Person> &q) {
        return !compare(q);
    }
    
    //PARAM READ-------------------------------------------------------------------------------
    void initAllVariables(mt19937 &rng) ;//initializes local variables
    void readAllParams(std::shared_ptr<Params> mp);//reads and initiates all paramters from the mp file
    void readSelectedParams(std::shared_ptr<Params> new_mp);//copy selected parameters from new mp file
    
    //HELPER FUNCTIONS-------------------------------------------------------------------------------
    inline int TNOW() const {return _stats->getTime();}//returns the current time (in months)
    inline const std::shared_ptr<Stats> getstat() const {return _stats;}
    inline void setStats(const std::shared_ptr<Stats> &s){ _stats=s;}
    inline const std::shared_ptr<Household> getHH() const {return _hh;}
    inline bool hasHH() const { return _hh != nullptr; }
    inline void setHH(const std::shared_ptr<Household> &hh){_hh=hh;}
    inline void clearHH() { _hh.reset();}
    inline int getId() const {return _id;}
    inline void setId(const int id){ _id=id;}
    inline int getTborn() const {return _tBorn;}
    inline bool isMarkedDead() const {return _markedDead;}
    inline void setMarkedDead(const bool b){_markedDead=b;}
    inline bool isMarkedTran_DS() const {return _bMarkedForTrans_DS;}
    inline bool isMarkedTran_DR() const {return _bMarkedForTrans_DR;}
    inline void setMarkedTran_DS(const bool b) { _bMarkedForTrans_DS=b;}
    inline void setMarkedTran_DR(const bool b){ _bMarkedForTrans_DR=b;}
    inline bool isTreated_ever() const {return _bTreated_ever;}
    inline int getAge() const {return _age;};
    inline void addAge(){_age++;}
    inline void setAge(const int age){_age=age;}
    inline int getGender() const {return _gender;};
    inline void setGender(const int g){ _gender=g;}
    inline int getDSstate() const {return _DSstate;}
    inline int getDRstate() const {return _DRstate;}
    //State timing accessors
    inline int getTimeEnteringDS_A() const {return _tDS_A; }
    inline int getTimeEnteringDR_A() const {return _tDR_A; }
    inline int getTimeEnterDS_EL() const { return _tDS_EL; }
    inline int getTimeEnterDR_EL() const { return _tDR_EL; }
    
    //Functions that allow for recalculation of slow progress
    inline int getProgressToADS_LLDS() const { return _tProgressToADS_LLDS; }
    inline int getProgressToADR_LLDR() const { return _tProgressToADR_LLDR; }
    inline void setProgressToADS_LLDS(int val) { _tProgressToADS_LLDS=val; }
    inline void setProgressToADR_LLDR(int val) { _tProgressToADR_LLDR=val; }

    //
    inline bool isFailingTreatment(){return _bIsGoingToFailTrt;}
    //MODELING CONTACTS/TRANSMISSION-------------------------------------------------------------------------------
    void modelHHcontacts(mt19937 &rng, vector<std::shared_ptr<Person>>&, const double infectiousness);//returns vectors of newly infected cases with DS and/or DR
    bool modelCOMcontact(mt19937 &rng, const std::shared_ptr<Person> q, const double infectiousness);//returns true if infected
    void setTransRouteDS(const int i){_nTransRouteDS=i;}     int getTransRouteDS() const {return _nTransRouteDS;}
    void setTransRouteDR(const int i){_nTransRouteDR=i;}     int getTransRouteDR() const {return _nTransRouteDR;}
    inline void addnReinf(){_nReinfToActiveDisease++;}
    void settTransDS(const int i){_tTransDS=i;}     int gettTransDS() const { return _tTransDS;}
    void settTransDR(const int i){_tTransDR=i;}     int gettTransDR() const { return _tTransDR;}
    inline int getContactAgeGroup() const { return (_age - 1 ) / 5; }
    
    //MODELING TB DYNAMICS-------------------------------------------------------------------------------
    //Demographics
    bool willDieNaturally(mt19937 &rng,const vector<double> vMort);//retunrs true if person is dying from natural mortality causes or old age >85
    //Natural history
    int returnTimeToFastProg(mt19937 &rng) const;
    int returnTimeToSlowProg(mt19937 &rng) const;
    double returnMortalityTB(mt19937 &rng) const;
    //Infectiousness
    double returnInfectiousness(mt19937 &rng) const;//computes infectiousnessa t all times
    double computeInfectiousness_DR() const;//linear function for infectiousness
    double computeInfectiousness_DS() const;//linear function for infectiousness
    //Immunity
    double returnImmunity(bool forDRTB) const;
    //Seeking care
    int getTimeToSeekCare(mt19937 &rng) const; //time to seek care prior to any treatment
    double returnProbDST() const;//returns the prob of receiving DST in a given year
    bool modelCareVisits(mt19937 &rng);
    //Getting infected
    void getInfectedDS(mt19937 &rng);
    void getInfectedDR(mt19937 &rng);
    void getInfectedDS(mt19937 &rng,const int state);//usued to model initial infection
    void getInfectedDR(mt19937 &rng,const int state);
    
    //ENTER DISEASE STATE-------------------------------------------------------------------------------
    void enterELDS(mt19937 &rng);
    void enterELDR(mt19937 &rng);
    void enterLLDS(mt19937 &rng);
    void enterLLDR(mt19937 &rng);
    void enterADS(mt19937 &rng, const bool dsfailure);
    void enterADR(mt19937 &rng, const bool drfailure);
    void enterRECDS(mt19937 &rng, const bool postPT);
    void enterRECDR(mt19937 &rng, const bool postPT);
    void enterTRDS_DS(mt19937 &rng);//DS patient entering DS treatment
    void enterTRDS_DR(mt19937 &rng);//DR patient entering DS treatment by mistake : will fail    void enterTRDR_DS(mt19937 &rng);//DS patient entering DR treatment by mistake : effective
    void enterTRDR_DR(mt19937 &rng);//DR patient entering DR treatment
    void enterTRDR_DS(mt19937 &rng);//DS patient entering DR treatment -wont happen
    
    //DISEASE PROGRESSION-------------------------------------------------------------------------------
    bool progressDS(mt19937 &rng);
    bool progressDR(mt19937 &rng);
    //
    bool progressELDS(mt19937 &rng);
    bool progressELDR(mt19937 &rng);
    bool progressLLDS(mt19937 &rng);
    bool progressLLDR(mt19937 &rng);
    bool progressADS(mt19937 &rng);
    bool progressADR(mt19937 &rng);
    bool progressRECDS(mt19937 &rng);
    bool progressRECDR(mt19937 &rng);
    bool progressTRDS_DS(mt19937 &rng);
    bool progressTRDS_DR(mt19937 &rng);
    bool progressTRDR_DR(mt19937 &rng);
    bool progressTRDR_DS(mt19937 &rng);
    
    inline int TNOW(){return _stats->getTime();}
    inline int YNOW(){return _stats->getYear();}
    //
    
    void getOnPT(mt19937 &rng);
    void getOffPT(mt19937 &rng);
    bool model_hhct_indexCase(mt19937 &rng);
    bool model_hhct_hhMember(mt19937 &rng);
    inline bool isOnPT(){return _bIsOnPt;}
    inline bool hasBeenOnPT(){return _bHasBeenOnPt;}
    inline int get_pt_startTime(){return _pt_start_time;}
    inline int get_pt_endTime(){return _pt_end_time;}
    vector<long double> return_pt_individualLevelOutputs();
    
    void set_hhct_hhID(int i){_hhct_hhID=i;}
    void set_hhct_hhSize(int i){_hhct_hhSize=i;};
    void set_hhct_hhPrevADS(int i){_hhct_hhPrevADS=i;};
    void set_hhct_hhPrevADR(int i){_hhct_hhPrevADR=i;};
    void set_hhct_hhPrevLDS(int i){_hhct_hhPrevLDS=i;};
    void set_hhct_hhPrevLDR(int i){_hhct_hhPrevLDR=i;};
    void set_hhct_nkidsLt6InHH(int i){_hhct_nkidsLt6InHH=i;}
    void set_hhct_nkidsLt16InHH(int i){_hhct_nkidsLt16InHH=i;}
    void set_hhct_hhPrevSUS_DSDR(int i){_hhct_hhPrevSUS_DSDR=i;}
    void set_hhct_hhPrevSUS_DS(int i){_hhct_hhPrevSUS_DS=i;}
    void set_hhct_hhPrevSUS_DR(int i){_hhct_hhPrevSUS_DR=i;}
    bool isHhct_hhMember(){return _hhct_hhMember;}
    bool isHhct_indexCase(){return _hhct_indexCase;}
    
    void debug();
};

#endif
