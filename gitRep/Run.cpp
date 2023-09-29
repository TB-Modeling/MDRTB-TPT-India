//
//  run.cpp
//  mdrtb
//
//  Created by Trinity on 7/18/19.
//  Copyright © 2019 Trinity. All rights reserved.
//
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <utility>
#include <random>
#include <chrono>

#include "Driver.h"
#include "Functions.h"
#include "Params.h"
#include "ThreadID.hpp"

//#include "mdrtb_lhc_13.hpp" //50,000 (run #13)  //April 22/2022
#include "mdrtb_lhc_14.hpp" //10,000 LHC for run 14 June 27/2022

#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/normal.hpp>

using namespace std;
using std::string;
using std::cout;
using namespace globalVariables;
using namespace functions;


/*
 Function to start the initial model and run it to year 1970 (calibration stage1),
 while implementing the "cull" procedure on a given interval
 */
void runDsPhase(const shared_ptr<ThreadID> tID, // unique replication number
                const int lhc1_size, //the size of LHS1
                const int lhc2_size, //the size of LHS2
                const int lhc2_cols, //the number of varibles in LHS2
                std::shared_ptr<Params> mp, //param file
                int startYear,// starting year (1500)
                int yearsBurninDemog, //bur in for demographics (0 at baseline)
                int initialInfections, //number of initial TB infections to seed the model
                int endYear, //years to end stage1
                bool cull, //0: no cull (allo for population growth)
                int cull_freq // frequency of cull
) {
    //Sanity check: //Sanity check; make sure that stage1_params_count and lhc1_width are equal before continuing.
    const auto stage1_params_count = globalVariables::__V_PARAM_DIST_1.size();
    const auto lhc_width = sizeof(mdrtb_lhc) / lhc1_size / sizeof(double);
    assert ( stage1_params_count == lhc_width );
    
    //The tID will correspond to a row in the hypercube; we need to get the information on this row
    //and set the values in mp.
    auto row_index = tID->get_index(0);
    cout << "Hypercube row " << row_index << endl;
    
    //The values in the hypercube start at 1 and end at n
    //Use the vmap function to properly map the values to the ranges.
        
    //Beta/Lognormal/Uniform Distributed parameters:
    auto lhc_index = 0;
    for (const auto &entry : globalVariables::__V_PARAM_DIST_1) {
        switch (entry.dt) {
            case globalVariables::DistType::UNIFORM:
            {
                //Evenly distribute the value
                mp->setD(entry.name,
                         vmap(mdrtb_lhc[row_index][lhc_index],
                              1,
                              lhc1_size,
                              entry.shape1,
                              entry.shape2)
                         /entry.scale);
                break;
            }
            case globalVariables::DistType::BETA:
            {
                const auto downscale = vmap(
                                            mdrtb_lhc[row_index][lhc_index],
                                            1,
                                            lhc1_size,
                                            almost_zero,
                                            almost_one
                                            );
                boost::math::beta_distribution<double> dist (entry.shape1, entry.shape2);
                mp->setD(entry.name, quantile(dist, downscale) / entry.scale);
                break;
            }
            case globalVariables::DistType::LOGNORMAL:
            {
                const auto downscale = vmap(
                                            mdrtb_lhc[row_index][lhc_index],
                                            1,
                                            lhc1_size,
                                            almost_zero,
                                            almost_one
                                            );
                boost::math::normal_distribution<double> dist(entry.shape1, entry.shape2);
                mp->setD(entry.name, exp(quantile(dist, downscale) / entry.scale));
                break;
            }
        }
        lhc_index++;
    }
    //The variables are all now set, we are ready to proceed to the run.
    cout<<"All parameter values are set. Ready to launch the simulation."<<endl;

    auto d = Driver(tID,  mp, 0);
    
    //Set the population to the proper year
    d.getPop()->get_stats()->setYear(startYear);
    
    //run the model to year 1970 with proper checks and create lhc2 at the end if successul
    d.calibRunStage1(yearsBurninDemog, initialInfections, endYear, cull, cull_freq,lhc2_size,lhc2_cols);
    
}
/*
 Function to read back populations in year 1970 and run a simulation to 2020
 */
void runDrPhase(const shared_ptr<ThreadID> tID, // ID of the current run (corresponding to initial LHS row; range 0-10k)
                const int lhc2_size,
                const int endYear, //2022
                 const std::string pop_filename, //The filename from which to load the population
                 const vector<int> stage2_line,//The hypercube values for stage2
                 vector<vector<long double>> initialOutputs //The initial outputs from the run that are passed on to the new run
) {
   
    
    cout << "Starting DR run " << tID->as_string() << endl;
    
    auto mp = std::make_shared<Params>("seedConfig.cfg");//read the param file
    
    auto d = Driver(tID, mp, 0);//start a new Driver object
    //0 as rng seed will pick a seed using the clock as above
    d.loadPop(pop_filename);
    
    //Seting up parameter values from the second LHS :
    //Implement another sanity check to make sure that the hypercube matches with the number of parameters
    const auto hc_width = stage2_line.size();
    const auto stage2_param_count = globalVariables::__V_PARAM_DIST_2.size();
    assert ( hc_width == stage2_param_count );
    
    auto hc_index = 0;
    std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
    for (const auto &entry : globalVariables::__V_PARAM_DIST_2) {
        switch (entry.dt) {
            case globalVariables::DistType::BETA:
            {
                //We have a different logic here that maybe unnecessary but we keep it for now:
                //we are resampling the random value from each bin of the hypercube everytime we call it.
                //No longer using vmap here; we are using the stage2 lhc spacing as bins,
                //then picking uniform random numbers from within the outer values of the bin.
                const auto uni_vals = getUniValues(stage2_line[hc_index],
                                                   lhc2_size,
                                                   almost_zero,
                                                   almost_one);
                std::uniform_real_distribution<double> uni (uni_vals.first,uni_vals.second);
                
                boost::math::beta_distribution<double> dist (entry.shape1, entry.shape2);
                d.getPop()->_mp->setD(entry.name, quantile(dist, uni(rng)) / entry.scale);
                break;
            }
            case globalVariables::DistType::UNIFORM:
            {
                //Uniformly set variables
                const auto uni_vals = getUniValues(stage2_line[hc_index],
                                                   lhc2_size,
                                                   almost_zero,
                                                   almost_one);
                std::uniform_real_distribution<double> uni (uni_vals.first,uni_vals.second);
                const auto variate = vmap(uni(rng),
                                          almost_zero,
                                          almost_one,
                                          entry.shape1,
                                          entry.shape2);
                d.getPop()->_mp->setD(entry.name, variate / entry.scale);
                break;
            }
            default:
                cout << "Should never reach here; all DR distributions are of type BETA or UNIFORM" << endl;
        }
        hc_index++;
    }
    
    cout<<"Parameter values are read for the second LHS. Ready to launch the simulation."<<endl;
    //------------------------------------------------------------------------------------------------------------------------------------
    //Set the driver mp to the population mp.
    d.swap_mp(d.getPop()->_mp); //make sure the driver has the same mp object as the pop
    
    //Make sure the population has all the right parameters
    d.getPop()->readAllParams(d.getPop()->_mp);
    
    //run the model forward
    d.calibRunStage2(endYear, initialOutputs, false);
}

/*
 Function to save populations for good-fitting models after stage2 calibration
 Reads back populations in year 1970 and run a simulation to endYear=2022, then saves the population
 Reads parameters from previous run instead of sampling
 */
void runDrPhase_savePop (const shared_ptr<ThreadID> tID,
                     const int endYear, //The length of the restart (from 1971 to 2020, so 51 years)
                     const std::string pop_filename, //The filename from which to load the population
                     const vector<int> stage2_line,//The hypercube values for stage2
                     vector<vector<long double>> initialOutputs //The initial outputs from the run that are passed on to the new run
) {
    
    cout << "Starting PopGen " << tID->as_string() << endl;
    
    auto mp = std::make_shared<Params>("seedConfig.cfg");//read the param file
    
    const auto runID = tID->get_index(0); //DS ID
    const auto hc_ID = tID->get_index(1); // DR ID
    stringstream stage2_output;
    stringstream stage2_seed;
    stage2_output << "stage2/out_stage2_complete_" << runID << "_" << hc_ID;
    stage2_seed << "seed_" << runID << "_" << hc_ID;
    //
    const auto stage2_file = readVector<double>(stage2_output.str());
    const auto stage2_seed_file = readVector<unsigned>(stage2_seed.str());
    const auto final_row = stage2_file[stage2_file.size() - 1];
    auto rng_seed = 0u;
    rng_seed = stage2_seed_file[0][0]; //Overwrite the rng_seed
    
    const double probResistance = final_row[61];
    const double coefMaxInfect_DR_RelTo_DS = final_row[62];
    const double coefTransReducPost2000 = final_row[63];
    const double coefReactReducPost2000 = final_row[64];
    //Missing 4 values

    const double probDRTrtFailure = final_row[68];
    const double probPretreatmentLTF_DRTB = final_row[67];
    const double probRelapse_DR = final_row[71];
    const double probSuccessDStrt_DRTB = final_row[76];
    
    
    auto d = Driver(tID, mp, rng_seed);//start a new Driver object
    d.loadPop(pop_filename);
    
    //We are entering the values from the previous run, not generating new ones
    //--------POP GENERATION--------
    d.getPop()->_mp->setD("probResistance", probResistance);
    d.getPop()->_mp->setD("coefMaxInfect_DR_RelTo_DS", coefMaxInfect_DR_RelTo_DS);
    d.getPop()->_mp->setD("coefTransReducPost2000", coefTransReducPost2000);
    d.getPop()->_mp->setD("coefReactReducPost2000", coefReactReducPost2000);
    d.getPop()->_mp->setD("probDRTrtFailure",probDRTrtFailure);
    d.getPop()->_mp->setD("probPretreatmentLTF_DRTB",probPretreatmentLTF_DRTB);
    d.getPop()->_mp->setD("probRelapse_DR",probRelapse_DR);
    d.getPop()->_mp->setD("probSuccessDStrt_DRTB",probSuccessDStrt_DRTB);
   
    cout<<"DR stage parameters are read from the previous run's config file, ready to generate population"<<endl;
    //------------------------------------------------------------------------------------------------------------------------------------
    //Set the driver mp to the population mp.
    d.swap_mp(d.getPop()->_mp); //make sure the driver has the same mp object as the pop
    
    //Make sure the population has all the right parameters
    d.getPop()->readAllParams(d.getPop()->_mp);
    
    //run the model forward
    d.calibRunStage2(endYear, initialOutputs, true);
}
//---------------------------------------------------
//PT SCENARIOS
//---------------------------------------------------

/*
 Method to read population saved in year 2020 and run a PT scenario
 */
void runPT_2020Forward(const shared_ptr<ThreadID> tID,  
                       const int ptScenario,// 0:no pt, 1:Isonizid; 2:Delamanid
                       const int ptStartYear,
                       const int ptEndYear,
                       const int pt_individualOutcomeYear,
                       const int finalSimYear,
                       const string pop_filename, //population saved in 2020
                       const unsigned seed
                       ) {
    
    
    //We need an mp file; load the default config.
    auto localmp = std::make_shared<Params>("seedConfig.cfg");//just a temp copy to start the driver object
    
    
    //Start a new Driver with random seed
    auto d = Driver(tID, localmp, seed);
    
    
    //Load the population in 2020
    d.loadPop(pop_filename);
    cout<<"Population read in year "<<d.getPop()->YNOW()<<endl; //begging of the year
    
    //the population mp includes the value of all lhc1&lhc2 parameters, but not the pt parameters
    //copy population's mp file to driver and update pt parameters from the localmp file
    d.swap_mp(d.getPop()->get_mp());
    
    
    //Update parameters ******************
    //read all pt params from file (popualtion copy maybe outdated)
    vector<string> varNames= {
        "hhct_probADRcase_followup",
        "hhct_testSens_atb",
        "hhct_testSens_ltb",
        "hhct_ptCoverage_0To5",
        "hhct_ptCoverage_0To15",
        "pt_probSuccess_rec",
        "pt_probSuccess_latent",
        "pt_duration",
        "pt_protAgainstReinfection"};
    for(auto var:varNames){
        d.get_mp()->setD(var, localmp->getD(var));
    }
    
    //Scenario  0,1,2,3 : only change pt_regimen
    d.get_mp()->setD("pt_regimen",ptScenario);
    
    //For Baseline, turn off contact tracing too
    if(ptScenario==3){
        d.get_mp()->setD("hhct_probADRcase_followup",0);
    }
    //*********************************
    
    //once all parameters are updated, we update them in the population and for all people
    d.getPop()->readAllParams(d.get_mp());
    
    //run PT scenarios
    d.runPtScenario( ptStartYear,ptEndYear,pt_individualOutcomeYear,finalSimYear,  false );
}

void runPTSA_2020Forward(const shared_ptr<ThreadID> tID, //iD of the current run
                         const int ptStartYear,
                         const int ptEndYear,
                         const int pt_individualOutcomeYear,
                         const int finalSimYear,
                         const string pop_filename, //population saved in 2020
                         const unsigned seed
                         ) {
    //Run the sensitivity analysis for PT
    //  To do this, we will run 100 reps of the Delaminid PT scenario, sampling 6 variables between
    //  values of 50% and 100%.  These variables are:
    //
    //   0.   Probability of household contact tracing for DR-TB patients receiving treatment
    //          Currently : 70%, Variable : hhct_probADRcase_followup
    //   1.   ATB screening sensitivity
    //          Currently : 90%, Variable : hhct_testSens_atb
    //   2.   LTB screening sensitivity
    //          Currently : 90%, Variable : hhct_testSens_ltb
    //   3.   Probability of PT success in clearing latent disease
    //          Currently : 70%, Variable : pt_probSuccess_latent
    //   4.   Probability of PT success in clearing recently-recovered disease
    //          Currently : 70%, Variable : pt_probSuccess_rec
    //   5.   PT protection against reinfection
    //          Currently : 70%, Variable : pt_protAgainstReinfection
    //
    
    //We need an mp file; load the default config.
    auto localmp = std::make_shared<Params>("seedConfig.cfg");//just a temp copy to start the driver object
    
    //Start a new Driver with random seed
    auto d = Driver(tID, localmp, seed);
    
    
    //Load the population in 2020
    d.loadPop(pop_filename);
    cout<<"Population read in year "<<d.getPop()->YNOW()<<endl; //begging of the year
    
    //the population mp includes the value of all lhc1&lhc2 parameters, but not the pt parameters
    //copy population's mp file to driver and update pt parameters from the localmp file
    d.swap_mp(d.getPop()->get_mp());
    
    
    //Update parameters ******************
    //read all pt params from file (popualtion copy maybe outdated)
    vector<string> varNames= {
        "hhct_probADRcase_followup",
        "hhct_testSens_atb",
        "hhct_testSens_ltb",
        "hhct_ptCoverage_0To5",
        "hhct_ptCoverage_0To15",
        "pt_probSuccess_rec",
        "pt_probSuccess_latent",
        "pt_duration",
        "pt_protAgainstReinfection"};

    for(auto var:varNames){
        d.get_mp()->setD(var, localmp->getD(var));
    }
    
    //Scenario: Always Delaminid (2) 
    //June 5th 2023; Running scenario 3
    d.get_mp()->setD("pt_regimen",3);
    //We now need an rng as we are sampling from a range
    std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

    //Need a uniform distribution between 0.5 and one
    std::uniform_real_distribution<double> uni (0.5,1);

    //Sample all six of the variables:
    d.get_mp()->setD("hhct_probADRcase_followup", uni(rng) );
    d.get_mp()->setD("hhct_testSens_atb", uni(rng));
    d.get_mp()->setD("hhct_testSens_ltb", uni(rng));
    d.get_mp()->setD("pt_probSuccess_latent", uni(rng));
    d.get_mp()->setD("pt_probSuccess_rec", uni(rng));
    d.get_mp()->setD("pt_protAgainstReinfection", uni(rng));
    //*********************************
    
    //once all parameters are updated, we update them in the population and for all people
    d.getPop()->readAllParams(d.get_mp());
    
    //run PT scenarios
    d.runPtScenario( ptStartYear,ptEndYear,pt_individualOutcomeYear,finalSimYear,  false );
}
