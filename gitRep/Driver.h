//  Driver.h
//  src
//
//  Created by Trinity on 4/18/16.
//  Copyright © 2016 Trinity. All rights reserved.
//

#ifndef __mdrtb__Driver__
#define __mdrtb__Driver__

#include "Population.h"
#include "Functions.h"
#include "Person.h"
#include "Stats.h"
#include "ThreadID.hpp"

#include <cstdio>
#include <vector>
#include <sstream>
#include <random>
#include <memory>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

using namespace std; //for std::vector
using namespace functions;


//calibration measures:

class Driver{
private:
    Population * pop;
    std::mt19937 rng;
    std::shared_ptr<ThreadID> _tID;//thread id (a unique identifier for each model replication); used as suffix for outputs
    unsigned int _rng_seed;
    std::shared_ptr<Params> _mp;
public:
    //---------------------------------------------------
    //CLASS FUNCTIONS
    //---------------------------------------------------
    //auto d = Driver(tID,  mp, 0, beta_flag);
    Driver(std::shared_ptr<Params> modelParams, unsigned seed = 0);
    Driver(std::shared_ptr<ThreadID> tID, std::shared_ptr<Params> modelParams, unsigned seed = 0);//for initializing a random rng based on thread
    Driver( const Driver& src ) ;
    ~Driver();
    
    //---------------------------------------------------
    //HELPER FUNCTIONS
    //---------------------------------------------------
    inline Population * getPop(){return pop;}
    std::shared_ptr<Params> get_mp() const {return _mp;}
    void initilizeEpidemic(int EL,int LL,int A, int REC);
    int getPopSize() const { return pop->returnNumPeople(); }
    unsigned int getRngSeed() const { return _rng_seed; }
    inline int YNOW(){return pop->get_stats()->getYear();}
    inline int TNOW(){return pop->get_stats()->getTime();}
    void swap_mp(std::shared_ptr<Params> new_mp){
        _mp.reset();
        _mp = new_mp;
    }
    
    //---------------------------------------------------
    //POPULATION ARCHIVE
    //---------------------------------------------------
    void writePopToFile(const string &filename); //saves a copy of the population to a text file
    void readPopFromFile(const string &filename); //loads population from a text file
    void loadPop(const string &filename); //loads population from a text file, keeps mp and tID from file.
    
    //---------------------------------------------------
    //ANNUAL RUN
    //---------------------------------------------------
    void stepYear();
    void burnin(int years);
    
    void modelParamChangesPost2000();
    //---------------------------------------------------
    //CALIBRATION
    //---------------------------------------------------
    void calibRunStage1(int burninDemog,
                      int initInfections,
                      int endYear,
                      bool cull,
                      int cull_freq,
                      int lhc2_size, 
                      int lhc2_cols );
    
    void calibRunStage2(int endYear,
                      vector<vector<long double>> initialOutputs,
                      bool save_pop);
    
    //---------------------------------------------------
    //PT SCENARIOS
    //---------------------------------------------------
    void runPtScenario( const int ptStartYear,
                       const int ptEndYear,
                       const int pt_individualOutcomeYear,
                       const int finalSimYear,
                       const bool saveEndPopulation);
    
};



#endif /* Driver_h */
