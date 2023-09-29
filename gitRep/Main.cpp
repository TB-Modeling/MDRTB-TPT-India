//  Created by Trinity on 5/19/16.
//  Copyright © 2016 Trinity. All rights reserved.
//  Resumed work: Oct 26. Parastu & Emily MDRTB model
#include <iostream>
#include <string>
#include <sstream>

#include "Params.h"
#include "Run.hpp"
#include "Functions.h"
#include "ThreadID.hpp"

using namespace std;
using namespace functions;
using std::cout;



//=============================================================================
/**
 * Entry-point.
 *
 * @param argc: arg count; 1 plus count of argv
 * @param argv: arg vector; arguments passed
 * @return: 0 if ran successfully, else 1.
 */

int main(int argc, const char * argv[]) {
    //0 - Stage1 (DS), : ds - year 1500 to 1970 only DSTB - with Cull and Preemptive checks
    //1 - Stage2 (DR), : dr - year 1970 to 2020 with DSTB and DRTB - no cull, some preemptive checks in 2000
    //2 - PT           : pt - PT experimentation 2022 forward
    //3 - PT_TEST_RunPop      : pop_ptesc_run
    //4 - PT_TEST_ReadPop      : pop_ptesc_read
    //5 - Testing parameter values : valtest
    //Before anything else, create the head file on disk
    //The head file was from a time before we had MARCC
    //output files.  Since we are limited by the number
    //of file on MARCC, we should remove this.
    
    //writeHeadFile(argc, argv);
    
    int runFlag = -1;
    
    if (argc < 2) {
        cout << "Missing arguments, exiting" << endl;
        return 0;
    }
    
    if ( strncmp(argv[1],"ds",2) == 0) {
        if (argc != 3) {
            cout << "Wrong DS argument, exiting" << endl;
        } else {
            runFlag = 0;
        }
    }
    else if ( strncmp(argv[1],"dr",2) ==0 ) {
        if (argc != 4) {
            cout << "Wrong DR arguments, exiting" << endl;
        } else {
            runFlag = 1;
        }
    } else if (strncmp(argv[1],"pt",2) == 0) {
        if (argc != 7) {
            cout << "Wrong PT arguments, exiting" << endl;
        } else {
            runFlag = 2;
        }
    } else if (strncmp(argv[1],"onewaysa",8) == 0) {
        if (argc != 6) {
            cout << "Wrong onewaysa arguments, exiting (" << argc << ")\n";
        } else {
            runFlag = 3;
        }
    } else if (strncmp(argv[1],"fr",2) == 0) {
	    //Arguments are ./mdrtb.out fr ds_id dr_id rep
        if (argc != 5) {
            cout << "Wrong Full Run arguments, exiting" << endl;
        } else {
            runFlag = 4;
        }
    }
    //size of LHS sample for stage 1 & 2 calibration
    const auto lhc1_size = 10000; //The number of DS runs
    //Each successful DS run will generate a random LHS for stage 2 parameters:
    const auto lhc2_size = 100; //The number of DR runs
    const auto lhc2_cols=__V_PARAM_DIST_2.size(); //number of DR parameters
    
    //tID: a composit variable
    //index(0): DS ID [0 .... LHC1 size]
    //index(1): DR ID
    
    
    if (runFlag == 0) { //-------------------------------------------------------------------------------------------------------------------------
        /* Single DS Run:
         -Start a new population
         -Read param values from LHS1
         -Run the model to 1970 (with/out cull & beta_sampling)
         -Run preemptive checks over time and stop models that fail those checks
         -Save output & population at 1970
         -If model survives to 1970, genrate a new latin hypercube for remaining parameters post-1970 (LHS2)
         */
        
        //Execution: ./mdrtb.out ds <Hypercube Line ID>
        
        const auto tID = make_ThreadID( { {"tID", stoul(argv[2]) } } );
        
        auto mp = make_shared<Params>("seedConfig.cfg");
        //Ensure that the DR parameters are set to no DR for now
        mp->setD("probResistance",0.0);
        mp->setD("coefMaxInfect_DR_RelTo_DS",0.0);
        mp->setD("hhct_probADRcase_followup", 0);
        
        
        int startYear=1500;
        int yearsBurninDemog=0;
        int initialInfections=300;
        int endYear=1970; //end the stage 1 calibration
        //int yearsBurninDstb=521; //Run until 2020 (startYear + yearsBurninDstb - 1)
        
        //Cull:
        //  If this is true, then the population will be culled every cull_freq years.
        //      The population will have continuous growth, and the culling will end in year 1900
        //  If this is false, the population will be kept at a fixed size until 1900, then it
        //      will be allowed to grow as above.  cull_freq is not used.
        const bool cull = true;
        const int cull_freq = 20;
        
        
        runDsPhase(tID,
                   lhc1_size,
                   lhc2_size,
                   lhc2_cols,
                   mp,
                   startYear,
                   yearsBurninDemog,
                   initialInfections,
                   endYear,
                   cull,
                   cull_freq);
        
        
        
    } else if (runFlag == 1) {//-------------------------------------------------------------------------------------------------------------------------
        /* Single DR Run:
         -Read in a population in 1970 from the pool of available popualtions on file
         -Pass on outputs from the initial run (saved in 1970) to the new run
         -Read corresponding LHS2 for this population (saved at the end of stage1)
         -Set param values from the corresponding LHS2
         -Run the model to endYear=2022
         -Run preemptive checks for DRTB incidence and stop models that fail it
         -If model survives to endYear, write the outputs to file
         */
        
        //Single DR Run
        int endYear=2022;//after the restart, Run until the end of 2022
        const auto dsID = stoul(argv[2]);
        const auto drID = stoul(argv[3]);
        const auto tID = make_ThreadID({ {"dsID", dsID},{"drID", drID} });
        
        
        //Craft the filename of the coresponding population: We need to use the dsID alone here; the population was created with only it.
        std::stringstream pop_filename;
        pop_filename << "stage1_pop_";
        pop_filename << dsID;
        pop_filename << "_1970.gz";
        
        //Craft the filename of the coresponding output file
        //As above, we use only the dsID here.
        std::stringstream output_filename;
        output_filename << "stage1/out_stage1_complete_";
        output_filename << dsID;
        //Read the outputs
        auto initialOutputs = readVector<long double> ( output_filename.str(), true );
        
        //Craft the filename of the corresponding hypercube file
        std::stringstream hc_filename;
        hc_filename << "stage2_hc_" << dsID;
        auto stage2_hc = readVector<int> ( hc_filename.str() );
        //Now we have the hypercube for this tID
        
        //Do we generate a population for this run?
        const bool gen_pop = true;
        
        if (!gen_pop) {
            runDrPhase(tID,
                    lhc2_size,
                    endYear,
                    pop_filename.str(),
                    stage2_hc[drID],
                    initialOutputs
                    );
        } else {
            runDrPhase_savePop(tID,
                           endYear,
                           pop_filename.str(),
                           stage2_hc[drID],
                           initialOutputs
                           );
        }
        
    } else if (runFlag == 2) {
        /* Single PT Run:
         -Read in a population in 2020 from the popualtions on file
         -Run the specific pt scenario asked for
         */
        
        //Execution: ./mdrtb.out pt <Hypercube Line ID> <Stage2 hypercube ID> <PtScenario> <Rep> <PtID>
        
        //AWS
        const auto dsID = stoul(argv[2]);
        const auto drID = stoul(argv[3]);
        const auto ptScenario = stoul(argv[4]);
        const auto iter = stoul(argv[5]);
        //ptID is the pt iteration number.  We are running 1,000 simulations pulled from a weighted
        //list of 681 runs; running 10 reps of the four pt scenarios on each.  We will at some point
        //execute the same run twice, and we don't want the output files of one to overwrite the
        //output files of the other, and from the model's perspective these are the same run
        //
        const auto ptID = stoul(argv[6]);
        
        const auto tID = make_ThreadID( { {"dsID", dsID},
            {"drID", drID},
            {"PTScen", ptScenario},
            {"rep", iter},
            {"ptID", ptID} } );
        
        const auto ptStartYear = 2023;
        const auto ptEndYear = 2028;
        const auto pt_individualOutcomeYear=2041;  //minimum of 10 years of follow up for those on PT
        const auto finalSimYear = 2042;
        
        stringstream popName;
        popName << "stage2_pop_" << dsID << "_" << drID << "_2022.gz";
        
        runPT_2020Forward(tID,ptScenario,ptStartYear,ptEndYear,pt_individualOutcomeYear,finalSimYear,popName.str());
        
        
    } else if (runFlag == 3) {
        //One Way sensitivity Analysis
        //  To do this, we will run 100 reps of the Delaminid PT scenario, varying 6 variables uniformly
	//  between the values of 50% and 100%.  These variables are:
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
        
        const auto dsID = stoul(argv[2]);
        const auto drID = stoul(argv[3]);
        const auto rep = stoul(argv[4]);
        const auto ptID = stoul(argv[5]);

        //We are operating such that each run varies the 6 variables
	
        //We don't need to identify scenario here as it's always delaminid
        //We should know which variable is being adjusted, and if it's high or low
        const auto tID = make_ThreadID( { {"dsID", dsID},
            {"drID", drID},
            {"rep", rep},
            {"ptID", ptID} } );
        
        const auto ptStartYear = 2023;
        const auto ptEndYear = 2028;
        const auto pt_individualOutcomeYear=2041;  //minimum of 10 years of follow up for those on PT
        const auto finalSimYear = 2042;
        
        stringstream popName;
        popName << "stage2_pop_" << dsID << "_" << drID << "_2022.gz";
        
        runPTSA_2020Forward(tID,ptStartYear,ptEndYear,pt_individualOutcomeYear,finalSimYear,popName.str());
    } else if (runFlag == 4) {
	//This is the full run to test against different seeds
	
        auto mp = make_shared<Params>("seedConfig.cfg");
        //Ensure that the DR parameters are set to no DR for now
        //mp->setD("probResistance",0.0);
        //mp->setD("coefMaxInfect_DR_RelTo_DS",0.0);
        //mp->setD("hhct_probADRcase_followup", 0);
        
        
        int startYear=1500;
        int yearsBurninDemog=0;
        int initialInfections=300;
        int dsEndYear=1970; //end the stage 1 calibration
        int drEndYear=2022;//after the restart, Run until the end of 2022
        const auto dsID = stoul(argv[2]);
        const auto drID = stoul(argv[3]);
	const auto rep = stoul(argv[4]);
        const auto tID = make_ThreadID({ {"dsID", dsID},{"drID", drID},{"rep",rep} });
        //int yearsBurninDstb=521; //Run until 2020 (startYear + yearsBurninDstb - 1)
        
        //Cull:
        //  If this is true, then the population will be culled every cull_freq years.
        //      The population will have continuous growth, and the culling will end in year 1900
        //  If this is false, the population will be kept at a fixed size until 1900, then it
        //      will be allowed to grow as above.  cull_freq is not used.
        const bool cull = true;
        const int cull_freq = 20;

        std::stringstream output_filename;
        output_filename << "stage2/out_stage2_complete_";
        output_filename << dsID << "_" << drID;
        //Read the model parameters
        auto run_values = readVector<long double> ( output_filename.str(), true );

	//We want to read a year post 1970 but before the reduction in transmission in 2000
	//1980 is fine, which is the 480th line of the output file
	const auto config_line = run_values[480];


	//Values that need to be set:
	//
	//DS
    	//"coefComTrans"
    	//"coefHHTrans"
    	//"probMaxMortalityTB"
    	//"probMaxSeekCare"
    	//"coefInfec_0to10"
    	//"probPretreatmentLTF_DSTB"
    	//"probDSTrtFailure"
    	//"probRelapse_DS"
    	//"probSlowProg"
    	//"probSpontResolution"
    	//"coefImmunityLatent"
    	//"coefInfect_trtFailure"
    	//"coefFastProg"
	//DR
    	//"probResistance"
    	//"coefMaxInfect_DR_RelTo_DS"
    	//"coefReactReducPost2000"
    	//"coefTransReducPost2000"
    	//"probPretreatmentLTF_DRTB"
    	//"probDRTrtFailure"
    	//"probRelapse_DR"
    	//"probSuccessDStrt_DRTB"
	//
	//Variables start at index 56 of the output file, but 56 itself is coefTrans
	
        mp->setD("coefHHTrans",config_line[56+1]);
        mp->setD("coefComTrans",config_line[56+2]);
        mp->setD("probMaxMortalityTB",config_line[56+3]);
        mp->setD("probMaxSeekCare",config_line[56+4]);
        mp->setD("probResistance",config_line[56+5]);
        mp->setD("coefMaxInfect_DR_RelTo_DS",config_line[56+6]);
        mp->setD("coefTransReducPost2000",config_line[56+7]);
        mp->setD("coefReactReducPost2000",config_line[56+8]);
        mp->setD("coefInfec_0to10",config_line[56+9 ]);
        mp->setD("probPretreatmentLTF_DSTB",config_line[56+10]);
        mp->setD("probPretreatmentLTF_DRTB",config_line[56+11]);
        mp->setD("probDRTrtFailure",config_line[56+12]);
        mp->setD("probDSTrtFailure",config_line[56+13]);
        mp->setD("probRelapse_DS",config_line[56+14]);
        mp->setD("probRelapse_DR",config_line[56+15]);
        mp->setD("probSlowProg",config_line[56+16]);
        mp->setD("probSpontResolution",config_line[56+17]);
        mp->setD("coefImmunityLatent",config_line[56+18]);
        mp->setD("coefInfect_trtFailure",config_line[56+19]);
        mp->setD("probSuccessDStrt_DRTB",config_line[56+20]);
        mp->setD("coefFastProg",config_line[56+21]);

        const bool gen_pop = false;
        
	//Full Rrefox
	//un
	//
	/*runFullDSDR(tID,
		    lhc1_size,
		    lhc2_size,
		    lhc2_cols,
		    mp,
		    startYear,
		    yearsBurninDemog,
		    initialInfections,
		    dsEndYear,
		    drEndYear,
		    cull,
		    cull_freq
		   );*/
    } else {
        std::cout << "Unrecognized Run Parameter : " << argv[1] << std::endl;
        return -1;
    }
    return 0;
}


// __hhct_probADRcase_followup: use this to model baseline vs pt-scenarios:  if 0, no hhct take place; if >0, some proportion of ADR cases are followed to households
// e.g., set to 0 till 2020;
//set to 0.1 in 2020 (start modeling pt)
//set to 0 in 2025 (stop pt)
//we control the __pt_regimen from the outside to model different scenarios (1=isonizid, 2=delamanid)
//

/* assumptions
 //HHCT: excluding those on DS/DR treatment.
 //Missed ATB cases will be tested for latent TB and can receive PT (no effect)
 //Assuming seperate probability of PTsuccess for those in latent (affecting risk of progression) and recovered (affecting risk of relapse) states
 
 //prolonged hh duration until no one is on PT
 */

/* Updates to the code
 Oct 8:
 -pt regimen set to 3 for baseline scenario
 -Added year of HHCT & pt to indiv outputs
 
 Oct 14:
 - took out incDS_failure and incDR_failure: we dont count trt failure toward incidence anymore.
 - found an error with how new and retreated patients were recorded in enterADS() function by mistake and took that out.
 - pt starts in 2021 now and population outcomes are reported in 2036
 */

//Nov 6 update
//logic test: why some people can recover from DRTB in Delamanid scenario, while having active DS?
//my guess is that they developed active DSTB while on PT, given that even among successful pt cases, reduction in pt transmission is <100%

//_hhct_indexCase_timeMarked not recorder for hh members
//when people are created, set lastDSstate =0 not -1


//Emily:
//not modeling trans reduction post 2020?
//cant trace source of infection to HH or Com for those developing DR by resistance 
