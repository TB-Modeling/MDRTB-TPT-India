
//  Driver.cpp
//  src
//
//  Created by Trinity on 4/18/16.
//  Copyright © 2016 Trinity. All rights reserved.
//


#include "Driver.h"
#include "Population.h"
#include "Household.h"
#include "Person.h"
#include "Stats.h"
#include "Functions.h"
#include "Params.h"
#include "ThreadID.hpp"
#include "GlobalVariables.h"
#include "ihs.hpp"

#include <iostream>
#include <vector>
#include <array>
#include <ctgmath> //pow
#include <cmath>
#include <algorithm>
#include <random>
#include <array>
#include <memory>

//Household analysis
#include <unordered_map>

#include <fstream>  // serialization
#include <sstream>  // serialization
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>


using namespace std;
using std::logic_error;
using std::string;
using namespace globalVariables;
using namespace functions;

//---------------------------------------------------
//ERRORS
//---------------------------------------------------
string errPrefixDriver = "Unexpected Error\n";
string errSuffixDriver = "\n\nExiting simulation.";
string drtbWrongStartYearErr = errPrefixDriver +
"DRTB evolution began on incorrect year." + errSuffixDriver;
string wrongMcmcTimeline = errPrefixDriver +
"Mcmc target year is not set properly." + errSuffixDriver;

//---------------------------------------------------
//CLASS FUNCTIONS
//---------------------------------------------------
Driver::Driver(std::shared_ptr<Params> modelParams, unsigned seed)  {
    unsigned new_seed = std::chrono::system_clock::now().time_since_epoch().count();
    unsigned the_seed = seed == 0 ? new_seed : seed;
    _rng_seed = the_seed;
    rng.seed(_rng_seed);
    //create a SUS pop
    _mp=modelParams;
    _tID= make_ThreadID( { {"tID", 0u} } );
    
    /*
     * //Do we want to draw random variates from a distribution for certain params?
     *if (dist_draw)
     *    substitute_vars(rng, __V_PARAM_DIST_1, _mp);
     */
    
    pop= new Population(rng, _mp );
}
Driver::Driver(std::shared_ptr<ThreadID> tID, std::shared_ptr<Params> modelParams, unsigned seed) {
    unsigned new_seed = std::chrono::system_clock::now().time_since_epoch().count();
    unsigned the_seed = seed == 0 ? new_seed : seed;
    _rng_seed = the_seed;
    rng.seed(_rng_seed);
    cout << "Run " << tID->as_string() << ", seed : " << _rng_seed << endl;
    writeSeedFile(tID, _rng_seed);
    //create a SUS pop
    _mp=modelParams;
    _tID=tID;
    
    /*
     *
     * //Do we want to draw random variates from a dist distribution for certain params?
     *if (dist_draw)
     *    substitute_vars(rng, __V_PARAM_DIST_1, _mp);
     */
    
    pop= new Population(rng, _mp);
}
Driver::Driver( const Driver& src ): rng(src.rng),_tID(src._tID),_rng_seed(src._rng_seed),_mp(src._mp) {
    pop= new Population(*src.pop);
}
Driver::~Driver(){
    //delete all CSA members
    delete pop;
}

//---------------------------------------------------
//POPULATION ARCHIVE
//---------------------------------------------------
void Driver::writePopToFile(const string &filename){
    ofstream ofs( filename + ".gz", ios_base::out | ios_base::binary );
    stringstream ss;
    {
        boost::archive::text_oarchive oa(ss);
        oa << pop; //pop is a pointer to population.  Boost.Serialization automatically serializes the object referenced by pop and not the address of the object.
    }
    //zip the file
    boost::iostreams::filtering_streambuf<boost::iostreams::input> filter;
    filter.push( boost::iostreams::gzip_compressor() );
    //filter.push( boost::iostreams::bzip2_compressor() );
    filter.push(ss);
    boost::iostreams::copy( filter, ofs );
    if(__bPRINT) cout<<"Population written to from "<<filename<<endl;
}
void Driver::readPopFromFile(const string &filename) {    /* This will only work on files saved by writePopToFile() */
    
    if (pop != nullptr) {
        delete pop;
    }
    
    {
        ifstream ifs(filename );
        stringstream ss;
        boost::iostreams::filtering_streambuf<boost::iostreams::input> filter;
        //Unzip the file
        filter.push(boost::iostreams::gzip_decompressor());
        //filter.push( boost::iostreams::bzip2_decompressor() );
        filter.push(ifs);
        //
        boost::iostreams::copy( filter, ss );
        boost::archive::text_iarchive ia( ss );
        ia >> pop;
    }
    
    //Now we have to make sure that the Params object has been assigned properly
    //to the population, and that the values inside the Params object have been
    //properly distibuted to the Person class
    pop->readAllParams( _mp );
    if(__bPRINT) cout<<"Population read from "<<filename<<endl;
}
void Driver::loadPop(const string &filename) {    /* This will only work on files saved by writePopToFile() */
    
    if (pop != nullptr) {
        delete pop;
    }
    
    {
        cout<<"Loading population "<<filename<<" . . ."<<endl;
        ifstream ifs(filename );
        stringstream ss;
        boost::iostreams::filtering_streambuf<boost::iostreams::input> filter;
        //Unzip the file
        filter.push(boost::iostreams::gzip_decompressor());
        //filter.push( boost::iostreams::bzip2_decompressor() );
        filter.push(ifs);
        //
        boost::iostreams::copy( filter, ss );
        boost::archive::text_iarchive ia( ss );
        ia >> pop;
    }
    
    //The other version blots out the mp and tID from the loaded population
    //and substitutes it's own.  This functions uses those values from the loaded
    //population
    if(__bPRINT) cout<<"Population read from "<<filename<<endl;
}

//---------------------------------------------------
//ANNUAL MODEL RUN
//---------------------------------------------------
/** StepYear
 * Runs the population forward for aone year (12 months).
 */
void Driver::stepYear(){//models a whole year of Sim.
    for (int i=1;i<13;i++){
        
        pop->modelContacts(rng);
        pop->modelDiseaseProgress(rng);
        pop->modelHhDynamics(rng);
        pop->get_stats()->incTime();
    }
    pop->modelNaturalDynamics(rng);
    pop->get_stats()->incYear();
}
/** Burnin
 * Runs the population forward for a given period. At the end, resets the
 * stats and set the time back to 0.
 */
void Driver::burnin(int years) {
    for (int j=0;j<years;j++){
        for (int i=1;i<13;i++){
            pop->modelContacts(rng);
            pop->modelDiseaseProgress(rng);
            pop->modelHhDynamics(rng);
            pop->get_stats()->incTime();
        }
        pop->modelNaturalDynamics(rng);
        if(__bPRINT) cout<<"burnin "<<YNOW()<<endl;
        if (getPop()->get_stats()->return_nIncDS() > 1000 ) {
            //Incidence is too high; aborting run
            //Return with error code 100 to alert R code
            cout << "Aborting due to incidence over 1000" << endl;
            exit(100);
        }
        pop->get_stats()->incYear();
        pop->get_stats()->resetAnnualOutputs();
    }
}

//---------------------------------------------------
//CALIBRATION
//---------------------------------------------------
/*
 Function to run the model to year 1970 (calib stage1); save output/population in 1970; generate a second lhc at the end (for calibration stage2) and write to file
 */
void Driver::calibRunStage1(int burninDemog,//burnin durationf or demographics
                            int initInfections,//number of initial infections to seed the epidemic
                            int endYear, //year to end stage1
                            bool cull, //boolean flag to decide if cull happens
                            int cull_freq, //freq of cull (number of years between each cull)
                            int lhc2_size, //the size of second lhs
                            int lhc2_cols //the number of variables in the second lhs
){
    //Do the initial burnin
    burnin(burninDemog); //no outputs generated
    
    //seed the initial infections
    pop->initializeEpidemic(rng,initInfections);
    if(__bPRINT)        cout<<"Epidemic was initialized at time "<<pop->get_stats()->getTime()<<" with "<<initInfections<<" new cases of DSTB"<<endl;
    
    //First round of household dynamics
    pop->modelHhDynamics(rng);
    
    //recording TB outputs:
    vector<vector<long double>> vvAnnualOutputs;
    
    double cumMortality = 0;
    double cumPrevalence = 0;
    double cumIncidence = 0;
    //run model forward
    while(YNOW()<endYear) {
        
        //Population culling every cull_freq ------------------
        if (YNOW()<=1900 && cull) {
            //Culling happens until 1900
            //Culling happens every 'cull_freq' years
            if ((YNOW() % cull_freq) == 0) {
                pop->cullPop( rng );
            }
        }
        
        //Model one simjulated year ------------------
        for (int j=1;j<13;j++){
            pop->modelContacts(rng);
            pop->modelDiseaseProgress(rng);
            pop->modelHhDynamics(rng);
            pop->get_stats()->incTime();
        }
        pop->modelNaturalDynamics(rng, cull);
        
        if(__bPRINT) {
            cout<<"year "<<YNOW();
            cout<<", nIncDS = " << pop->get_stats()->return_nIncDS() << endl;
        }
        
        //Annaual outputs ------------------
        auto temp=pop->returnBaseOutputs();
        temp.push_back(pop->returnEarlyProgressionProp());
        temp.push_back(pop->returnLTBIPrev());
        temp.push_back(_rng_seed);/* Random seed */
        vvAnnualOutputs.push_back(temp);
        
        //===========================
        //Preemptive stop testing: check the DS incidence and stop models with excess DSTB ------------------
        //1.2. Reject if DS-TB incidence is more than 500 cases per 100,000 people (checked every 10 years between year 1500 and 1600, and every 100 years between 1600 and 1900.
        //3. Reject if DS-TB incidence is less than 100 cases per 100,000 people (checked in 1900)
        if (YNOW() < 1600 && YNOW() % 10 == 0) {
            const auto nIncDS = static_cast<double>(getPop()->_stats->return_nIncDS());
            const auto pop_size = getPop()->returnNumPeople();
            const auto pIncDS = nIncDS / pop_size * 100000; //check incidence rate
            if (pIncDS > __DS_INC_CUTOFF_HIGH) {
                //Exit the model early.
                cout << "Breaking Run Due to High DSTB incidence >500 in " << YNOW() << ": " << pIncDS << endl;
                break;
            }
        }
        if (YNOW() % 100 == 0) {            //Check for extreme values of pIncDS (>500)
            const auto nIncDS = static_cast<double>(getPop()->_stats->return_nIncDS());
            const auto pop_size = getPop()->returnNumPeople();
            const auto pIncDS = nIncDS / pop_size * 100000;
            if (pIncDS > __DS_INC_CUTOFF_HIGH) {
                //Exit the model early.
                cout << "Breaking Run Due to High DSTB incidence >500 in " << YNOW() << ": " << pIncDS << endl;
                break;
            }
            if (YNOW() == 1900) {   //Check for extreme values of pIncDS (<100)
                //Lower limit only applies in 1900
                if (pIncDS < __DS_INC_CUTOFF_LOW ) {
                    cout << "Breaking Run Due to High DSTB incidence <100 in " << YNOW() << ": " << pIncDS << endl;
                    break;
                }
            }
        }
        //Prevent the code from excuting on the first year, 1500
        //4. Reject if the ratio of TB mortality to TB incidence is >66%
        //  (checked in 1600 , and every 100 years between year 1600 to 1900)
        //5. Reject if the ratio of TB prevalence to annual TB incidence is <0.5
        //  (checked in 1600 , and every 100 years between year 1600 to 1900)
        //Both these checks are averaged out over 5 years (??96,??97,??98,??99,??00)
                
        bool preemptive_exit = false;
        
        switch (YNOW() % 100) {
            case 96:
                cumMortality = static_cast<double>(getPop()->_stats->return_nMortalityDS());
                cumPrevalence = static_cast<double>(pop->returnPrevalence_DSA());
                cumIncidence = static_cast<double>(getPop()->_stats->return_nIncDS());
                break;
            case 97:
            case 98:
            case 99:
                //Year ??97
                //Year ??98
                //Year ??99
                cumMortality += static_cast<double>(getPop()->_stats->return_nMortalityDS());
                cumIncidence += static_cast<double>(getPop()->_stats->return_nIncDS());
                cumPrevalence += static_cast<double>(pop->returnPrevalence_DSA());
                break;
            case 0:
                if (YNOW() > 1500) {
                    //Year ??00
                    cumMortality += static_cast<double>(getPop()->_stats->return_nMortalityDS());
                    cumIncidence += static_cast<double>(getPop()->_stats->return_nIncDS());
                    cumPrevalence += static_cast<double>(pop->returnPrevalence_DSA());
                    //Do the checking!
                    if ( (cumMortality / cumIncidence) > __DS_MORT_INC_CUTOFF ) {
                        cout << "Breaking Run Due to High case fatality in " << YNOW() << ": " << (cumMortality / cumIncidence) << endl;
                        preemptive_exit = true;
                    }
                    if ((cumPrevalence / cumIncidence ) < __DS_PREV_INC_CUTOFF ) {
                        cout << "Breaking Run Due to High prev to inc ratio in " << YNOW() << ": " << (cumPrevalence / cumIncidence ) << endl;
                        preemptive_exit = true;
                    }
                }
                break;
            default:
                break;
        }
        
        if (preemptive_exit) {
            //One of the 5 year average checks failed above, exit.
            break;
        }
        
        
        //===========================
        pop->get_stats()->incYear();
        pop->get_stats()->resetAllOutputs(); //tally and annual
               
    }
    
    
    //After the model is run to 1970, save the population to a file, and then generate a lhc2 for stage2 calibration
    //Do not save the population if we break above, which would be before year 1900
    

    if (YNOW() != endYear) {
        //write out the outputs so we can examine the run, complete or incomplete
        writeVector(_tID, vvAnnualOutputs, "stage1/out_stage1_incomplete");
    } else {
        //Complete Run
        //Write the data vector
        writeVector(_tID, vvAnnualOutputs, "stage1/out_stage1_complete");
        //Write the population
        stringstream fname;
        fname << "stage1_pop_"<<_tID->as_string()<<"_"<<YNOW();
        writePopToFile(fname.str());
        if(__bPRINT)            cout<<"pop saved at year "<<YNOW()<<endl;
        
        //--------------------------------------------------------------------------
        //Write the stage2 hypercube
        //Run with a count of stage2_count
        const auto ihs_dim = lhc2_cols;
        const auto ihs_runcount = lhc2_size;
        //const auto ihs_runcount = 1;
        const auto ihs_duplication = 5; //a fixed factor for the lhs algoritm
        
        //This might not be the best seed; could get the clock again or generate a random number with
        //std::random_device.  Will be fine for now.
        auto ihs_seed = ihs::get_seed();
        
        auto ihs_result = ihs::ihs( ihs_dim, ihs_runcount, ihs_duplication, ihs_seed ); //1D vector
        int stage2_index = 0;
        //unwrapping ihs_result into a 2D vector:
        vector<vector<int>> stage2_hc;
        for (auto ihs_index = 0; ihs_index < ihs_runcount; ihs_index ++) {
            stage2_hc.push_back(vector<int>());
            for (auto j = 0; j < ihs_dim; j++) {
                stage2_hc[ihs_index].push_back(ihs_result[stage2_index++]);
            }
        }
        writeVector(_tID, stage2_hc, "stage2_hc", true, false);
        //--------------------------------------------------------------------------
        
    }
    
}
/*
 Function to run the model to year 2020 (calib stage2);
 */
void Driver::calibRunStage2(int endYear,
                            vector<vector<long double>> initialOutputs,
                            bool save_pop
                            ) {
    
    //Everything is set; begin the run
    double pRetreatmentDR_total = 0.0;
    
    auto runData=initialOutputs;
    
    while(YNOW() <endYear) {
        //Model dynamic changes in parameter values --------------------
        if (YNOW()>=2000) {
            // Model reductions in transmission and reactivation rate post year 2000
            modelParamChangesPost2000();
            
            if(YNOW()==2000 && __bPRINT)
                cout << "Year 2000; dowscaling variables\n";
        }
        
        
        //Model simulated year ---------------------
        for (int j=1;j<13;j++){
            pop->modelContacts(rng);
            pop->modelDiseaseProgress(rng);
            pop->modelHhDynamics(rng);
            pop->get_stats()->incTime();
        }
        //pop->modelNaturalDynamics(rng, cull);
        pop->modelNaturalDynamics(rng, true);
        //Annaual outputs ---------------------
        auto temp=pop->returnBaseOutputs();
        temp.push_back(pop->returnEarlyProgressionProp());
        temp.push_back(pop->returnLTBIPrev());
        temp.push_back(_rng_seed);/* Random seed */
        runData.push_back(temp);
        
        //DR Preemptive stop testing ---------------------
        //3. Reject if proportion of retreatment TB cases that have DR-TB (1996-2000) is more than 40%
        //(checked in 2000)
        if (YNOW() > 1995 && YNOW() <= 2000) {
            //5 Years: 1996, 1997, 1998, 1999, 2000
            //Check for extreme values of DR retreatment ( > pRetreatmentDR_cutoff )
            pRetreatmentDR_total += static_cast<double>(pop->get_stats()->return_nRetreatedDRTB()) /
            static_cast<double>(pop->get_stats()->return_nRetreatedPatient());
            if (YNOW() == 2000) {
                const auto pRetreatmentDR = pRetreatmentDR_total / 5.0;
                if (pRetreatmentDR > __DR_PRETREATMENT_CUTOFF) {
                    cout << "Breaking Run Doe to Abnormal DR Retreatment ( " <<  pRetreatmentDR << " )" << endl;
                    
                    //Early exit due to pRetreatmentDR being too high
                    writeVector(_tID, runData, "stage2/out_stage2_incomplete");
                    
                    break;
                }
            }
        }
        pop->get_stats()->incYear();
        pop->get_stats()->resetAllOutputs(); //tally and annual
    }
    
    //Normal Exit
    if (YNOW()>2000){
        writeVector(_tID, runData, "stage2/out_stage2_complete");
        //Save pop?
        if (save_pop) {
            stringstream fname;
            fname << "stage2_pop_"<<_tID ->as_string() <<"_"<<YNOW();
            writePopToFile(fname.str());
        }
    }
}


//---------------------------------------------------
//PT SCENARIOS
//---------------------------------------------------

/** runPtScenario
 Function to get the population in 1970 and run it forward under a given pt scenario
 */
void Driver::runPtScenario( const int ptStartYear,
                           const int ptEndYear,
                           const int pt_individualOutcomeYear,
                           const int finalSimYear,
                           const bool saveEndPopulation){
    
    vector<vector<long double>> vvAnnualOutputs; //vector of annual outputs colelcted to the end
    vector<vector<long double>>  vvIndividualLevelOutputs; //vector of individual level outputs collected in pt_individualOutcomeYear
    
    double hhct_probADRcase_followup0= _mp->getD("hhct_probADRcase_followup");
    cout<<"*******************************************"<<endl;
    
    // before PT starts: --------------------------------------------------------------------------------
    
    //run DR model to pt start date:
    int yearsToRun=ptStartYear-YNOW();
    if (yearsToRun>0){
        _mp->setD("hhct_probADRcase_followup", 0);
        pop->readAllParams(_mp);
        if (__bPRINT) cout<<"Year "<<YNOW()<<": tunring off HHCT till year 2020"<<endl;
        //
        for (int i=0;i<yearsToRun;i++) {
            // Model reductions in transmission and reactivation rate post year 2000
            modelParamChangesPost2000();
            
            for (int j=1;j<13;j++){
                pop->modelContacts(rng);
                pop->modelDiseaseProgress(rng);
                pop->modelHhDynamics(rng);
                pop->get_stats()->incTime();
            }
            pop->modelNaturalDynamics(rng);
            if(__bPRINT) cout<<"year "<<YNOW()<<endl;
            /* Saving annaual outputs */
            vector<long double> temp=pop->return_pt_annualOutputs();
            temp.push_back(pop->returnEarlyProgressionProp());
            temp.push_back(pop->returnLTBIPrev());
            temp.push_back(_rng_seed);/* Random seed */
            vvAnnualOutputs.push_back(temp);
            /* Resetting Stats */
            pop->get_stats()->incYear();
            pop->get_stats()->resetAllOutputs();
        }}
    
    //During pt:  ------------------------------------------------------------------------------------
    //update PT parameters and start PT implementation
    if(YNOW()==ptStartYear ){
        _mp->setD("hhct_probADRcase_followup", hhct_probADRcase_followup0);
        pop->readAllParams(_mp);
        if (__bPRINT) cout<<"Year "<<YNOW()<<" : Starting HHCT/PT implementation till "<<ptEndYear<<endl;
    }
    
    //run the model to the end of PT (2025)
    yearsToRun=ptEndYear-ptStartYear;
    for (int i=0;i<yearsToRun;i++) {
        // Model reductions in transmission and reactivation rate post year 2000
        modelParamChangesPost2000();
        
        for (int j=1;j<13;j++){
            pop->modelContacts(rng);
            pop->modelDiseaseProgress(rng);
            pop->modelHhDynamics(rng);
            //*******
            pop->update_pt_dynamics(rng);//run this every month
            //*******
            pop->get_stats()->incTime();
        }
        pop->modelNaturalDynamics(rng);
        if(__bPRINT) cout<<"year "<<YNOW()<<endl;
        /* Saving annaual outputs */
        vector<long double> temp=pop->return_pt_annualOutputs();
        temp.push_back(pop->returnEarlyProgressionProp());
        temp.push_back(pop->returnLTBIPrev());
        temp.push_back(_rng_seed);/* Random seed */
        vvAnnualOutputs.push_back(temp);
        /* Resetting Stats */
        pop->get_stats()->incYear();
        pop->get_stats()->resetAllOutputs();
    }
    
    
    //update PT parameters and stop PT implementation
    if(YNOW()==ptEndYear){
        _mp->setD("hhct_probADRcase_followup", 0);
        pop->readAllParams(_mp);
        if (__bPRINT)  cout<<"Year "<<YNOW()<<" : End of HHCT/PT implementation "<<endl;
    }
    
    // After PT: ------------------------------------------------------------------------------------
    yearsToRun=finalSimYear-YNOW();
    for (int i=0;i<yearsToRun;i++) {
        // Model reductions in transmission and reactivation rate post year 2000
        modelParamChangesPost2000();
        
        for (int j=1;j<13;j++){
            pop->modelContacts(rng);
            pop->modelDiseaseProgress(rng);
            pop->modelHhDynamics(rng);
            //*******
            if (i==0) pop->update_pt_dynamics(rng);//run this only forone year post PT to make sure we get people off PT
            //*******
            pop->get_stats()->incTime();
        }
        pop->modelNaturalDynamics(rng);
        if(__bPRINT) cout<<"year "<<YNOW()<<endl;
        /* Saving annaual outputs */
        vector<long double> temp=pop->return_pt_annualOutputs();
        temp.push_back(pop->returnEarlyProgressionProp());
        temp.push_back(pop->returnLTBIPrev());
        temp.push_back(_rng_seed);/* Random seed */
        vvAnnualOutputs.push_back(temp);
        //****************************************
        if (YNOW()==pt_individualOutcomeYear){
            vvIndividualLevelOutputs= pop->return_pt_individualLevelOutputs();
        }
        /* Resetting Stats */
        pop->get_stats()->incYear();
        pop->get_stats()->resetAllOutputs();
    }
    //--------------------------
    //save annual outputs
    writeVector(_tID,vvAnnualOutputs, "pt_sa/out_pt_annualOutputs", true);
    if (__bPRINT) cout<<"annual pt outputs generated"<<endl;
    writeVector(_tID,vvIndividualLevelOutputs, "pt_sa/out_pt_individualOutputs", true);
    if (__bPRINT)  cout<<"individual pt outputs generated"<<endl;
    //
    if (saveEndPopulation){
        stringstream fname;fname<<"pt_pop_"<<_tID->as_string();
        writePopToFile(fname.str());
    }
}

/* Model reductions in transmission and reactivation rate post year 2000 */
void Driver::modelParamChangesPost2000(){
    //coefFastProg
    //via coefReactReducPost2000
    {
        const auto reducCoef = _mp->getD("coefReactReducPost2000");
        auto coefVal = _mp->getD("coefFastProg");
        coefVal *= 1.0 - reducCoef;
        _mp->setD("coefFastProg", coefVal);
    }
    //coefHHTrans + coefComTrans
    //via coefTransReducPost2000
    {
        const auto reducCoef = _mp->getD("coefTransReducPost2000");
        auto hhcoefVal = _mp->getD("coefHHTrans");
        auto comcoefVal = _mp->getD("coefComTrans");
        hhcoefVal *= 1.0 - reducCoef;
        comcoefVal *= 1.0 - reducCoef;
        _mp->setD("coefHHTrans", hhcoefVal);
        _mp->setD("coefComTrans", comcoefVal);
    }
    //probSlowProg
    //via coefReactReducPost2000
    {
        const auto reducCoef = _mp->getD("coefReactReducPost2000");
        auto coefVal = _mp->getD("probSlowProg");
        coefVal *= 1.0 - reducCoef;
        _mp->setD("probSlowProg", coefVal);
    }
    //Load these new values into the members of the population
    pop->readAllParams(_mp);
    pop->recalculateSlowProg(rng);//Recalculate all slowProgress values if -1
    if(__bPRINT)  cout << "Dowscaling variables\n";
}
