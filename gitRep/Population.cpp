//
//  Population.cpp
//  src
//
//  Created by Trinity on 8/4/15.
//  Copyright (c) 2015 Trinity. All rights reserved.
//
#include "Population.h"
#include "Household.h"
#include "Person.h"
#include "Stats.h"
#include "Functions.h"
#include "Params.h"
#include "GlobalVariables.h"

#include <algorithm>
#include <cmath>
#include <ctgmath> //pow
#include <fstream>  // serialization
#include <iostream>
#include <random>
#include <sstream>  // serialization
#include <vector>
#include <chrono>
#include <memory>


//Serialization
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

using namespace std;
using std::logic_error;
using std::mt19937;
using std::string;
using namespace globalVariables;
using namespace functions;

#include "householdRecipes.hpp"
namespace HouseholdData = DHSHouseHoldData; //reading in the data for HH generation
//---------------------------------------------------
//Error messages
string errPrefixPopulation = "Unexpected Error\n";
string errSuffixPopulation = "\n\nExiting simulation.";
string unfoundPersonErr = errPrefixPopulation +
"Tried to return person but was unable to locate.\n"
"Person id: " + errSuffixPopulation;
string nullPersonStatsErr = errPrefixPopulation +
"Unable to access Person's stat reference.\n" + errSuffixPopulation;
string nullPersonHHErr = errPrefixPopulation +
"Unable to access Person's household reference.\n" + errSuffixPopulation;
string badHhDynamicsErr = errPrefixPopulation +
"Unable to dissolve households with active TB members in them.\n" + errSuffixPopulation;


//---------------------------------------------------
//CLASS FUNCTIONS
//---------------------------------------------------
Population::Population() {}
Population::Population(mt19937 &rng, std::shared_ptr<Params> mp){
    _stats = std::make_shared<Stats>();//create a shared pointer for an object of type Stats, will be shared with Person.
    _mp = mp; //This makes a copy of each _mp per Driver
    
    //creating community members
    for (int i=0;i<__COMMUNITY_SIZE;i++){
        std::shared_ptr<Person> p = std::make_shared<Person>(rng,_mp,_stats,nullptr,false); //set hh to a nullptr
        _vCommunity.push_back(p);
    }
    //
    //_vPeopleReceivedPt is empty
}
Population::~Population() {
    //no need to delete shared pointers:
    //    for ( std::shared_ptr<Household> hh : _vHouseholds ) { delete (hh);}
    //    delete _stats;
}
//Serialization:
bool Population::compare(Population * p){
    //    //compare fix attributes:
    
    //    //stats?params?
    //    if (! _stats->compare(p->_stats)) return false;
    //    if (!_mp->compare(*p->_mp)) return false;
    //    //
    if (_vHouseholds.size() != p->_vHouseholds.size()) return false;
    //    //loop over HH members:
    //    int N=_vHouseholds.size();
    //    for (int i=0;i<N;i++){
    //        if (! _vHouseholds[i]->compare( p->_vHouseholds[i])) return false;
    //    }
    
    
    return true;
}

//---------------------------------------------------
//PARAMETERS READ
//---------------------------------------------------
void Population::readAllParams(std::shared_ptr<Params> mp){//reads all parameter values from the mp file
    _mp = mp;
    for (const auto& p: _vCommunity){
        p->readAllParams(mp);//reads all params from mp
    }
}

//---------------------------------------------------
//HELPER FUNCTIONS
//---------------------------------------------------
std::shared_ptr<Person> Population::returnPerson(const int id) const {
    for (auto& p: _vCommunity){
        if (p->getId()==id) return p;
    }
    throw logic_error(unfoundPersonErr + to_string(1));
}
void Population::initializeEpidemic(mt19937 &rng, const int prevalence){
    vector<int> vIDs; int N=(int) _vCommunity.size();
    for (int i=0;i<N;i++)    vIDs.push_back(i);
    //
    for (int i=0;i<prevalence;i++){
        int j=rng_unif_int(rng,(int) vIDs.size());
        int id=vIDs[j];
        _vCommunity[id]->setTransRouteDS(-2); //coded  value to mark those in the initial wave
        _vCommunity[id]->enterADS(rng,false);
        vIDs[j]=vIDs.back();vIDs.pop_back();
    }
}

void Population::initializeEpidemic(mt19937 &rng, const int ELDS, const int LLDS, const int ADS, const int REC) {
    vector<int> vIDs; int N=(int) _vCommunity.size();
    for (int i=0;i<N;i++) vIDs.push_back(i);
    //
    int id=0; int j=0;
    for (int i=0;i<ELDS;i++){
        j=rng_unif_int(rng,(int) vIDs.size());
        id=vIDs[j];
        _vCommunity[id]->setTransRouteDS(-2);
        _vCommunity[id]->enterELDS(rng);
        vIDs[j]=vIDs.back();vIDs.pop_back();
    }
    for (int i=0;i<LLDS;i++){
        j=rng_unif_int(rng,(int) vIDs.size());
        id=vIDs[j];
        _vCommunity[id]->setTransRouteDS(-2);
        _vCommunity[id]->enterLLDS(rng);
        vIDs[j]=vIDs.back();vIDs.pop_back();
    }
    for (int i=0;i<ADS;i++){
        j=rng_unif_int(rng,(int) vIDs.size());
        id=vIDs[j];
        _vCommunity[id]->setTransRouteDS(-2);
        _vCommunity[id]->enterADS(rng,false);
        vIDs[j]=vIDs.back();vIDs.pop_back();
    }
    for (int i=0;i<REC;i++){
        j=rng_unif_int(rng,(int) vIDs.size());
        id=vIDs[j];
        _vCommunity[id]->setTransRouteDS(-2);
        _vCommunity[id]->enterRECDS(rng,false);
        vIDs[j]=vIDs.back();vIDs.pop_back();
    }
}
//---------------------------------------------------
//ANNUAL MODULES
//---------------------------------------------------
void Population::modelNaturalDynamics(mt19937 &rng, bool cull ){ 
    //00- READ ANNUAL MORTALITY PROBABILITIES FOR EACH YEAR: use year 2000 for years <=2000, then 2001 to 2016, and apply 2016 afterward
    const unsigned ynow=YNOW();
    //To get the growth rate correctly, the original population
    //size should be estimated before DS/DR deaths were modeled
    auto original_pop_size = returnNumPeople() + _stats->return_nMortalityDS()+_stats->return_nMortalityDR();
    vector<double> vMort_male=__vvMORTALITY_MALE[0]; //year 2000 or before
    vector<double> vMort_female=__vvMORTALITY_FEMALE[0];
    if ((ynow>2000) && (ynow < 2016)) {
        vMort_male=__vvMORTALITY_MALE[ynow-2000];
        vMort_female=__vvMORTALITY_FEMALE[ynow-2000];
    }
    if (ynow>2015){
        vMort_male=__vvMORTALITY_MALE[16];
        vMort_female=__vvMORTALITY_FEMALE[16];
    }
    //1-MODEL DEATHS
    int nNaturalDeaths=0;
    int N=(int) _vCommunity.size();
    for(int i=N-1;i>-1;i--){ //loop through the populaton, from end to start, so we can pop_back() those who die
        std::shared_ptr<Person> & p= _vCommunity[i];
        //1-Die due to natural mortality?
        int gender=p->getGender();//0 male; 1 female
        bool willdie=false;
        if (gender==0){
            willdie= p->willDieNaturally(rng,vMort_male);}
        else{
            willdie= p->willDieNaturally(rng,vMort_female);}
        //
        if (willdie){ //2-REMOVE DEAD PEOPLE
            if (p->isMarkedDead()) {
                cout <<"Already Marked to Die" << endl;
            }
            p->setMarkedDead(true);
            nNaturalDeaths++;
            //record stats
            _stats->record_nNaturalDeaths();
            _stats->record_vAgeAtDeath(p->getAge());
            //if person is actively infected or in treatemnet for TB, count the mortality
            if (p->getDSstate()>3) _stats->record_nMortalityDS();
            if (p->getDRstate()>3) _stats->record_nMortalityDR();
            //Remove the dead person from their current household
            if (p->hasHH()){ //if this person has a household
                p->getHH()->removeDeadMembers();
                p->clearHH();
                //p->setHH(nullptr);
            }
            //Remove the dead person from community
            _vCommunity[i].reset();
            _vCommunity[i]=_vCommunity.back();
            _vCommunity.pop_back();
        }
        else //3-ADD AGE FOR THOSE ALIVE
            p->addAge();
    }
    //MODEL BIRTHS:
    // This code assumes that we are starting to record at year 2000, so everything is calibrated to that.
    double coef = 0.0;
    if (cull) {
        //Growth Rate is constant
        /*
         *coef = _growthRateConstat;
         */
        //Growth rate changes after _growthRateVariableYear (2000)
        coef = (ynow >= _growthRateVariableYear)? (
                                                   (ynow >= _growthRateVariableYear + _growthRateN)?
                                                   _vgrowthRates[_growthRateN - 1] :
                                                   _vgrowthRates[ynow - _growthRateVariableYear]
                                                   ) :
        _growthRateConstant;
    } else {
        //Zero growth until 1900, 2% growth until 2000, decreasing afterwards
        coef = (ynow >= _growthRateConstantYear) ? (
                                                    (ynow <= _growthRateVariableYear) ?
                                                    _growthRateConstant :
                                                    (ynow < (_growthRateVariableYear + _growthRateN)) ?
                                                    _vgrowthRates[ynow - _growthRateVariableYear] :
                                                    _vgrowthRates[_growthRateN - 1]
                                                    ) :
        0.0;
    }
    
    const auto nBirths = static_cast<int>(original_pop_size * (1 + coef)) - returnNumPeople();
    modelBirths(rng,nBirths);
}
void Population::modelBirths(mt19937 &rng, const int nBirths){
    //assign a proportion of births to existing households
    //1-determin the proportion of population that are in HHs
    int nHH=0; int popSize=0;
    for (auto & p : _vCommunity){
        if (p->hasHH()) nHH++;
        popSize++;
    }
    double propWithHH=nHH/(double)popSize;
    //2-allocate a proportional number of births to existing households
    double nAssignments= floor(propWithHH * nBirths);
    //nAssignments = 0;
    
    
    for (int i=0;i<nBirths;i++){
        std::shared_ptr<Person> p = std::make_shared<Person>(rng, _mp, _stats, nullptr, true);
        _vCommunity.push_back(p);
        if (nAssignments > 0){
            int var=rng_unif_int(rng,(int)  _vHouseholds.size()); //choose a random household
            p->setHH(_vHouseholds[var]); //set their household pointer
            _vHouseholds[var]->addMember(p); // add them to the household
            nAssignments--;
        }}
}
void Population::modelDiseaseProgress(mt19937 &rng) {
    
    int N=_vCommunity.size();
    for(int i=N-1;i>-1;i--){ //loop through the populaton, from end to start, to pop_back() those who die
        std::shared_ptr<Person> & p= _vCommunity[i];
        if (p->getDSstate()>0 )
            p->progressDS(rng);
        if (p->getDRstate()>0 )
            p->progressDR(rng);
        if(p->isMarkedDead()){
            //Recod stats
            _stats->record_vAgeAtDeath(p->getAge());
            //Remove the dead person from their current household
            if (p->hasHH())
                p->getHH()->removeDeadMembers(); // !!!!!!should this be here? (if p is in a household!
            //Remove the dead person from community
            _vCommunity[i].reset();
            _vCommunity[i]=_vCommunity.back();
            _vCommunity.pop_back();
        }}
}
void Population::modelContacts(mt19937 &rng){
    //1-Crating a 2D-vector of community members within different age groups for sampling below
    std::vector<std::shared_ptr<Person>> vMarked;//list all newly infected cases with DS and/or DR
     array<vector<std::size_t>, 17> communityContacts {};
    
    //Setup
    for (auto commIndex = 0u; commIndex < _vCommunity.size(); commIndex++) {
        //Mark all individuals negative for DR and DS transmission
        _vCommunity[commIndex]->setMarkedTran_DR(false);
        _vCommunity[commIndex]->setMarkedTran_DS(false);
        //Store the index into _vCommunity into the proper age group
        const auto ag = _vCommunity[commIndex]->getContactAgeGroup();
        communityContacts[ag].push_back ( commIndex );
    }
    for (auto& p: _vCommunity){ //model contacts (transmissions) from those with active TB or those failing treatment
           if ((p->getDSstate()==DS_A) || //active DSTB
               (p->getDRstate()==DR_A) || //active DRTB
               (p->getDSstate()==DS_TRDS && p->isFailingTreatment()) || //has DSTB and is failing DSTB treatment
               (p->getDRstate()==DR_TRDS ) || //has DRTB and is failing DSTB treatment
               (p->getDRstate()==DR_TRDR && p->isFailingTreatment()) //has DRTB and is failing DRTB treatment
               )
           {
//            // Update the current infectiousness **********************************
            const double inf = p->returnInfectiousness(rng);
            // HH contacts ********************************************************
            //// Pass the vMarked vector into the function so it can be filled; saves
            // on the creation and destruction of a new vector (9s per run)
            // Model the person's household contacts; add any newly infected people
            // to vMarked
            p->modelHHcontacts(rng, vMarked, inf);
            // inserts newly infected cases with DS and DR into vMarked
            
            // Community contacts *************************************************
            // Assinging community contacts based on proportion of iteraction
            // Varibles defined in GlobalVariables.h
            const auto contactAgeGroup = p->getContactAgeGroup();
            const auto contactProp = _propContactsByAgeGroup[contactAgeGroup];
            const auto numContacts = _totalContactNumber[contactAgeGroup];
            
            //Build a list of contacts for each age group, then model the community
            //contact on each.  If transmission occured, push onto vMarked
            //randomly selecting contacts without replacement
            for (auto i = 0u; i < contactProp.size(); i++) {
                const auto numContactsInAgeGroup = unsigned ( contactProp[i] * numContacts );
                const auto agSize = communityContacts[i].size();
                
                if (numContactsInAgeGroup>= agSize){ //we need more contacts than the size of age-group
                    for (auto j = 0u; j < agSize; j++ ) { //model contacts with all members of agegroup i
                        std::shared_ptr<Person> q = _vCommunity[ communityContacts[i][j] ];
                        if (p->modelCOMcontact(rng, q, inf))
                            vMarked.push_back(q);
                    }
                } else { 
                    //we need less contacts than the size of age-group
                    //generate a vector of IDs for people in that agegroup
                    //iota fills a vector in ascending order, starting with the
                    //provided value (in this case, 0)
                    std::vector<int> tempIds(agSize);
                    std::iota(tempIds.begin(),tempIds.end(), 0);//0,1,2,3,...
                
                    //randomly shuffle the vector //std::shuffle ( tempIds.begin(), tempIds.end(), rng); Way too slow! changed the logic below
                                        
                    //model contacts for the first numContactsInAgeGroup IDs
                    for (auto j = 0u; j < numContactsInAgeGroup; j++ ) {
                        int var=rng_unif_int(rng, tempIds.size()); // choose a random member of tempIds
                        int contactID=tempIds[var]; //save the contact ID in that location
                        tempIds[var]=tempIds.back(); tempIds.pop_back(); //remove that member (selecion without replacement)
                        //model the contact event 
                        std::shared_ptr<Person> q = _vCommunity[ communityContacts[i][contactID]];
                        if (p->modelCOMcontact(rng, q, inf))
                            vMarked.push_back(q);
                    }
                }
                
            }
        }
    }
    //=========================================================================
    // Model Infection events: set _markedForTrans=False; so if a person is reinfected a few times, they're not recounted.
    for (std::shared_ptr<Person> p : vMarked){
        if(p->isMarkedTran_DS())
            p->enterELDS(rng);
        if(p->isMarkedTran_DR())
            p->enterELDR(rng);
    }
}

/*
 Loops through the population and identifies those active cases without a household. Depending on the index case's age and gender, draws a random receipe for HH composition and assigns random members of the community to this household
 */
void Population::modelHhDynamics(mt19937 &rng){
    // **** RESUME OR DISSOLVE EXISTING HOUSEHOLDS
    int TNOWW= TNOW();
    int N=(int) _vHouseholds.size();
    
    for (int i=N-1; i>-1; i--){ //loop from end to the bgginingg so that we pop back the households that are dissolved
        shared_ptr<Household> hh= _vHouseholds[i];
        //0- Update remaining times:
        hh->incRemainingTime(-1);
        //1- Check to see if there was any new infections in the HH at this time step. if so, rest the dissolution time
        bool cont=true; //will continute to dissolve hh
        for (auto& p: *hh){
            if( cont && ((p->getTimeEnteringDS_A()==TNOWW) || (p->getTimeEnteringDR_A()==TNOWW))){
                hh->setRemainingTime( __HOUSEHOLD_FOLLOWUP_PERIOD );
                cont=false; //so if there are more than one new case in this household, we dont increase the dissolution time again and again
            }}
        
        //2- if no new infection was detected, check the remaining time and dissolve if it's due
        if ((cont) && (hh->getRemainingTime() == 0 )) {
            //as a sanity check see if any household member is actively infected or is on PT
            for (auto& p: *hh ) {
                if ((p->getDRstate()>= 4) || (p->getDSstate()>=4 ) || (p->isOnPT() )){
                    //if there are existing cases in the household (Active or getting treated); or on PT; 
                    //who have not left the household yet, just prolong the follow up period until they leave
                    hh->incRemainingTime(1);
                    cont=false; //so if there are more than one new case in this household, we dont increase the dissolution time again and again
                }}
            if (cont){
                for (auto& p: *hh )
                    p->clearHH();
                _vHouseholds[i]=_vHouseholds.back(); _vHouseholds.pop_back();
            }}
        //3- if the household is empty, delete it
        if (hh->getSize() == 0) {
            _vHouseholds[i]=_vHouseholds.back();
            _vHouseholds.pop_back();
        }}
    
    // **** FORM NEW HOUSEHOLDS
    std::vector<std::shared_ptr<Person>> vActiveCasesWithNoHousehold;//list all active cases who are infectious
    //Instead of creating 6 separate vectors, create one array of 6 vectors
    array < vector<std::shared_ptr<Person>>, 6> groupArray {};
    //Use Constants for access:
    const auto MGT21 = 0;
    const auto FGT21 = 1;
    const auto MGT14LT21 = 2;
    const auto FGT14LT21 = 3;
    const auto MLT14 = 4;
    const auto FLT14 = 5;
    
    for (auto& p: _vCommunity) {
        // 1- new active cases without a household:
        if ((p->getTimeEnteringDR_A()==TNOWW) || (p->getTimeEnteringDS_A()==TNOWW)) {
            if (!p->hasHH()) {
                vActiveCasesWithNoHousehold.push_back(p);
            }
        } else {
            //The older code would skip grouping these entries if we had no
            //active cases with no household; this probably doesn't happen
            //often so we're going to skip that case here and just group
            //everybody without a household.  In this way we skip an entire
            //second loop through _vCommunity
            if (!p->hasHH()) { //We must be careful here; we are using the fact that they don't have
                //a household as an indication that they don't have TB, but without
                //the above if-statement we will get TB infected people in the buckets.
                
                //Not a new active case and has no household, should be grouped
                if (p->getGender()==0) { //Male
                    if (p->getAge()<15)
                        groupArray[MLT14].push_back(p);
                    else{
                        if (p->getAge()<21)
                            groupArray[MGT14LT21].push_back(p);
                        else
                            groupArray[MGT21].push_back(p);
                    }
                }
                else { //Female
                    if (p->getAge()<15)
                        groupArray[FLT14].push_back(p);
                    else{
                        if (p->getAge()<21)
                            groupArray[FGT14LT21].push_back(p);
                        else
                            groupArray[FGT21].push_back(p);
                    }
                }
            }
        }
    }
    
    // Loop over all active cases, form new households for them =============================================
    for (auto& p:vActiveCasesWithNoHousehold){//based on person's age and gender, choose the approperiate hhsize vector
        //1- Select a random HHrecipe------------------------------------------------------
        HouseholdData::Record hhRecipe;
        int var;
        {  if (p->getGender()==0){//Male
            if (p->getAge()<15) {//male child
                var=rng_unif_int(rng, HouseholdData::nChildM); //random row
                hhRecipe=HouseholdData::dataChildM[var];}
            else{
                if (p->getAge()<21) {//male teen
                    var=rng_unif_int(rng, HouseholdData::nTeenM); //random row
                    hhRecipe=HouseholdData::dataTeenM[var];}
                else{//male adult
                    var=rng_unif_int(rng, HouseholdData::nAdultM); //random row
                    hhRecipe=HouseholdData::dataAdultM[var];}
            }}
        else{//Female
            if (p->getAge()<15) {//female child
                var=rng_unif_int(rng, HouseholdData::nChildF); //random row
                hhRecipe=HouseholdData::dataChildF[var];}
            else{
                if (p->getAge()<21) {//female teen
                    var=rng_unif_int(rng, HouseholdData::nTeenF); //random row
                    hhRecipe=HouseholdData::dataTeenF[var];}
                else{//female adult
                    var=rng_unif_int(rng, HouseholdData::nAdultF); //random row
                    hhRecipe=HouseholdData::dataAdultF[var];}
            }}
        }
        //2- Form the Household based on the selected recipe ------------------------------------------------------
        int HhId = _stats->getHouseholdID();_stats->incHouseholdID();
        std::shared_ptr<Household> hh=std::make_shared<Household>(rng,_mp,HhId, _stats->getTime());
        //add this person to the household:
        p->setHH(hh);
        hh->addMember(p);
        for (int i=0;i<hhRecipe.numFGT21 && groupArray[FGT21].size() != 0;i++){//Female greater than 21
            var=rng_unif_int(rng, (int) groupArray[FGT21].size());
            //choose a random member of available population
            groupArray[FGT21][var]->setHH(hh);//set their household pointer
            hh->addMember(groupArray[FGT21][var]);//add member to the household:
            groupArray[FGT21][var]=groupArray[FGT21].back();
            groupArray[FGT21].pop_back();//remove this selected member
        }
        for (int i=0;i<hhRecipe.numMGT21 && groupArray[MGT21].size() != 0;i++){//Male greater than 21
            var=rng_unif_int(rng, (int) groupArray[MGT21].size());
            groupArray[MGT21][var]->setHH(hh);
            hh->addMember(groupArray[MGT21][var]);
            groupArray[MGT21][var]=groupArray[MGT21].back();
            groupArray[MGT21].pop_back();//remove this selected member
        }
        for (int i=0;i<hhRecipe.numFGT14LT21 && groupArray[FGT14LT21].size() != 0;i++){//Female greater than 14, less than 21
            var=rng_unif_int(rng, (int) groupArray[FGT14LT21].size());
            groupArray[FGT14LT21][var]->setHH(hh);
            hh->addMember(groupArray[FGT14LT21][var]);
            groupArray[FGT14LT21][var]=groupArray[FGT14LT21].back();
            groupArray[FGT14LT21].pop_back();
        }
        for (int i=0;i<hhRecipe.numMGT14LT21 && groupArray[MGT14LT21].size() != 0;i++){//Male greater than 14, less than 21
            var=rng_unif_int(rng, (int) groupArray[MGT14LT21].size());
            groupArray[MGT14LT21][var]->setHH(hh);
            hh->addMember(groupArray[MGT14LT21][var]);
            groupArray[MGT14LT21][var]=groupArray[MGT14LT21].back();
            groupArray[MGT14LT21].pop_back();
        }
        for (int i=0;i<hhRecipe.numFLT14 && groupArray[FLT14].size() != 0;i++){//Female less than 14
            var=rng_unif_int(rng, (int) groupArray[FLT14].size());
            groupArray[FLT14][var]->setHH(hh);
            hh->addMember(groupArray[FLT14][var]);
            groupArray[FLT14][var]=groupArray[FLT14].back();
            groupArray[FLT14].pop_back();
        }
        for (int i=0;i<hhRecipe.numMLT14 && groupArray[MLT14].size() != 0;i++){//Male less than 14
            var=rng_unif_int(rng, (int) groupArray[MLT14].size());
            groupArray[MLT14][var]->setHH(hh);
            hh->addMember(groupArray[MLT14][var]);
            groupArray[MLT14][var]=groupArray[MLT14].back();
            groupArray[MLT14].pop_back();
        }
        //3- Add this household to the community ------------------------------------------------------
        _vHouseholds.push_back(hh);
    }
}
/*
 * Population::cullPop(rng)
 *
 * We want to keep the population low while we burn-in to a reasonable rate/population state;
 * the cullPop function will remove both individuals and households such that the ending
 * population is still representative of the larger population, but with a number of
 * individuals closer to globalVariables::__COMMUNITY_SIZE, which is curently 100,000
 */
void Population::cullPop( std::mt19937 &rng ) {
    const double targetSize = globalVariables::__COMMUNITY_SIZE;
    //Get the current population size
    const auto pop_size = returnNumPeople();
    //Calculate the percentage of people and households we want to remove
    // 1 - (100000 / pop_size)
    const auto percent_to_remove = 1 - (targetSize / static_cast<double>(pop_size));
    
    //Remove the people:
    //PseudoCode-
    //  get a list of the people without households and not marked for dead
    //  get the size of that list
    //  multiply by percent_to_remove to get the number of people to remove
    //
    //  if the number of people to remove is greater than zero, continue.
    //
    //  generate a list of integers from 0 to the size of the above list.
    //  shuffle that list of integers
    //
    //  loop person_remove_count times {
    //      mark as dead the next person from the shuffled list of integers
    //  }
    
    
    auto household_free = popNoHouseholdNotDead();
    
    const auto hhf_size = household_free.size();
    
    const auto person_remove_count = ceil(hhf_size * percent_to_remove);
    
    if (person_remove_count > 0) {
        auto p_idx = 0u;
        vector<unsigned int> person_indexes (hhf_size);
        std::generate_n(std::begin(person_indexes), hhf_size, [&p_idx](){return p_idx++;});
        std::shuffle(std::begin(person_indexes), std::end(person_indexes), rng);
        
        for (auto lc = 0u; lc < person_remove_count; lc++) {
            household_free[person_indexes[lc]]->setMarkedDead(true);
        }
    }
    
    //Remove the households:
    //PseudoCode-
    //  get the number of households
    //  multiply the number of households by the percent_to_remove, get the hh_remove_count
    //
    //  Create a list of the integers between 0 and household size (generate_n)
    //  Shuffle that list of integers
    //
    //  loop hh_remove_count times {
    //      get the household that corresponds to the next integer from the list
    //      for each housemember:
    //          mark as dead
    //          clear the household pointer
    //
    //      empty (drain) the household
    //  }
    //
    const auto hh_size = returnNumHouseholds();
    auto hh_remove_count = ceil(hh_size * percent_to_remove);
    
    //Generate the vector<int> and shuffle it
    auto h_idx = 0u;
    vector<int> hh_indexes (hh_size);
    std::generate_n(std::begin(hh_indexes), hh_size, [&h_idx](){return h_idx++;});
    std::shuffle(std::begin(hh_indexes), std::end(hh_indexes), rng);
    
    auto hh_pop_removed = 0;
    //Loop hh_remove_count times
    for (auto index = 0u; index < hh_remove_count; index++) {
        auto hh = _vHouseholds[hh_indexes[index]];
        for (auto p : *hh) {
            //Loop through the members
            //Mark the member for death
            p->setMarkedDead(true);
            //Clear the household shared pointer
            p->clearHH();
            hh_pop_removed++;
        }
        //Empty the household
        hh->drain();
    }
    
    //Remove the marked people from the population
    //Loop through the whole population backwards and remove those who are markedDead
    for(int i=pop_size-1; i>=0; i--) {
        if (_vCommunity[i]->isMarkedDead()) {
            _vCommunity[i].reset();
            _vCommunity[i]=_vCommunity.back();
            _vCommunity.pop_back();
        }
    }
    
    //Delete the empty households
    //Loop through the household vector backwards and remove those that are empty
    for (int i=hh_size-1; i>=0; i--) {
        if (_vHouseholds[i]->getSize() == 0) {
            _vHouseholds[i].reset();
            _vHouseholds[i]=_vHouseholds.back();
            _vHouseholds.pop_back();
        }
    }
    
    //Display
    if (__bPRINT) {
        cout << "Cull - removing " << person_remove_count + hh_pop_removed;
        cout << " people.  " << person_remove_count << " tb neg individuals and ";
        cout << hh_remove_count << " households (total pop : " << hh_pop_removed;
        cout << ") from a non household population of ";
        cout << hhf_size << ", a total population of " << pop_size << " with ";
        cout << hh_size << " households.\n";
    }
}

//Beyond the DS stage, starting in year 2000 we model reductions in both Transmission and Reactivation.
//This function is used when Reactivation (probSlowProb) is reduced.  If an individual has a value other
//than -1 for DS or DR progression, then they are scheduled for a future disease progression.  
//Recalculate the time their progression is scheduled with the new value of progSlowProg
void Population::recalculateSlowProg( mt19937 &rng ) {
    for (auto &p : _vCommunity) {
        if (p->getProgressToADS_LLDS() > -1) {
            p->setProgressToADS_LLDS ( p->returnTimeToSlowProg(rng) );
        }
        if (p->getProgressToADR_LLDR() > -1) {
            p->setProgressToADR_LLDR ( p->returnTimeToSlowProg(rng) );
        }
    }
}

//---------------------------------------------------
//OUTPUTS
//---------------------------------------------------
unsigned Population::returnNumPeople() const {
    return (int) _vCommunity.size();
}
vector<double> Population::returnDistPopAgeBucket(  const bool includeMale, const bool includeFemale ) const {
    // 5-year age brackets, 0-4, 5-9, ...., 100+ (21 intervals)
    vector<double> vDist(21,0);
    for (auto& p: _vCommunity){
        int male = 0;
        int female = 1;
        bool includePerson =
        (includeMale && includeFemale) ||
        (includeMale && p->getGender() == male) ||
        (includeFemale && p->getGender() == female);
        if (includePerson) {
            int ageBucket = floor(p->getAge() / 5);
            if (ageBucket > 20) ageBucket=20;  // marginal outlier
            vDist[ageBucket]++;
        }
    }
    return (vDist);
    /*//21 agegroups
     "0-4",
     "5-9",
     "10-14",
     "15-19 ",
     "20-24",
     "25-29",
     "30-34",
     "35-39",
     "40-44",
     "45-49",
     "50-54",
     "55-59",
     "60-64",
     "65-69",
     "70-74",
     "75-79",
     "80-84",
     "85-89",
     "90-94",
     "95-99",
     "100-104"*/
}
double Population::returnMedianPopAge() const {
    vector<double> vAges;
    for (auto & p: _vCommunity){
        vAges.push_back(p->getAge());
    }
    return (vectorMedian(vAges));
}

vector<double> Population::returnDistPopGend() const {
    vector<double> vDist(2,0);
    int n=0;
    for (auto& p: _vCommunity){
        n++;
        vDist[p->getGender()]++;
    }
    return (vectorDivision(vDist,n));
}
int Population::returnNumHouseholds() const {
    return (int) _vHouseholds.size();
}
vector<int> Population::returnDistHhSize() const {
    vector<int> vDist(100,0);//0-10 members
    int n=0;
    for (auto& hh: _vHouseholds){
        n++;
        vDist[hh->getSize()]++;
    }
    vDist.push_back(n);
    return vDist;
}
int Population::returnNumPeopleInHouseholds() const {
    int n=0;
    for (auto & p:_vCommunity)
        if (p->hasHH()) n++;
    return n;
}
double Population::returnMedianHhSize() const {
    vector<double> vSizes;
    for (auto& hh: _vHouseholds){
        vSizes.push_back(hh->getSize());
    }
    return (vectorMedian(vSizes));
}
vector<vector<double>> Population::returnDistStateSizes_DSDR() const {
    vector<double> temp(7,0); //6 DS states
    vector<vector<double>> vec(7,temp);//7 DR states
    for (auto& p: _vCommunity){
        int ds=p->getDSstate();
        int dr=p->getDRstate();
        vec[dr][ds]++;
    }
    return vec;
}
vector<double> Population::returnDistStateSizes_DSDR_oneline() const {
    vector<double> temp(14,0); //6 DS states
    for (auto& p: _vCommunity){
        temp[p->getDSstate()]++;
        temp[p->getDRstate()+7]++;
    }
    temp.push_back(_stats->getYear());
    return temp;
}
vector<double> Population::returnDistStateSizes_DS() const{
    vector<double> vec(7,0); //6 DS states
    for (auto& p: _vCommunity){
        int ds=p->getDSstate();
        vec[ds]++;
    }
    return vec;
}
vector<double> Population::returnDistStateSizes_DR() const {
    vector<double> vec(7,0); //6 DS states
    for (auto& p: _vCommunity){
        int dr=p->getDRstate();
        vec[dr]++;
    }
    return vec;
}
int Population::returnPrevalence_DSA() const {
    int var=0;
    for (auto& p: _vCommunity){
        if (p->getDSstate()==DS_A) var++;
    }
    return var;
}
int Population::returnPrevalence_DRA() const {
    int var=0;
    for (auto& p: _vCommunity){
        if (p->getDRstate()==DR_A) var++;
    }
    return var;
}
/*
 return age dist of people in households of size X (dim 1 is age, dim 2 is the number of HHs that fall in this category)
 */
vector<vector<int>> Population::returnDistHhAge (const unsigned hhSize) const {
    vector<vector<int>> ret;
    for (const auto &hh: _vHouseholds) {
        if (hh->getSize() == hhSize) {
            vector<int> hh_value; //Previous version had default values; caused errors
            const auto personList = hh->getMembers();
            for (const auto &person : personList) {
                hh_value.push_back(person->getAge());
            }
            std::sort(hh_value.begin(), hh_value.end());
            ret.push_back(hh_value);
        }
    }
    
    return ret;
}
/*
 * Population::returnDistHhAge_allHhSizes
 * Returns a 3 dimensional vector : dim 1 is a vector of ages for households of size x, dim 2 is the number 
 * of households of size x, and dim 3 is x
 */
vector<vector<vector<int>>>  Population::returnDistHhAge_allHhSizes () const {
    //Figure out the largest household size
    //So we know how many we need to check and return.
    //
    const auto max_hh_size = getMaxHHSize();
    
    vector<vector<vector<int>>> ret;
    

    //Starting at size 1 because there aren't any people in households size 0
    //End at max size, inclusive

    for (std::size_t i = 1; i <= max_hh_size; i ++) {
        ret.push_back (returnDistHhAge(i));
    }
    
    return ret;
}
/*
 * Get the size of the largest household by examining all households and finding the largest.
 */
std::size_t Population::getMaxHHSize() const {
    std::size_t max_hh_size {0};
    for (const auto &hh : _vHouseholds) {
        const auto size = hh->getSize();
        if (max_hh_size < size) {
            max_hh_size = size;
        }
    }
    return max_hh_size;
}
/*
 * Returns the proportion of people with active disease who got there via early/fast
 * progression
 */
double Population::returnEarlyProgressionProp() {
    auto total_active = 0;
    auto total_early_prog = 0;
    for (const auto &p: _vCommunity) {
        if (p->getDSstate() == DS_A) {//Person with active disease
            total_active++;
            //when was their time to enter early latent infection
            const auto el_start = p->getTimeEnterDS_EL();
            const auto a_start = p->getTimeEnteringDS_A();
            
            //I'm not sure how the progression goes right now; I'm checking
            //if el_start is -1 because I imagine there may be a way to proceed
            //directly to late latent infection.  If there is not then the
            //check is unnecessary, but will not cause an error.  -1 is the default
            //value for all the timing variables
            
            if ( el_start != -1 && ( (a_start - el_start) <= (12 * 5) ) ) {
                
                total_early_prog++;
            }
            
        }
    }
    return static_cast<double>(total_early_prog) / static_cast<double>(total_active);
}

/*
    Population::returnLTBIPrev
    This function will return the proportion of people who have EL or LL for their DS status
    (Early latent and Late latent)
 */
double Population::returnLTBIPrev() {
    const double pop_size = static_cast<double>(_vCommunity.size());
    double latent_count = 0.0;
    
    for (const auto &p: _vCommunity) {
        const auto p_status = p->getDSstate();
        
        if (p_status == DS_EL || p_status == DS_LL) {
            latent_count++;
        }
    }
    return latent_count / pop_size;
}
/*
    Population::popNoHouseholdNotDead
    Return a list of shared pointers to people who don't have a household
    and aren't marked dead.  Used in the cull.
 */
vector<std::shared_ptr<Person>> Population::popNoHouseholdNotDead() {
    vector<std::shared_ptr<Person>> noHHnD;
    for (auto i = 0u; i < returnNumPeople(); i++) {
        if (!_vCommunity[i]->hasHH() && !_vCommunity[i]->isMarkedDead()) {
            noHHnD.push_back ( _vCommunity[i] );
        }
    }
    return noHHnD;
}

/*
 Baseline set of all outputs reported from all runs throughout the calibration period (pre-2020)
 */
vector<long double> Population:: returnBaseOutputs() {
    
    vector<long double> vRes;
    /*
     */ //This will be added in writeVector, not here
    vRes.push_back(_stats->getYear());
    vRes.push_back(returnNumPeople());
    //"rep","year", "pop"
    
    vRes.push_back(returnPrevalence_DSA()); //6 DS Prevalence, 9 Total
    vRes.push_back(_stats->return_nIncDS());
    vRes.push_back( _stats->return_nIncDS_fastprog());
    vRes.push_back( _stats->return_nIncDS_slowprog());
    vRes.push_back( _stats->return_nIncDS_relapse());
    //    vRes.push_back( _stats->return_nIncDS_Failure()); //we dont count trt failure toward incidence  
    //"nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure"
    
    vRes.push_back(returnPrevalence_DRA()); //7 DR Prevalence, 16 Total
    vRes.push_back( _stats->return_nIncDR());
    vRes.push_back( _stats->return_nIncDR_fastprog());
    vRes.push_back( _stats->return_nIncDR_slowprog());
    vRes.push_back( _stats->return_nIncDR_relapse());
    vRes.push_back( _stats->return_nIncDR_resist());
    //    vRes.push_back( _stats->return_nIncDR_Failure());//we dont count trt failure toward incidence
    //"nPrevDR","nIncDR","nIncDR_fastProg","nIncDR_slowProg","nIncDR_relapse","nIncDR_resis","nIncDR_failure",
    
    vRes.push_back( _stats->return_nTransDS()); //4 DS & DR Transmission, 20 Total
    vRes.push_back( _stats->return_nTransDR());
    vRes.push_back( _stats->return_nTransDS_HH());
    vRes.push_back( _stats->return_nTransDR_HH());
    //"nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH",
    
    vRes.push_back( _stats->return_nIncHH()); //4 HH & Com Incidence, 24 Total
    vRes.push_back( _stats->return_nIncCOM());
    vRes.push_back( _stats->return_nIncDS_HH());
    vRes.push_back( _stats->return_nIncDR_HH());
    //"nIncHH","nIncCom","nIncDsHH","nIncDrHH",
    
    vRes.push_back( _stats->return_nELDS()); //4 Mortality, 28 Total
    vRes.push_back( _stats->return_nELDR());
    vRes.push_back( _stats->return_nMortalityDS());
    vRes.push_back( _stats->return_nMortalityDR());
    //"nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR" ,
    
    vRes.push_back( _stats->return_nVisits()); //5 Diagnosis/Treatment, 33 Total
    vRes.push_back( _stats->return_nDiag_DS());
    vRes.push_back( _stats->return_nDiag_DR());
    vRes.push_back( _stats->return_nDST());
    vRes.push_back( _stats->return_nPretreatmentLTF());
    //"nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
    
    vRes.push_back( _stats->return_nTrtDS_DS()); //6 Therapy, 39 Total
    vRes.push_back( _stats->return_nTrtDS_DR());
    vRes.push_back( _stats->return_nTrtDR_DR());
    vRes.push_back( _stats->return_nFailedDSTrt_DS());
    vRes.push_back( _stats->return_nFailedDSTrt_DR());
    vRes.push_back( _stats->return_nFailedDRTrt_DR());
    //"nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
    //"nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR",
    
    vRes.push_back( _stats->return_nNewPatient() ); //4 Patient, 43 Total
    vRes.push_back( _stats->return_nRetreatedPatient() );
    vRes.push_back( _stats->return_nNewDRTB() );
    vRes.push_back( _stats->return_nRetreatedDRTB() );
    //"nNewTB","nRetreatedTB","nNewDR","nRetreatedDR",
    
    vector<double> vec=returnDistStateSizes_DS();
    vRes.insert(vRes.end(),vec.begin(),vec.end()); //7 Disease State Size,
    //"nSUSDS","nELDS","nLLDS","nRDS","nADS","nTRDS_DS","nTRDR_DS",
    vec=returnDistStateSizes_DR();
    vRes.insert(vRes.end(),vec.begin(),vec.end()); //7 Disease State Sizes
    //"nSUSDR","nELDR","nLLDR","nRDR","nADR","nTRDR_DS","nTRDR_DR",
    
    vRes.push_back(_mp->getD("coefTrans")); //7 Variables, 57 Total (+2)
    vRes.push_back(_mp->getD("coefHHTrans"));
    vRes.push_back(_mp->getD("coefComTrans"));
    vRes.push_back(_mp->getD("probMaxMortalityTB"));
    vRes.push_back(_mp->getD("probMaxSeekCare"));
    vRes.push_back(_mp->getD("probResistance"));
    vRes.push_back(_mp->getD("coefMaxInfect_DR_RelTo_DS"));
    vRes.push_back(_mp->getD("coefTransReducPost2000"));
    vRes.push_back(_mp->getD("coefReactReducPost2000"));
    //"_coefTrans","_coefHHTrans","_coefComTrans","_probMaxMortalityTB",
    //"_probMaxSeekCare","_probResistance","_coefMaxInfect_DR_RelTo_DS",
    //"_coefTransReducPost2000", "_coefReactReducPost2000"
    
    vRes.push_back(_mp->getD("coefInfec_0to10")); //12 Distributed Variables, 69 Total +1
    vRes.push_back(_mp->getD("probPretreatmentLTF_DSTB"));
    vRes.push_back(_mp->getD("probPretreatmentLTF_DRTB"));
    vRes.push_back(_mp->getD("probDRTrtFailure"));
    vRes.push_back(_mp->getD("probDSTrtFailure"));
    vRes.push_back(_mp->getD("probRelapse_DS"));
    vRes.push_back(_mp->getD("probRelapse_DR"));
    vRes.push_back(_mp->getD("probSlowProg"));
    vRes.push_back(_mp->getD("probSpontResolution"));
    vRes.push_back(_mp->getD("coefImmunityLatent"));
    vRes.push_back(_mp->getD("coefInfect_trtFailure"));
    vRes.push_back(_mp->getD("probSuccessDStrt_DRTB"));
    vRes.push_back(_mp->getD("coefFastProg")); //+1
    //"coefInfec_0to10", "probPretreatmentLTF_DSTB", "probPretreatmentLTF_DRTB",
    //"probDRTrtFailure", "probDSTrtFailure", "probRelapse_DS", "probRelapse_DR",
    //"probSlowProg", "probSpontResolution", "coefImmunityLatent",
    //"coefInfect_trtFailure", "probSuccessDStrt_DRTB", "coefFastProg"
    
    //Tally outputs:
    /* duration dstb */
    double durDstb= (double) vectorAve(_stats->return_vDurDSTB()); //2 DS Duration, 71 Total
    vRes.push_back(durDstb);
    vRes.push_back(_stats->return_vDurDSTB().size());
    /* duration d3tb */
    double durDrtb= (double) vectorAve(_stats->return_vDurDRTB()); //2 DR Duration, 73 Total
    vRes.push_back(durDrtb);
    vRes.push_back(_stats->return_vDurDRTB().size());
    //"durDS","nDurDS","durDR","nDurDR",
    
    /* prev of dstb/drtb in households at the time of dstb diagnosis */
    vRes.push_back(_stats->return_vHhSize_atDiagADS().size()); // #of HH traced //4 HH DS Information, 77 Total
    vRes.push_back( vectorSum( _stats->return_vHhSize_atDiagADS()) ); // #of HH members found
    vRes.push_back( vectorSum(_stats->return_vHhPrevSUS_atDiagADS())); // #of hh members with ADS at ADS diagnosis
    vRes.push_back( vectorSum(_stats->return_vHhPrev_ADS_atDiagADS())); // #of hh members with ADS at ADS diagnosis
    vRes.push_back( vectorSum(_stats->return_vHhPrev_ADR_atDiagADS()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_LADS_atDiagADS()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_LADR_atDiagADS()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_LADS_LADR_atDiagADS()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_ADS_LADR_atDiagADS()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_ADR_LADS_atDiagADS()));
    
    vRes.push_back( vectorSum(_stats->return_vHhPrevADS_hhOriginated_atDiagADS()));
    vRes.push_back( vectorSum(_stats->return_vHhPrevLADS_hhOriginated_atDiagADS()));
    //"nHhTraced_diagADS", "nHhMembersTraced_diagADS","nHhPrevADS_diagADS", "nHhPrevADR_diagADS",
    
    vRes.push_back(_stats->return_vHhSize_atDiagADR().size());
    vRes.push_back( vectorSum( _stats->return_vHhSize_atDiagADR()) );
    vRes.push_back( vectorSum(_stats->return_vHhPrevSUS_atDiagADR()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_ADS_atDiagADR()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_ADR_atDiagADR()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_LADS_atDiagADR()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_LADR_atDiagADR()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_LADS_LADR_atDiagADR()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_ADS_LADR_atDiagADR()));
    vRes.push_back( vectorSum(_stats->return_vHhPrev_ADR_LADS_atDiagADR()));
    
    vRes.push_back( vectorSum(_stats->return_vHhPrevADR_hhOriginated_atDiagADR()));
    vRes.push_back( vectorSum(_stats->return_vHhPrevLADR_hhOriginated_atDiagADR()));
    //  "nHhTraced_diagADR", "nHhMembersTraced_diagADR",  "nHhPrevSUS_diagADR","nHhPrevADS_diagADR", "nHhPrevADR_diagADR", "nHhPrevLADR_diagADR","nHhPrevADR_hhOriginated_diagADR","nHhPrevLADR_hhOriginated_diagADR",
    
    //+ 1, coefFastProg
    //+ 2, Additional varaibles to reduce transmission after 2000, total 86
    //+ 1, Proportion of active cases that progressed from early latent to active in 5 years or less, total 87
    //+ 1, Prevalence of LTBI (EL and LL), total 88
    //+ 1, RNG Seed, total 89
  
    
    vector<double> vDemog= returnDemogOutputs();
    vRes.insert(vRes.end(),vDemog.begin(), vDemog.end());
    

    return (vRes);
}
vector<double> Population::returnDemogOutputs(){
    
//additional outputs needed:
//    median age
//    Population proportion <=5y
//    Population proportion <=15y
//    Median age of households
//    Median size of households
    vector<int> vAges;
    double n=0;
    double lt5=0;
    double lt15=0;
    double a;
    for (const auto &p: _vCommunity){
        a=p->getAge();
        vAges.push_back(a );
        if (a<=15) lt15++;
        if (a<=5) lt5++;
        n++;
    }
    vector<double> vRes;
    vRes.push_back(vectorMedian(vAges)); //median age
    vRes.push_back(lt5/n); //Population proportion <=5y
    vRes.push_back(lt15/n);// Population proportion <=15y
    //------------------------------------
    vector<double> vHhsizes;
    vector<double> vHhAges;
    double numHH=0;
    double numPeopleInHH=0;
    lt5=0;
    lt15=0;
    for (const auto &hh: _vHouseholds) {
        vHhsizes.push_back(hh->getSize());
        numHH++;
            const auto personList = hh->getMembers();
            for (const auto &person : personList) {
                numPeopleInHH++;
                a=person->getAge();
                vHhAges.push_back(a);
                if (a<=15) lt15++;
                if (a<=5) lt5++;
            }
    }
    vRes.push_back(numHH); // number of HH
    vRes.push_back(numPeopleInHH); //Num of people in HH
    vRes.push_back(lt5/numPeopleInHH); //prp <=5
    vRes.push_back(lt15/numPeopleInHH); //prp <=15
    vRes.push_back(vectorMedian(vHhsizes));//Median size of households
    vRes.push_back(vectorMedian(vHhAges));//   Median age of households
    
    return (vRes);
}

//---------------------------------------------------
//DEBUG
//---------------------------------------------------
void Population::debug1(){
    for (const auto &p: _vPeopleReceivedPt) {
        p->debug();
    }
    
    
}
vector<double> Population::debug2(mt19937 &rng){
    //    Person * p=_vHouseholds[1]->getMember(1);
    //    p->setAge(30);
    vector<double> vec;
    //    for(int i=0;i<100000;i++){
    //        double v=p->returnTimeToFastProg(rng,false);
    //        if(v>0)    vec.push_back(v);
    //    }
    return vec;
}


//---------------------------------------------------
//PT scenarios
//---------------------------------------------------

void Population::update_pt_dynamics(mt19937 &rng){
    int n1=0;int n2=0;
    //loop over the popualtion and find those who just started PT and add them to our vector
    for (auto & p: _vCommunity){
        if (p->get_pt_startTime() ==TNOW()) {
            _vPeopleReceivedPt.push_back(p);
            n1++;
        }
    }
    //check time of PT end and get people off PT
    for (auto & p: _vPeopleReceivedPt){
        if(p->isOnPT() && p->get_pt_endTime()==TNOW()){
            p->getOffPT(rng);
            n2++;
        }}
}

vector<vector<long double>> Population:: return_pt_individualLevelOutputs(){
    vector<vector<long double>> vvRes;
    for (auto p:_vPeopleReceivedPt){
        vector<long double> vRes= p->return_pt_individualLevelOutputs() ;
        vvRes.push_back(vRes);
    }
    //clear this vector so that people who have died are killed
    _vPeopleReceivedPt.clear();
    
    return vvRes;
}
vector<long double> Population:: return_pt_annualOutputs() {
    auto vRes=returnBaseOutputs();
    int nOnPT=0;
    int nHasBeenOnPT=0;
    for (auto& p: _vCommunity){
        if (p->isOnPT())
            nOnPT++;
        if (p->hasBeenOnPT()) nHasBeenOnPT++;
    }
    //
    //    cout<<_stats->return_nPt_started()<<" "<<
    //    _stats->return_nPt_completed()<<" "<<
    //    nOnPT<<" "<<
    //    nHasBeenOnPT<<" "<<endl;
    
    vRes.push_back(_mp->getD("hhct_probADRcase_followup"));
    vRes.push_back(_mp->getD("hhct_testSens_atb"));
    vRes.push_back(_mp->getD("hhct_testSens_ltb"));
    vRes.push_back(_mp->getD("hhct_ptCoverage_0To5"));
    vRes.push_back(_mp->getD("hhct_ptCoverage_0To15"));
    vRes.push_back(_mp->getD("pt_regimen"));
    vRes.push_back(_mp->getD("pt_probSuccess_rec"));
    vRes.push_back(_mp->getD("pt_probSuccess_latent"));
    vRes.push_back(_mp->getD("pt_duration"));
    vRes.push_back(_mp->getD("pt_protAgainstReinfection"));
    //
    vRes.push_back(_stats->return_nHhct_hhScreened());
    vRes.push_back(_stats->return_nHhct_hhMembersScreened());
    vRes.push_back(_stats->return_nHhct_hhMembersLt6());
    vRes.push_back(_stats->return_nHhct_hhMembersLt16());
    vRes.push_back(_stats->return_nHhct_DS_atbDiagnosed());
    vRes.push_back(_stats->return_nHhct_DS_ltbDiagnosed());
    vRes.push_back(_stats->return_nHhct_DR_atbDiagnosed());
    vRes.push_back(_stats->return_nHhct_DR_ltbDiagnosed());
    vRes.push_back(_stats->return_nPt_started());
    vRes.push_back(_stats->return_nPt_completed());
    vRes.push_back(nOnPT);
    vRes.push_back(nHasBeenOnPT);
    vRes.push_back(_stats->return_nIncDS_onPt());
    vRes.push_back(_stats->return_nIncDS_postPt());
    vRes.push_back(_stats->return_nIncDR_onPt());
    vRes.push_back(_stats->return_nIncDR_postPt());
    //
    
    
    return vRes;
}
