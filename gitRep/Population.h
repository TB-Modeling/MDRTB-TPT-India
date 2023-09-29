//
//  Population.h
//  src
//
//  Created by Trinity on 8/4/15.
//  Copyright (c) 2015 Trinity. All rights reserved.
//

#ifndef __mdrtb__Population__
#define __mdrtb__Population__

#include "Household.h"
#include "Person.h"
#include "Params.h"
#include "Stats.h"
#include "ThreadID.hpp" 


#include <cstdio>
#include <vector>
#include <random>
#include <memory>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;


class Population{
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _vCommunity;
        ar & _vHouseholds;
        ar & _stats;
        ar & _mp;
        ar & _vPeopleReceivedPt;
    }
    //-------------------------------------------------------------------------------
    // Members
    //-------------------------------------------------------------------------------
public:
    vector<std::shared_ptr<Person>> _vCommunity; //vector houseing all community members
    vector<std::shared_ptr<Household>> _vHouseholds;//vector housing temporary households
    std::shared_ptr<Stats> _stats;
    std::shared_ptr<Params> _mp;
    vector<std::shared_ptr<Person>> _vPeopleReceivedPt; //vector houseing all those who received PT 

    //---------------------------------------------------
    //Class functions
    //---------------------------------------------------
    Population();
    Population(mt19937 &rng, std::shared_ptr<Params> mp);
    ~Population();
    
    //---------------------------------------------------
    //Serialization
    //---------------------------------------------------
    Population(std::string filename); //Serialization Constructor: makes a new population from this file
    bool compare(Population * p); //comapres this person with p to make sure all attributes are the same
    bool operator != (Population* p) {
        return !compare(p);
    }
   
    //---------------------------------------------------
    //Helper functions
    //---------------------------------------------------
    void readAllParams(std::shared_ptr<Params> mp);//reads parameter values from the mp file
    inline int TNOW(){return _stats->getTime();}
    inline int YNOW(){return _stats->getYear();}
    std::shared_ptr<Stats> get_stats() const {return _stats;}
    std::shared_ptr<Params> get_mp() const {return _mp;}
    std::shared_ptr<Person> returnPerson(const int i) const;
    std::shared_ptr<Household> operator [](const int i) const {return _vHouseholds[i];}
    vector<std::shared_ptr<Household>>::iterator begin()   {     return _vHouseholds.begin();}
    vector<std::shared_ptr<Household>>::iterator end()   {     return _vHouseholds.end();}
    void initializeEpidemic(mt19937 &rng, const int prevalence);
    void initializeEpidemic(mt19937 &rng, const int ELDS, const int LLDS, const int ADS, const int REC) ;
   
    void recalculateSlowProg(mt19937 &rng);
    //---------------------------------------------------
    //Annual dynamics
    //---------------------------------------------------
    void modelNaturalDynamics(mt19937 &rng, bool cull = false);////models natural dynamic processes among Households
    void modelBirths(mt19937 &rng, const int nNaturalDeaths);
    void modelDiseaseProgress(mt19937 &rng);
    void modelContacts(mt19937 &rng);
    void modelHhDynamics(mt19937 &rng);
    void cullPop( std::mt19937& );
    
    //---------------------------------------------------
    //Output colelction
    //---------------------------------------------------
    unsigned returnNumPeople() const;
    vector<double> returnDistPopAgeBucket(const bool includeMale=true, const bool includeFemale=true) const;
    double returnMedianPopAge() const;
    vector<double> returnDistPopGend() const;
    int returnNumHouseholds() const; //return number of existing households
    int  returnNumPeopleInHouseholds() const;//return number of people who are in households
    double returnMedianHhSize() const;
    vector<int> returnDistHhSize() const; //number of households of size 0 ... N
    vector<vector<int>> returnDistHhAge (const unsigned hhSize) const;//return age dist of people in households of size X (dim 1 is age, dim 2 is the number of HHs that fall in this category)
    vector<vector<vector<int>>>  returnDistHhAge_allHhSizes () const;//return age dist of people in households of size X (dim 1 is hhSize, dim 2 is age, dim 3 is the number of HHs that fall in this category)
    std::size_t getMaxHHSize() const;
    vector<vector<double>> returnDistStateSizes_DSDR() const;
    vector<double>  returnDistStateSizes_DSDR_oneline() const;
    vector<double> returnDistStateSizes_DS() const;
    vector<double> returnDistStateSizes_DR() const;
    int returnPrevalence_DSA() const;
    int returnPrevalence_DRA() const;
    double returnEarlyProgressionProp();
    double returnLTBIPrev();
    vector<std::shared_ptr<Person>> popNoHouseholdNotDead();
    vector<long double> returnBaseOutputs();
  
    //---------------------------------------------------
    //DEBUG
    //---------------------------------------------------
    inline int getYear(){return _stats->getYear();}
    void debug1();
    vector<double> debug2(mt19937 &rng);
    inline std::size_t getPTVecLen() const { return _vPeopleReceivedPt.size(); }
    
    //---------------------------------------------------
    //PT scenarios
    //---------------------------------------------------
    void update_pt_dynamics(mt19937 &rng);
    vector<vector<long double>>  return_pt_individualLevelOutputs();
    vector<long double>  return_pt_annualOutputs();
    
    vector<double> returnDemogOutputs();

};



#endif /* defined(__mdrtb__Population__) */
