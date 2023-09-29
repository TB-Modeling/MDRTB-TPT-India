//
//  run.hpp
//  mdrtb
//
//  Created by Trinity on 7/18/19.
//  Copyright © 2019 Trinity. All rights reserved.
//
#pragma once
#include <string>
#include <memory>
#include "Params.h"
#include "ThreadID.hpp"

using namespace std;
using std::string;

const string myCurrentDateTime();


//DS PHASE, 1500 - 1970
// * Post Beta/Cull Calibration function
void runDsPhase(const shared_ptr<ThreadID> tID,
                const int lhc1_size,
                const int lhc2_size,
                const int lhc2_cols,
                std::shared_ptr<Params> mp,
                int startYear,
                int yearsBurninDemog,
                int initialInfections,
                int endYear,
                bool cull,
                int cull_freq);

//DR PHASE, 1970 - 2022
void runDrPhase(const shared_ptr<ThreadID> tID, //iD of the current run
                const int lhc2_size,
                const int endYear,                
                const std::string pop_filename,
                const vector<int> subRun,
                vector<vector<long double>> initialOutputs
);

//For a specific number of good-fitting models in stage 2: run the DRTB phase again and save population at the end
void runDrPhase_savePop(const shared_ptr<ThreadID> tID,
                    const int endYear,
                    const std::string pop_filename,
                    const vector<int> subRun,
                    vector<vector<long double>> initialOutputs  
);

//Running the PT scenarios
void runPT_2020Forward(const shared_ptr<ThreadID> tID, //iD of the current run
                       const int ptScenario,// 0:no pt, 1:Isonizid; 2:Delamanid
                       const int ptStartYear,
                       const int ptEndYear,
                       const int pt_individualOutcomeYear,
                       const int finalSimYear,
                       const string pop_filename, //population saved in 2020
                       const unsigned seed = 0
                       ) ;

//Running the One Way SA PT scenarios
void runPTSA_2020Forward(const shared_ptr<ThreadID> tID, //iD of the current run
                         const int ptStartYear,
                         const int ptEndYear,
                         const int pt_individualOutcomeYear,
                         const int finalSimYear,
                         const string pop_filename, //population saved in 2020
                         const unsigned seed = 0
                         ) ;
