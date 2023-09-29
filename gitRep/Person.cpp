/*  Person.cpp
 
 Created by Trinity on 8/4/15.
 Copyright (c) 2015 Trinity. All rights reserved.
 
 transType=
 -1 for model initialization;
 -2 for initial infections with no origin,
 -0 for hh and 1 for comm.
 Once infected, we dont reset this value any more unless it's written over by
 a new value after reinfection.
 */

#include "Functions.h"
#include "Person.h"
#include "Stats.h"
#include "Params.h"
#include "GlobalVariables.h"

#include <iostream>
#include <vector>
// #include <algorithm>
#include <random>
#include <array>
#include <memory>

using namespace std;
using namespace globalVariables;
using functions::rng_geo;
using functions::rng_unif_decimal;
using functions::rng_unif_int;


//-----------------------------------------------------------------------------
//Error messages
string errPrefix = "Unexpected Error\n";
string errSuffix = "\n\nExiting simulation.";
string simultaneousDsDrTbErr = errPrefix +
"A patient within the simulation began drug-resistant TB treatment, but "
"was in an active drug-susceptible TB state at the time. This should not "
"happen." + errSuffix;
string postDrActiveDsTbErr = errPrefix +
"A person recovering from DRTB can not be actively infected with DSTB."
+ errSuffix;
string sub100pSpecificityErr = errPrefix +
"DS patients can be misclassified for DRTB diagnosis." + errSuffix;
string sub100pSpecificityErr2 = errPrefix +
"DS patients can be misclassified for DRTB diagnosis / treatment."
+ errSuffix;
string noDsStateErr = errPrefix + "None of the DS states matched." + errSuffix;
string noDrStateErr = errPrefix + "None of the DS states matched." + errSuffix;
string wrongAge=  errPrefix + "Peson should age beyond max age." + errSuffix;
string noRecovery=  errPrefix + "Peson should not go back to susceptible state." + errSuffix;
string badPtEnd= errPrefix + "Person is taken off PT due to an infection that occured before PT began." + errSuffix;
string badPtCall= errPrefix + "The diagnosis time doesnt match treatment initiation and PT souldnt be modeled here." + errSuffix;
string badHhStatus= errPrefix + "Infected person should belong to a household." + errSuffix;
string noCareSeeking=errPrefix + "Probability of seeking care is zero." + errSuffix;
string noActiveDisease=errPrefix + "Patient is not infected anymore." + errSuffix;
string missingHh=errPrefix + "ATB patient doesnt have a HH." + errSuffix;
string errPlaceboPt=errPrefix + "PLACEBO arm can not be succcessful." + errSuffix;


//-----------------------------------------------------------------------------
//CLASS FUNCTIONS
//-----------------------------------------------------------------------------
Person::Person(){}

Person::Person(mt19937 &rng, std::shared_ptr<Params> mp, const std::shared_ptr<Stats> &s, const std::shared_ptr<Household> &hh, bool bNewBorn){
    _mp = mp;
    _stats = s;
    _hh=hh;
    // Setting the gender:
    if (rng_unif_decimal(rng) < __PROP_MALE) _gender=0; //0 is male
    else _gender=1;
    // Setting the age:
    if (bNewBorn){
        _age=0;
    }
    else{
        vector<double> vec=(_gender==0)? __vCUM_AGE_DIST2000_MALE : __vCUM_AGE_DIST2000_FEMALE;
        int ageGroup=functions::returnRand_multinom(rng,vec );//a number between 0 to vec.size()
        _age= rng_unif_int(rng,5)+ ageGroup*5;
    }
    readAllParams(mp);
    initAllVariables (rng);
}
Person::~Person(){
    //cout<<"I'm dying "<<_id<<endl;
}
//Serialization: comapres this person with q to make sure all attributes are the same
bool Person::compare(std::shared_ptr<Person> q){
    if (_id!= q->_id) return false;
    if (_tBorn!= q->_tBorn) return false;
    if (_age!= q->_age) return false;
    if (_gender!= q->_gender) return false;

#define XM(type, name, default_value, desc) \
    if (name != q->name) return false;
PERSON_VARS_SIMPLE
#undef XM
    
#define XM(name, desc) \
    if (__##name != q->__##name) return false;
PERSON_MODPARAM
#undef XM
    //
    //if (_stats != q->_stats) return false; we check the stats object at the population level (before getting down here);
    
    return true;
}
//-----------------------------------------------------------------------------
//PARAMS READ
//-----------------------------------------------------------------------------
void Person::initAllVariables (mt19937 &rng) {//local variables
    _id = _stats->getPersonID();_stats->incPersonID();
    _tBorn = _stats->getTime();
    
#define XM(type, name, default_value, desc) \
    name = default_value;
PERSON_VARS_SIMPLE
#undef XM
}

void Person::readAllParams ( std::shared_ptr<Params> mp ) {//read values from input file
    _mp = mp;

#define XM(name, desc) \
    __##name = _mp->getD(#name);
PERSON_MODPARAM
#undef XM

}

void Person::readSelectedParams(std::shared_ptr<Params> mp ){//reads selected params from the mp file
    __probResistance=mp->getD("probResistance");
}

//INITITAL INFECTIONS-----------------------------------------------
void Person::getInfectedDS(mt19937 &rng,const int state) {//get infected with DSTB-for epidemic initialization
    if (state==1)    enterELDS(rng);
    else {
        if (state==2) enterLLDS(rng);
        else{
            if (state==3) enterADS(rng,false);
            else enterRECDS(rng,false);
        }
    }
}

//-----------------------------------------------------------------------------
// MODELING CONTACTS/TRANSMISSION WITHIN HOUSEHOLD
//-----------------------------------------------------------------------------
void Person::modelHHcontacts(mt19937 &rng, vector<shared_ptr<Person>> &contact_list, const double infectiousness){ //loops through HH members and models transmission events
    //vector<shared_ptr<Person>> vMarked; //local vector to hold those marked for new transmission
    //coefHHTrans is no longer fixed in Person, pull the value in from _mp
    const auto coefHHTrans = _mp->getD("coefHHTrans");
    vector<shared_ptr<Person>> hhmembers=_hh->getMembers(); //vector of all hh members
    for (std::shared_ptr<Person> q:hhmembers){
        if((q->getId() != this->getId()) && (!q->isMarkedTran_DS()) && (!q->isMarkedTran_DR())){ //only model new HH transmissions to those who are not infected in this timestep already
            double imm=0;
            if(_DSstate==DS_A) imm=q->returnImmunity(false);
            else imm=q->returnImmunity(true);
            //infectiousness was updated in the last step in the population class
            double ptrans= infectiousness *(1-imm)* __coefTrans * coefHHTrans; //computing the probability of transmission
            if (rng_unif_decimal(rng)< ptrans){//transmision happens
                
                //we will update tTrans (and nReinf) in the ELDS and ELDR states (if reinfection happens)
                //**************
                if (_DSstate==DS_A){// transmission was from DSTB
                    q->setTransRouteDS(0); //Household transmission code
                    q->setMarkedTran_DS(true);
                    contact_list.push_back(q);
                    _stats->record_nTransDS();
                    _stats->record_nTransDS_HH();}
                if (_DRstate==DR_A){// transmisison was from DRTB
                    q->setTransRouteDR(0); //Household transmission code
                    q->setMarkedTran_DR(true);
                    contact_list.push_back(q);
                    _stats->record_nTransDR();
                    _stats->record_nTransDR_HH();
                }
                
            }}}
    //return vMarked;
}
bool Person::modelCOMcontact(mt19937 &rng,const std::shared_ptr<Person> q, const double infectiousness){//models COM transmission to a selected member of population (person * q)
    //only model new transmissions for those who are not infected in this timestep (will not override existing HH transmission)
    if ((q->getId()!= this->getId()) && (!q->isMarkedTran_DS()) && (!q->isMarkedTran_DR() )){
        double imm=0;
        if(_DSstate==DS_A) imm=q->returnImmunity(false);
        else imm=q->returnImmunity(true);        //infectiousness was updated in the last step
        //coefComTrans is no longer fixed in Person, pull the value in from _mp
        auto coefComTrans = _mp->getD("coefComTrans");
        double ptrans= infectiousness *(1- imm) * __coefTrans * coefComTrans;
        if (rng_unif_decimal(rng)< ptrans){//transmision happens
            
            //*****************************
            if (_DSstate==DS_A){//transmisison was from DSTB
                q->setTransRouteDS(1); //community transmission code
                q->setMarkedTran_DS(true);
                _stats->record_nTransDS();
                return true;
            }
            if (_DRstate==DR_A){//transmisison was from DRTB
                q->setTransRouteDR(1); //community transmission code
                q->setMarkedTran_DR(true);
                _stats->record_nTransDR();
                return true;
            }}}
    return false;
}

//-----------------------------------------------------------------------------
//MODELING TB DYNAMICS
//-----------------------------------------------------------------------------
//Demographics-----------------------------------------------------------------
bool Person::willDieNaturally(mt19937 &rng,const vector<double> vMort){//returns true if person is ready to die
    if (rng_unif_decimal(rng)< vMort[_age]){//ll die
        if (_DSstate>3){//with active disease or in treatment
            _stats->record_vDurDSTB(TNOW()-_tActiveInfection);
        }
        if (_DRstate>3){
            _stats->record_vDurDRTB(TNOW()-_tActiveInfection);
        }
        //save death attributes
        _death_time=TNOW();
        _death_route=0; //natural mortality
        _death_dsState=_DSstate;
        _death_drState=_DRstate;
        return true;
    }
    if (_age > __MAX_AGE) throw logic_error(wrongAge);
    return false;
}

//Natural history--------------------------------------------------------------
double Person::returnMortalityTB(mt19937 &rng) const {//returns the probability of TB mortality as a function of time since infection (linear increase and then fix mean)
    double tSinceInfection=TNOW()-_tActiveInfection;    //time from active disease development or treatment failure
    if (tSinceInfection>__durToMaxLevel)//prob of mortality in each month is independant of the previous month and has been increase. so we use a new rng number
        return __probMaxMortalityTB;
    else
        return ((tSinceInfection/__durToMaxLevel)*__probMaxMortalityTB);
}

int Person::returnTimeToFastProg(mt19937 &rng) const {//At the time of entry to ELTB, computes the time remaining to fast prog to active disease over the first 60 months (returns -1 if fastPrg doesnt occur)
    //1- __coefFastProg : overall coef of fastprog; varied in calibration
    
    //2- coef of fastProg based on age:
    double cFPbyage;
    if (_age>15) cFPbyage=__probFastProgAge_adult;
    else { if (_age>10) cFPbyage=__probFastProgAge_10To15;
    else  { if (_age>2) cFPbyage=__probFastProgAge_2To10;
        else cFPbyage=__probFastProgAge_0To2;}}
    //
    int tProgress=-1;//time of fast progression
    //
    double probFP=cFPbyage*__coefFastProg; //individual level prob of FP
    //fast progression happens?
    if(rng_unif_decimal(rng)<probFP){ //if so, at which month?
        double var=rng_unif_decimal(rng);
        int x=0; //index in the vector
        while( var> fp_expo_decay[x] )
            x=x+1;
        //
        tProgress=TNOW()+x+1; //since we started from 0
    }
    return tProgress;
}

int Person::returnTimeToSlowProg(mt19937 &rng) const {//I used geometric distribution to estiamte the time remaining to a future event
    double monthlyProb= (__probSlowProg) / 12;  //Convert this to a montly probability; it is stored
    return (rng_geo(rng, monthlyProb)+TNOW());  //in the model as a yearly prob.
}

//Infectiousness---------------------------------------------------------------
double Person::returnInfectiousness(mt19937 &rng) const {//updates/returns TB infectiousness depending on age and current TB state
    //checks trt status and MDR type to decide infectiousness
    double infectiousness=0;
    if (_age < 10 ) {
        infectiousness=__coefInfec_0to10;
        return infectiousness;
    }
    if (_DSstate== DS_A ) {// active DSTB
        infectiousness=computeInfectiousness_DS();
        return infectiousness;
    }
    if ((_DSstate==DS_TRDS )||(_DSstate==DS_TRDR )){//DS patients under treatment
        double var=0;
        if (_bIsGoingToFailTrt) var=__coefInfect_trtFailure;
        infectiousness=computeInfectiousness_DS()* var;
        return infectiousness;
    }
    if (_DRstate== DR_A) {// active DRTB
        infectiousness =computeInfectiousness_DR();
        return infectiousness;
    }
    if ((_DRstate==DR_TRDS )||(_DRstate==DR_TRDR )){//DS patients under DS treatment
        double var=0;
        if (_bIsGoingToFailTrt) var=__coefInfect_trtFailure;
        infectiousness=computeInfectiousness_DR()* var;
        return infectiousness;
    }
    return 0;//if all fails, infectiousness is set to 0
}
double Person::computeInfectiousness_DR() const {//computes infectiousness as a function of time since infection (linear growth and then fix value)
    double tSinceInfection= TNOW() - _tActiveInfection;//months since infection
    double max=__coefMaxInfect_DR_RelTo_DS*__coefMaxInfect_DS;
    if (tSinceInfection>__durToMaxLevel)
        return max;
    else
        return ((tSinceInfection/__durToMaxLevel) * max);
}
double Person::computeInfectiousness_DS() const {
    double tSinceInfection= TNOW() - _tActiveInfection;//months since infection
    if (tSinceInfection>__durToMaxLevel)
        return __coefMaxInfect_DS;
    else
        return ((tSinceInfection/__durToMaxLevel)* __coefMaxInfect_DS);
}

//Immunity---------------------------------------------------------------------
double Person::returnImmunity(bool towardDRTB) const { //returns immunity toward future transmissions with DSTB (default) or drtb
    //#####Intervention:
    double protcPT=0; //no default protection
    if (!towardDRTB){ //both PT regimen protect against DSTB
        if(_bIsOnPt && (_pt_regimen==ISONAIZID || _pt_regimen==DELAMANID) )
            protcPT=__pt_protAgainstReinfection; //[0-1]
        return (1-
                ((1-max(_immDR,_immDS)) *(1-protcPT)));//susceptibilityDS/DR * susceptibilitytPt
    }else{
        if((_bIsOnPt) && (_pt_regimen==DELAMANID))
            protcPT=__pt_protAgainstReinfection;
        return (1-
                ((1-max(_immDR,_immDS)) *(1-protcPT)));//
    }
    
}

//Care seeking-----------------------------------------------------------------
/* Time to seek care as a function of time since infection (linear growth and
 * then fix value).
 */
int Person::getTimeToSeekCare(mt19937 &rng) const {
    //if (__probMaxSeekCare==0) throw logic_error(noCareSeeking);
    
    int res=0;
    double tSinceInfection=TNOW() - _tActiveInfection;
    //
    if (tSinceInfection< __durToMaxLevel){ //still within the initial infection period (0-8 months < 9)
        for (int m=tSinceInfection;m<__durToMaxLevel;m++){//loop through remaining months from now to __durToMaxLevel, and model potential event
            double prob=(m/__durToMaxLevel)*__probMaxSeekCare;
            if (rng_unif_decimal(rng)<=prob)
                return ( TNOW() + (m-tSinceInfection));//ends here
        }
        //else: estiamte the future event froma  geometric dist
        int delay= rng_geo(rng, __probMaxSeekCare);
        //
        res=TNOW()+ (__durToMaxLevel-tSinceInfection)+ delay;
    }
    else{ // more time has passed since infection and we are already at maximum level
        res= TNOW()+ rng_geo(rng, __probMaxSeekCare);
    }
    return res;
}

/*
 * Care visits are only modeled for TB infected patients and not healthy
 * individuals. In each care visit, people are (i) first diagnosed with TB
 * (DSTB) and, (ii) depending on calendar year and previous history, they might
 * be tested for DRTB too. At the end, they will receive one diagnosis (either
 * DSTB or DRTB).
 *
 * Following diagnosis, (iii) there is a prob of (a) loss to followup before
 * starting treatment. If they're lost to followup, (b) they can return to care
 * in the future and won't remember this visit. Otherwise, they can (c) start
 * 1 of the following treatments:
 * - infected with DSTB & receiving trt for DSTB
 * - infected with DSTB & receiving trt for DRTB
 * - infected with DRTB & receiving trt for DSTB
 * - infected with DRTB & receiving trt for DRTB
 *
 * @param *rng: Random number
 */
bool Person::modelCareVisits(mt19937 &rng) {
    if (_tSeekCare==TNOW()){
        if ((_DSstate!=4) && (_DRstate!=4)) throw logic_error(noActiveDisease);
        //        _tSeekCare=-1;//reset the variable
        _nVisits++;
        _stats->record_nVisits();//
        
        /* i. Initial TB (DSTB) Diagnosis... ==================================
         *  ...has already been completed by this point. */
        
        /* ii. DSTB Screening =================================================
         * Once patient is diagnosed with DSTB, we check the possibility of
         * drug susceptibility testing (DST). Only a fraction of patients
         * receive DST, and if they dont receive it, DRTB goes undiagnosed. */
        bool bDiagDRTB=false;
        double pDST=returnProbDST(); //a function of calendar year and previous TB status
        if (rng_unif_decimal(rng) <= pDST){
            _stats->record_nDST();
            if (_DRstate==4) //active DRTB?
                bDiagDRTB=true;
        }
        //record time of diagnosis:
        if (bDiagDRTB)
            _tDiagnosis_DR=TNOW();
        else
            _tDiagnosis_DS=TNOW();
        // saving statistics: begin *************************************************************************************
        //record the diagnosis:
        if (bDiagDRTB)
            _stats->record_nDiag_DR();
        else
            _stats->record_nDiag_DS();
        
        /* what fraction of patients getting diagnosed with TB *have* DR, whether or not they get diagnosed as such.*/
        if (_bTreated_ever) {
            _stats->record_nRetreatedPatient();
            if(_DRstate==DR_A) _stats->record_nRetreatedDRTB();
        }
        else{
            _stats->record_nNewPatient();
            if(_DRstate==DR_A)  _stats->record_nNewDRTB();
        }
        /* what proportion of hh contacts are infected with TB at the time of diagnosis */
        /* we also record the prop of DS-index case hh members who are infcted with any TB and originated in HH*/
        if (_hh == nullptr) throw logic_error(missingHh);
        
        double nSusDSDR=0;
        double nSusDS=0;
        double nSusDR=0;
        double nADS=0;
        double nADR=0;
        double nLDS=0; //number of hh members with latent or active or inTrt DS
        double nLDR=0;
        double nLDS_LDR=0;
        double nADS_LDR=0;
        double nADR_LDS=0;
        
        double nADS_HH=0; //number of hh members with ADS who origiated from hh
        double nADR_HH=0;
        
        double nLDS_HH=0; //number of hh members with latent or rec tb who origiated from hh
        double nLDR_HH=0;
        double nKidsLt6=0;
        double nKidsLt16=0;
        
        for(std::shared_ptr<Person> p:_hh->getMembers()){
            if (p->getId()!= _id){ //exclude the index case
                if(p->getAge()<6) nKidsLt6++;
                if(p->getAge()<16) nKidsLt16++;
                
                if(p->getDSstate()==DS_SUS && p->getDRstate()==DR_SUS) nSusDSDR++;
                if(p->getDSstate()==DS_SUS  ) nSusDS++;
                if(p->getDRstate()==DR_SUS) nSusDR++;
                
                if(p->getDSstate()==DS_A) nADS++; //ADS
                if(p->getDRstate()==DR_A) nADR++; //ADR
                if(p->getDSstate()==DS_EL|| p->getDSstate()==DS_LL || p->getDSstate()==DS_REC) nLDS++; //HH members who are latently infected
                if(p->getDRstate()==DR_EL|| p->getDRstate()==DR_LL || p->getDRstate()==DR_REC) nLDR++;
                //combinations
               if((p->getDSstate()==DS_A) &&
                   (p->getDRstate()>DR_SUS)) nADS_LDR++;
                
                if((p->getDRstate()==DR_A) &&
                   (p->getDRstate()>DS_SUS)) nADR_LDS++;
                
                if((p->getDSstate()==DS_EL|| p->getDSstate()==DS_LL || p->getDSstate()==DS_REC) &&
                   (p->getDRstate()==DR_EL|| p->getDRstate()==DR_LL || p->getDRstate()==DR_REC)) nLDS_LDR++;

                
                if ((p->getDSstate()==DS_A)
                    && (p->getTransRouteDS()==0))     nADS_HH++; //number of HH members with active TB who were infected from this hh
                
                if ((p->getDSstate()==DS_EL|| p->getDSstate()==DS_LL || p->getDSstate()==DS_REC)
                    && (p->getTransRouteDS()==0))    nLDS_HH++;
                
                if ((p->getDRstate()==DR_A)
                    && (p->getTransRouteDR()==0))   nADR_HH++; //number of HH members with active TB who were infected from this hh
                
                if ((p->getDRstate()==DR_EL || p->getDRstate()==DR_LL || p->getDRstate()==DR_REC)
                    && (p->getTransRouteDR()==0))    nLDR_HH++;
            }}
        if(!bDiagDRTB){
            _stats->record_vHhSize_atDiagADS(_hh->getSize() - 1 ); //# of household members traced minus the index case
            _stats->record_vHhPrevSUS_atDiagADS(nSusDSDR);
            _stats->record_vHhPrev_ADS_atDiagADS(nADS);
            _stats->record_vHhPrev_ADR_atDiagADS(nADR);
            _stats->record_vHhPrev_LADS_atDiagADS(nLDS);
            _stats->record_vHhPrev_LADR_atDiagADS(nLDR);
            _stats->record_vHhPrev_LADS_LADR_atDiagADS(nLDS_LDR);
            _stats->record_vHhPrev_ADS_LADR_atDiagADS(nADR_LDS);
            _stats->record_vHhPrev_ADR_LADS_atDiagADS(nADS_LDR);
            //nLADR
            _stats->record_vHhPrevADS_hhOriginated_atDiagADS(nADS_HH);
            _stats->record_vHhPrevLADS_hhOriginated_atDiagADS(nLDS_HH);
            //nADR_HH
            //nLADR_HH
            
        }
        if(bDiagDRTB){
            _stats->record_vHhSize_atDiagADR(_hh->getSize() - 1 ); //# of household members traced minus the index case
            _stats->record_vHhPrevSUS_atDiagADR(nSusDSDR);
            _stats->record_vHhPrev_ADS_atDiagADR(nADS  );
            _stats->record_vHhPrev_ADR_atDiagADR(nADR );
            _stats->record_vHhPrev_LADS_atDiagADR(nLDS);
            _stats->record_vHhPrev_LADR_atDiagADR(nLDR);
            _stats->record_vHhPrev_LADS_LADR_atDiagADR(nLDS_LDR);
            _stats->record_vHhPrev_ADS_LADR_atDiagADR(nADS_LDR);
            _stats->record_vHhPrev_ADR_LADS_atDiagADR(nADR_LDS);
            //nADS_HH
            //nLADS_HH
            _stats->record_vHhPrevADR_hhOriginated_atDiagADR(nADR_HH);
            _stats->record_vHhPrevLADR_hhOriginated_atDiagADR(nLDR_HH);
            
        }
        // saving statistics: end ************************************************************************************************************//
        // Model prob of pretreatment loss to follow up
        double pLTF=__probPretreatmentLTF_DSTB;
        if (bDiagDRTB) pLTF=__probPretreatmentLTF_DRTB;
        if (rng_unif_decimal(rng)<=pLTF) { //person is lost but will return to care later on
            _tLTFU=TNOW();
            _stats->record_nPretreatmentLTF();
            _tSeekCare=getTimeToSeekCare(rng); //we wont remember (save) this diagnosis for this patient
        }
        else{ // Receiving treatment
            if (bDiagDRTB){ //diagnosed with DRTB
                //*******************************************************************************************************************************************
                //*******************************************************************************************************************************************
                //Implementation of PT
                if((__hhct_probADRcase_followup>0)&&(rng_unif_decimal(rng)<__hhct_probADRcase_followup)){
                    if (_hh->get_hhct_isScreened()==false){ //make sure that the hh has not been screened before
                        
                        //record stats:
                        _hh->set_hhct_isScreened(true);
                        _stats->record_nHhct_hhScreened();
                        _stats->record_nHhct_hhMembersScreened(_hh->getSize()-1);
                        _stats->record_nHhct_hhMembersLt6(nKidsLt6);
                        _stats->record_nHhct_hhMembersLt16(nKidsLt16);
                        
                        
                        //record personal atributes
                        _hhct_hhID=_hh->getId();
                        _hhct_hhSize=_hh->getSize()-1; //size of hh excluding the index case
                        _hhct_hhPrevSUS_DSDR = nSusDSDR;//prevalence of SUS people
                        _hhct_hhPrevSUS_DS = nSusDS;//prevalence of SUS people
                        _hhct_hhPrevSUS_DR = nSusDS;//prevalence of SUS people
                        _hhct_hhPrevADS = nADS;//prevalence of active DS at hhct among hh members (excludig the index case)
                        _hhct_hhPrevADR =nADR;
                        _hhct_hhPrevLDS=nLDS; //prev of latent DS among hh members
                        _hhct_hhPrevLDR=nLDR;
                        _hhct_nkidsLt6InHH=nKidsLt6;
                        _hhct_nkidsLt16InHH=nKidsLt16;
                        //
                        model_hhct_indexCase(rng);
                    }}
                //*******************************************************************************************************************************************
                //*******************************************************************************************************************************************
                if (getDRstate()==4) enterTRDR_DR(rng); //a-correct treatment (DR infected and receiving DR trt)
                if (getDSstate()==4)   throw logic_error(simultaneousDsDrTbErr); //b- DS infected but receiving DR treatment
            }
            else{
                if (getDSstate()==4) enterTRDS_DS(rng);//c- correct treatment (DS infected and receiving DS trt)
                if (getDRstate()==4) enterTRDS_DR(rng);//d-wrong treatment (DR infected but receiving DS treatment which will fail)
            }
        }
    }
    return false;
}
double Person::returnProbDST() const {//computes and returns the prob of receiving drug susceptibility testing (DST) as a function of time and previous TB status
    if (_nTrtFailure_recent>1)//if two or more treatment failure, we will do DST
        return 1;
    //
    int y= _stats->getYear() ; //0 corresponds to year starting_year_DR, going up to final_year
    double pDST=0;
    if (y>=__DST_FINAL_YEAR){//after final year, use rate for final year
        if (_bTreated_ever)
            pDST= __vPROB_DST__RETREATED_PATIENTS.back();
        else
            pDST= __vPROB_DST__NEW_PATIENTS.back();
    }
    else{
        if (y>__DST_INITIAL_YEAR){ //from 1990 forward, there is a gradual growth in DST prob for new and retreated patients
            if (_bTreated_ever) pDST=__vPROB_DST__RETREATED_PATIENTS[y - __DST_INITIAL_YEAR];
            else  pDST=__vPROB_DST__NEW_PATIENTS[y - __DST_INITIAL_YEAR];
        }}
    
    return pDST;
}
//-----------------------------------------------------------------------------
//ENTER DISEASE STATE
//-----------------------------------------------------------------------------
/* At the time of entry to a TB state, we generally record stats, update local
 variables, and determin the time of progression from this state to other
 potential states.
 -----------------------------------------------------------------------------*/
void Person::enterADS(mt19937 &rng, const bool dsFail ){ //dsFail=T if person has developed active disease after treatment failure
    //Record post-pt stats----------------
    if(_bHasBeenOnPt){ //only record if this person has been on PT and there is no other record before this one (first incidence since PT)
        if (_bIsOnPt) _stats->record_nIncDS_onPt(); //number of incidences among those on PT
        else _stats->record_nIncDS_postPt(); //number of incidences among those post-PT
        //
        if(_postPt_first_dsInc_time==-1) {
            _postPt_first_dsInc_time=TNOW();
            _postPt_first_dsInc_lastDsState=_DSstate;//the current health state has not updated yet
            _postPt_first_dsInc_lastDrState=_DRstate;
            _postPt_first_dsInc_transRoute=_nTransRouteDS;
            _postPt_first_dsInc_transTime=_tTransDS;
        }}
    
    //Record stats ------------------
    if (dsFail==false){
        _stats->record_nIncDS();//we dont count failed trt toward incidence
        if (_nTransRouteDS==0) {
            _stats->record_nIncHH();
            _stats->record_nIncDS_HH();}
        if (_nTransRouteDS==1) _stats->record_nIncCOM();
    }
    
    //Update variables ------------------
    _nReinfToActiveDisease=0; //reset number of reinfections until active disease development
    if (dsFail==false) //if person has developed active disease after treatment failure, we dont update the time of original infection
        _tActiveInfection=TNOW(); //if it's a failure, we dont reset the time
    //
    _bIsGoingToFailTrt=false;
    _lastDSstate=_DSstate;
    _DSstate=DS_A;
    _immDS=1;//since is in ADS
    _tDS_A=TNOW();
    //Times to progress------------------
    //a-seeking care:
    _tSeekCare=getTimeToSeekCare(rng);
    //b-spont Resolution:
    _tProgressToRECDS_ADS=rng_geo(rng,__probSpontResolution)+TNOW(); //geometric dis based on per month prob
}
void Person::enterADR(mt19937 &rng, const bool drFail){ //entered either through new infection, developing resistance or after a failed trt
    //those developing resistance post DS-treatment are moved to RECDS() before coming here (DS-treatment cleared DS infection but created DR infection)
    
    //Record post-pt stats----------------
    if(_bHasBeenOnPt){ //only record if this person has been on PT and there is no other record before this one (first incidence since PT)
        if (_bIsOnPt) _stats->record_nIncDR_onPt();
        else _stats->record_nIncDR_postPt();
        //
        if(_postPt_first_drInc_time==-1) {
            _postPt_first_drInc_time=TNOW();
            _postPt_first_drInc_lastDrState=_DRstate;
            _postPt_first_drInc_lastDsState=_DSstate;
            _postPt_first_drInc_transRoute=_nTransRouteDR;
            _postPt_first_drInc_transTime=_tTransDR;
        }}
    
    //Update variables ------------------
    _nReinfToActiveDisease=0; //reset number of infections until disease development
    _bIsGoingToFailTrt=false;
    _lastDRstate=_DRstate;
    _DRstate=DR_A;
    _immDR=1;
    _tDR_A=TNOW();
    
    //Three groups of patients enter ADR: 1)new transmissions or relapsed patients, 2) DSTB patients failing DSTrt that results in resistance, 3)  DRTB patients failing DStrt or DRtrt (for the third group, we dont reset the infection clock)
    if (drFail==false ){
        _tActiveInfection=TNOW(); //if it's a failure, we dont reset the time
        
        //Record Stats ------------------
        _stats->record_nIncDR(); //We dont count trt failures toward new incidence
        if (_nTransRouteDR==0){_stats->record_nIncHH(); _stats->record_nIncDR_HH();}
        if (_nTransRouteDR==1){_stats->record_nIncCOM();}
    }
    
    //Times to progress  ------------------
    //a-seeking care:
    _tSeekCare=getTimeToSeekCare(rng);
    //b-spont Resolution:
    _tProgressToRECDR_ADR=rng_geo(rng,__probSpontResolution)+TNOW(); //geometric dis based on per month prob
}
//
void Person::enterELDS(mt19937 &rng){
    //Record stats ------------------
    _stats->record_nELDS();
    //Update variables -----------------
    bool bReinfecFromELDS=false;
    if (_DSstate==DS_EL) bReinfecFromELDS=true;
    
    //for all infections except those from ELDS:
    if (bReinfecFromELDS==false){
        _tProgressToADS_ELDS=-1;
        _tProgressToLLDS_ELDS=-1;
    }
    
    _lastDSstate=_DSstate;
    _DSstate=DS_EL;
    _tDS_EL=TNOW();
    _immDS=__coefImmunityLatent;
    _tTransDS=TNOW();
    _nReinfToActiveDisease++;
    //Times to progress ------------------ // we will either move to active disease or late latency
    
    //For those who got reinfected while in ELDS: Dont allow reinfections to shorten the existing time to progression (so people wont get stuck in an infinit loop)
    double newProgTime=returnTimeToFastProg(rng);
    if (bReinfecFromELDS && _tProgressToADS_ELDS>0){//reinfected from ELDS & already has an event sheduled?
        if( (newProgTime>0) //the new time to event is positive
           && (newProgTime <_tProgressToADS_ELDS)) //and this time is smaller than the previous one
            _tProgressToADS_ELDS=newProgTime ;      //replace this time
    }
    else{
        _tProgressToADS_ELDS=newProgTime;
    }
    
    //move to LLTB?
    if(_tProgressToADS_ELDS==-1)
        _tProgressToLLDS_ELDS= TNOW() + __DUR_ELTB; //5 years (it's a global parameter)
}
void Person::enterLLDS(mt19937 &rng){
    //Update variables------------------
    _lastDSstate=_DSstate;
    _DSstate=DS_LL;
    _tDS_LL=TNOW();
    _immDS=__coefImmunityLatent;
    //Times to progress------------------
    //can only move to active disaese
    _tProgressToADS_LLDS=-1;
    _tProgressToADS_LLDS=returnTimeToSlowProg(rng); //geometric dist based on monthly rate
}
void Person::enterELDR(mt19937 &rng){
    //Record stats------------------
    _stats->record_nELDR();
    //Update variables------------------
    bool bReinfecFromELDR=false;
    if (_DRstate==DR_EL) bReinfecFromELDR=true;
    //for all infections except those from ELDR:
    if (bReinfecFromELDR==false){
        _tProgressToLLDR_ELDR=-1;
        _tProgressToADR_ELDR=-1;
    }
    
    _lastDRstate=_DRstate;
    _DRstate=DR_EL;
    _tDR_EL=TNOW();
    _immDR=__coefImmunityLatent;
    _tTransDR=TNOW();
    _nReinfToActiveDisease++;
   
    //Times to progress ------------------ // we will either move to active disease or late latency
    
    //For those who got reinfected while in ELDR: Dont allow reinfections to shorten the existing time to progression (so people wont get stuck in an infinit loop)
    double newProgTime=returnTimeToFastProg(rng);
    if (bReinfecFromELDR && _tProgressToADR_ELDR>0){//reinfected from ELDR & already has an event sheduled?
        if( (newProgTime>0) //the new time to event is positive
           && (newProgTime <_tProgressToADR_ELDR)) //and this time is smaller than the previous one
            _tProgressToADR_ELDR=newProgTime ;      //replace this time
        //else if var<0 or var>_tProgressToADS_ELDS we dont change _tProgressToADS_ELDS
    }
    else{    //if there was no reinfection or _tProgressToADS_ELDS<0
        _tProgressToADR_ELDR=newProgTime ;
    }
    //move to LLTB?
    if(_tProgressToADR_ELDR==-1)
        _tProgressToLLDR_ELDR= TNOW() + __DUR_ELTB; //5 years (it's a global parameter)
}
void Person::enterLLDR(mt19937 &rng){
    //Update params------------------
    _lastDRstate=_DRstate;
    _DRstate=DR_LL;
    _tDR_LL=TNOW();
    _immDR=__coefImmunityLatent;
    //Times to progress------------------
    _tProgressToADR_LLDR=-1;
    _tProgressToADR_LLDR=returnTimeToSlowProg(rng); //geometric dist based on monthly prob
}
//
void Person::enterRECDS(mt19937 &rng,bool postPT){
    //Update variables------------------
    _lastDSstate=_DSstate;
    _DSstate=DS_REC;
    _tDS_REC=TNOW();
    _immDS=__coefImmunityLatent;
    if (_DRstate<4){ //sus or latent infection or recovered for DRTB?
        //        _tSeekCare=-1;
        _bIsGoingToFailTrt=false;
        _tActiveInfection=-1;
    }
    //Times to progress------------------
    //if didnt come here with active DRTB (resulted from deveoping resistance) they can relapse back to ADS
    _tProgressToADS_RECDS=-1;
    if(postPT==false){
        if(rng_unif_decimal(rng)<__probRelapse_DS){ //cumulative prob of relapse in the first two years post recovery
            _tProgressToADS_RECDS=rng_unif_int(rng, 24)+TNOW();//determine the month of relapse
        }}
    //otherwise, stay recovered for lifetime
}
void Person::enterRECDR(mt19937 &rng, bool postPT){ //we assume if DRtrt also treats DS infection, so RECDS also happens
    //if (_DSstate == 4)
    //    throw logic_error(postDrActiveDsTbErr);
    //Update variables------------------
    _lastDRstate=_DRstate;
    _DRstate=DR_REC;
    _tDR_REC=TNOW();
    _immDR=__coefImmunityLatent;
    //    _tSeekCare=-1;
    _bIsGoingToFailTrt=false;
    _tActiveInfection=-1;
    //Times to progress------------------
    _tProgressToADR_RECDR=-1;
    if(postPT==false){
        if(rng_unif_decimal(rng)<__probRelapse_DR){ //cumulative prob of relapse in the first two years post recovery
            _tProgressToADR_RECDR=rng_unif_int(rng, 24)+TNOW();//determine the month of relapse
        }}
    //    else //won't relapse, so it's the same as susceptible since there is no immunity for recovered patients
}
//

void Person::enterTRDS_DS(mt19937 &rng){
    //Record stats----------------
    _stats->record_nTrtDS_DS();
    //Update variables----------------
    _lastDSstate=_DSstate;
    _DSstate=DS_TRDS;
    _tDS_TRDS=TNOW();
    _immDS=1;
    _bTreated_ever=true;
    //Times to progress------------------
    _tProgressToADR_TRDS=-1; //this refers to developing resistance as a result of dstb trt failure
    _bIsGoingToFailTrt=false;
    _tProgressToADS_TRDS=-1;
    _tProgressToRECDS_TRDS=-1;
    //Fail or recover?
    /* Aug 2018: at then end of treatment duration, there are 3 possibilities,
     * and each one happens if the other wont happen. So this is modeled as cumulative probabilities.
     * should not model each process seperately (not being chosen for the
     * first, increases the chances for the next one) */
    double rand=rng_unif_decimal(rng);
    
    if (rand<__probResistance){//develop resistance after trt failure?
        _bIsGoingToFailTrt=true;
        _tProgressToADR_TRDS=__durTrt_DS+TNOW(); //option1
    }
    else{
        if(rand<__probDSTrtFailure+__probResistance){//no resistance but will fail the treatment ?
            _bIsGoingToFailTrt=true;
            _tProgressToADS_TRDS=__durTrt_DS+TNOW();//option2
        }
        else{ //get recovered
            _tProgressToRECDS_TRDS=__durTrt_DS +TNOW();//option3
        }}
}
/* Checkpoint: In this model we dont model <100% specificity, so this should
 * not happen. */
void Person::enterTRDR_DS(mt19937 &rng){
    throw logic_error(sub100pSpecificityErr);
}
void Person::enterTRDS_DR(mt19937 &rng){
    //Record stats----------------
    _stats->record_nTrtDS_DR();
    //Update variables----------------
    _lastDRstate=_DRstate;
    _DRstate=DR_TRDS;
    _tDR_TRDS=TNOW();
    _immDR=1;
    _bTreated_ever=true;
    _tProgressToADR_TRDS=-1;
    _tProgressToRECDR_TRDS=-1;
    //Times to progress--------------------
    if (rng_unif_decimal(rng)<__probSuccessDStrt_DRTB){
        _bIsGoingToFailTrt=false; // Treatment will succeed!
        _tProgressToRECDR_TRDS=__durTrt_DS +TNOW();
    }
    else{
        _bIsGoingToFailTrt=true; // Treatment will fail!
        _tProgressToADR_TRDS=__durTrt_DS +TNOW();}
}
void Person::enterTRDR_DR(mt19937 &rng){
    //Record stats----------------
    _stats->record_nTrtDR_DR();
    //Update variables----------------
    _lastDRstate=_DRstate;
    _DRstate=DR_TRDR;
    _tDR_TRDR=TNOW();
    _immDR=1;
    _bTreated_ever=true;
    //Times to progress-----------------
    _tProgressToADR_TRDR=-1;
    _tProgressToRECDR_TRDR=-1;
    _bIsGoingToFailTrt=false;
    if (rng_unif_decimal(rng)<__probDRTrtFailure){
        _bIsGoingToFailTrt=true;
        _tProgressToADR_TRDR=__durTrt_DR +TNOW();//Failed trt: go back to ADR
    }
    else{
        _tProgressToRECDR_TRDR=__durTrt_DR +TNOW();//successful trt: go to REC
    }
}

//-----------------------------------------------------------------------------
//PROGRESS DISEASE STATE
//-----------------------------------------------------------------------------
bool Person::progressDS(mt19937 &rng){//depending on the current state, will model the progression of the disease.
    switch( _DSstate){
        case (DS_SUS):{
            return true;
        }
        case ( DS_EL ):{
            progressELDS(rng);
            return true;
        }
        case ( DS_LL ):{
            progressLLDS(rng);
            return true;
        }
        case ( DS_A ):{
            progressADS(rng);
            return true;
        }
        case ( DS_TRDS):{
            progressTRDS_DS(rng);
            return true;
        }
        case ( DS_TRDR):{
            progressTRDR_DS(rng);
            return true;
        }
        case ( DS_REC ):{
            progressRECDS(rng);
            return true;
        }
        default:{
            throw logic_error(noDsStateErr);
        }
    }
}
bool Person::progressDR(mt19937 &rng){//depending on the current state, will model the progression of the disease.
    switch( _DRstate){
        case (DR_SUS):{
            return true;
        }
        case ( DR_EL ):{
            progressELDR(rng);
            return true;
        }
        case ( DR_LL ):{
            progressLLDR(rng);
            return true;
        }
        case ( DR_A ):{
            progressADR(rng);
            return true;
        }
        case ( DR_TRDS):{
            progressTRDS_DR(rng);
            return true;
        }
        case ( DR_TRDR):{
            progressTRDR_DR(rng);
            return true;
        }
        case ( DR_REC ):{
            progressRECDR(rng);
            return true;
        }
        default:{
            throw logic_error(noDrStateErr);
        }
    }
}
//
bool Person::progressADS(mt19937 &rng){//model diagnosis and treatment for DSTB
    double mort=returnMortalityTB(rng);
    if (rng_unif_decimal(rng)<=mort){//check death first as its an absorbing state
        _markedDead=true; //byebye
        _stats->record_nMortalityDS();
        _stats->record_vDurDSTB(TNOW()-_tActiveInfection);
        //save personal attributes
        _death_time=TNOW();
        _death_route=1; //natural mortality
        _death_dsState=_DSstate;
        _death_drState=_DRstate;
        return true;
    }
    //Spont resolution?
    if((_tProgressToRECDS_ADS>=0)&&(_tProgressToRECDS_ADS<=TNOW())){
        //        _tProgressToRECDS_ADS=-1;
        //        _stats->record_vDurDSTB(TNOW()-_tActiveInfection); shouldnt count this for spont resolution, duration is defined to treatment or death
        enterRECDS(rng,false);
        return true;
    }
    //
    modelCareVisits(rng);
    //
    return false;
}
bool Person::progressADR(mt19937 &rng){//model misDiag, trt for DSTB
    double mort=returnMortalityTB(rng);
    if (rng_unif_decimal(rng)<mort){//check death first as its an absorbing state
        _markedDead=true;//byebye
        _stats->record_nMortalityDR();
        _stats->record_vDurDRTB(TNOW()-_tActiveInfection);
        //save personal attributes
        _death_time=TNOW();
        _death_route=1; //natural mortality
        _death_dsState=_DSstate;
        _death_drState=_DRstate;
        return true;
    }
    //spont resolution?
    if((_tProgressToRECDR_ADR>=0)&&(_tProgressToRECDR_ADR<=TNOW())){
        //        _tProgressToRECDR_ADR=-1;
        _stats->record_vDurDRTB(TNOW()-_tActiveInfection);
        enterRECDR(rng,false);
        return true;
    }
    //
    modelCareVisits(rng);
    //
    return false;
}
//
bool Person::progressELDS(mt19937 &rng){
    //if person is on PT (both regimen) and PT is sccessful, there is a certain reduction in risk of prog to active TB; at the end of PT they will recover
    if (_bIsOnPt==true)
        if(_pt_successCode_latent==1 && (_pt_regimen==ISONAIZID || _pt_regimen==DELAMANID)){
            return false;
        }
    //----------------
    
    //progress to ADS?
    if ((_DRstate<4)&& //make sure that person is not actively infected or in trt with "DRTB" (state 4,5,6)
        (_tProgressToADS_ELDS>=0)&&(_tProgressToADS_ELDS<=TNOW())){
        //        _tProgressToADS_ELDS=-1;
        _stats->record_nIncDS_fastprog();
        enterADS(rng,false);
        return true;
    }
    //progress to LLDS?
    if ((_tProgressToLLDS_ELDS>=0)&&(_tProgressToLLDS_ELDS<=TNOW())){
        //        _tProgressToLLDS_ELDS=-1;
        enterLLDS(rng);
        return true;
    }
    return false;
}
bool Person::progressELDR(mt19937 &rng){
    //if person is on PT (only DELAMANID) and PT is sccessful, there is a certain reduction in risk of prog to active TB; at the end of PT they will recover
    if (_bIsOnPt==true)
        if( _pt_successCode_latent==1 && _pt_regimen==DELAMANID){
            return false;
        }
    //----------------
    
    //progress to ADR?
    if((_DSstate<4)&&//make sure that person is not actively infected or in trt with "DSTB" (state 4,5,6)
       (_tProgressToADR_ELDR>=0)&&(_tProgressToADR_ELDR<=TNOW())){
        //        _tProgressToADR_ELDR=-1;
        _stats->record_nIncDR_fastprog();
        enterADR(rng,false);//NewInfection
        return true;
    }
    //progress to LLDR?
    if((_tProgressToLLDR_ELDR>=0)&&(_tProgressToLLDR_ELDR<=TNOW())){
        //        _tProgressToLLDR_ELDR=-1;
        enterLLDR(rng);
        return true;
    }
    return false;
}
bool Person::progressLLDS(mt19937 &rng){
    //if person is on PT (both regimen) and PT is sccessful, there is a certain reduction in risk of prog to active TB; at the end of PT they will recover
    if (_bIsOnPt==true)
        if( _pt_successCode_latent==1 && (_pt_regimen==ISONAIZID || _pt_regimen==DELAMANID)){
            return false;
        }
    //----------------
    
    if (_DRstate>3)//if actively infected or in treatment for DR disease, we cant progress here
        return false;
    //progress to ADS?
    if ((_tProgressToADS_LLDS>=0)&&(_tProgressToADS_LLDS<=TNOW())){//can only progress if not active DR or under trt
        //        _tProgressToADS_LLDS=-1;
        _stats->record_nIncDS_slowprog();
        enterADS(rng,false);
        return true;
    }
    return false;
}
bool Person::progressLLDR(mt19937 &rng){
    //if person is on PT (only DELAMANID) and PT is sccessful, there is a certain reduction in risk of prog to active TB; at the end of PT they will recover
    if (_bIsOnPt==true)
        if( _pt_successCode_latent==1 && _pt_regimen==DELAMANID){
            return false;
        }
    //----------------
    
    if (_DSstate>3) //if actively infected or in treatment for DS disease, we cant progress here
        return false;
    //progress to ADR?
    if((_tProgressToADR_LLDR>=0)&&(_tProgressToADR_LLDR<=TNOW())){
        //        _tProgressToADR_LLDR=-1;
        _stats->record_nIncDR_slowprog();
        enterADR(rng,false);//New Infection
        return true;
    }
    return false;
}
//
bool Person::progressRECDS(mt19937 &rng){
    //if someone is onPT, they wont relapse.
    if (_bIsOnPt)
        if (_pt_regimen==ISONAIZID || _pt_regimen==DELAMANID) _tProgressToADS_RECDS=-1;
    //---------------
    
    if (_DRstate>3) //if actively infected or in treatment for DR disease, we cant progress here
        return false;
    //relapse to ADS?
    if ((_tProgressToADS_RECDS>=0) &&(_tProgressToADS_RECDS<=TNOW())) {
        //        _tProgressToADS_RECDS=-1;
        _stats->record_nIncDS_relapse();
        enterADS(rng,false);
        return true;
    }
    
    return false;
}
bool Person::progressRECDR(mt19937 &rng){
    //if someone is onPT, they wont relapse.
    if (_bIsOnPt )
        if( _pt_regimen==DELAMANID ) _tProgressToADR_RECDR=-1;
    //---------------
    
    if (_DSstate>3) //if actively ifected or in treatment for DS disease, we cant progress here
        return false;
    //relapse to ADR?
    if((_tProgressToADR_RECDR>=0)&&(_tProgressToADR_RECDR<=TNOW())){
        //        _tProgressToADR_RECDR=-1;
        _stats->record_nIncDR_relapse();
        enterADR(rng,false); //relapse
        return true;
    }
    return false;
}
//
bool Person::progressTRDS_DS(mt19937 &rng){
    //if the person has active TB but is misdiagnosed with LTB and is current on PT, they have to stop
    if (_bIsOnPt==true) getOffPT(rng);
    
    //model TB mortality for those failing trt-----------------
    if (_bIsGoingToFailTrt){
        double mort=returnMortalityTB(rng);
        if (rng_unif_decimal(rng)<= mort){
            _markedDead=true;//byebye
            _stats->record_nMortalityDS();
            _stats->record_vDurDSTB(TNOW()-_tActiveInfection);
            //save personal attributes
            _death_time=TNOW();
            _death_route=1; //natural mortality
            _death_dsState=_DSstate;
            _death_drState=_DRstate;
            return true;
        }}
    // Develop resistance: going to ADR
    if ((_tProgressToADR_TRDS>=0)&&(_tProgressToADR_TRDS<=TNOW())) {
        //        _tProgressToADR_TRDS=-1;
        _nTrtFailure_recent++; //still counts it as a trt failure
        _stats->record_nFailedDSTrt_DS();
        _stats->record_nIncDR_resist();
        //
        _stats->record_vDurDSTB(TNOW()-_tActiveInfection);
        
        //code the DR transmission based on original DS transmission:
        _nTransRouteDR=_nTransRouteDS+2; //_nTransRouteDS==0 or 1 >> _nTransRouteDR=2 (HH) or 3 (COM)
        _tTransDR=TNOW();
        //
        enterRECDS(rng, false);//DS is treated first
        enterADR(rng,false);//NEW DR infection occure
        return true;
    }
    // Failing trt: going back to ADS
    if ((_tProgressToADS_TRDS>=0)&&(_tProgressToADS_TRDS<=TNOW())) {
        //        _tProgressToADS_TRDS=-1;
        _nTrtFailure_recent++;
        _stats->record_nFailedDSTrt_DS();
        //        _stats->record_nIncDS_Failure();//we dont count trt failures as new incidence
        //
        enterADS(rng,true);
        return true;
    }
    //get recovered?
    if ((_tProgressToRECDS_TRDS>=0)&&(_tProgressToRECDS_TRDS<=TNOW())) {
        //        _tProgressToRECDS_TRDS=-1;
        _stats->record_vDurDSTB(TNOW()-_tActiveInfection);
        enterRECDS(rng,false);
        return true;
    }
    return false;
}
/* Checkpoint: In this model we dont model <100% specificity, so this shouldn't
 * happen. */
bool Person::progressTRDR_DS(mt19937 &rng){
    throw logic_error(sub100pSpecificityErr2);
}
bool Person::progressTRDS_DR(mt19937 &rng){
    //if the person has active TB but is misdiagnosed with LTB and is current on PT, they have to stop
    if (_bIsOnPt==true) getOffPT(rng);
    
    //model TB mortality among those failing treatment (all)
    if (_bIsGoingToFailTrt){
        double mort=returnMortalityTB(rng);
        if (rng_unif_decimal(rng)<= mort){
            _markedDead=true;//byebye
            _stats->record_nMortalityDR();
            _stats->record_vDurDRTB(TNOW()-_tActiveInfection);
            //save personal attributes
            _death_time=TNOW();
            _death_route=1; //natural mortality
            _death_dsState=_DSstate;
            _death_drState=_DRstate;
            return true;
        }}
    //1-Failing DS treatment?
    if ((_tProgressToADR_TRDS>=0)&&(_tProgressToADR_TRDS<=TNOW())) {
        //        _tProgressToADR_TRDS=-1;
        _nTrtFailure_recent++;
        _stats->record_nFailedDSTrt_DR();
        //        _stats->record_nIncDR_Failure();//we dont count trt failures toward new incidence
        //even failed DS treatment clears latent DS infections
        if ((_DSstate==DS_EL) || (_DSstate==DS_LL))  enterRECDS(rng, false);
        //
        enterADR(rng,true);//true: trt failure
        return true;
    }
    //2-Treatment is effective? a small percentage of DS-treatment work for DRTB too
    if ((_tProgressToRECDR_TRDS>=0)&&(_tProgressToRECDR_TRDS<=TNOW())) {
        //        _tProgressToRECDR_TRDS=-1;
        _stats->record_vDurDRTB(TNOW()-_tActiveInfection);
        //successful DS treatment clears latent DS infections
        if ((_DSstate==DS_EL) || (_DSstate==DS_LL))  enterRECDS(rng, false);
        //treat DRTB
        enterRECDR(rng,false);
    }
    return false;
}
bool Person::progressTRDR_DR(mt19937 &rng){
    //if the person has active TB but is misdiagnosed with LTB and is current on PT, they have to stop
    if (_bIsOnPt==true) getOffPT(rng);
    
    //model TB mortality among those failing treatment
    if (_bIsGoingToFailTrt){
        double mort=returnMortalityTB(rng);
        if (rng_unif_decimal(rng)<= mort){
            _markedDead=true;//byebye
            _stats->record_nMortalityDR();
            _stats->record_vDurDRTB(TNOW()-_tActiveInfection);
            //save personal attributes
            _death_time=TNOW();
            _death_route=1; //natural mortality
            _death_dsState=_DSstate;
            _death_drState=_DRstate;
            return true;
        }}
    //1-Failing DR treatment?
    if ((_tProgressToADR_TRDR>=0)&&(_tProgressToADR_TRDR<=TNOW())) {
        //        _tProgressToADR_TRDR=-1;
        _nTrtFailure_recent++;
        _stats->record_nFailedDRTrt_DR();
        //        _stats->record_nIncDR_Failure(); //we dont count trt failures toward new incidence
        //even failed DR treatment clears latent DS infections
        if ((_DSstate==DS_EL) || (_DSstate==DS_LL))  enterRECDS(rng, false);
        //
        enterADR(rng,true);//true: trt failure
        return true;
    }
    //2-Successful DR treatment?
    if ((_tProgressToRECDR_TRDR>=0)&&(_tProgressToRECDR_TRDR<=TNOW())) {
        //        _tProgressToRECDR_TRDR=-1;
        _stats->record_vDurDRTB(TNOW()-_tActiveInfection);
        //successful DR treatment clears latent DS infections
        if ((_DSstate==DS_EL) || (_DSstate==DS_LL))  enterRECDS(rng, false);
        //
        enterRECDR(rng,false);
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
/* !! since we should withhold prgress if the other disease is active (or under
 treatment), we have to recheck that the time of progression is not already
 past.
 // */
/*
 Model household contact tracing for the index case
 */
bool Person::model_hhct_indexCase(mt19937 &rng){ //model new HHCT and screening:
    if ((_hh == nullptr) ) throw logic_error(badHhStatus);
    
    //save personal attributes for the index case
    _hhct_indexCase=true;
    _hhct_indexCase_timeMarked=TNOW();
    _hhct_age=_age;
    _hhct_time=TNOW();
    _hhct_year=YNOW();
    _hhct_dsState=_DSstate;
    _hhct_drState=_DRstate;
    _hhct_dsTransTime=_tTransDS;
    _hhct_drTransTime=_tTransDR;
    _hhct_dsTransRoute=_nTransRouteDS;
    _hhct_drTransRoute=_nTransRouteDR;
    //these values are set previously and we pass them to other people who are screened in this hh
    //        _hhct_hhID
    //        _hhct_hhSize
    //        _hhct_hhPrevSUS_DSDR
    //        _hhct_hhPrevSUS_DS
    //        _hhct_hhPrevSUS_DR
    //        _hhct_hhPrevADS
    //        _hhct_hhPrevADR
    //        _hhct_hhPrevLDS
    //        _hhct_hhPrevLDR
    //        _hhct_nkidsLt6InHH
    //        _hhct_nkidsLt16InHH
    
    //model hhct
    if(_hh->getSize()>1){
        vector<shared_ptr<Person>> hhmembers=_hh->getMembers(); //vector of all hh members
        for (std::shared_ptr<Person> q: hhmembers){
            if (q->_bIsOnPt==true ) throw logic_error(badHhStatus); //this hh is getting screened twice or this person is coming from another hh, that shouldnt have dissolved?
            
            if((q->getId() != this->getId())){
                //set fix attributes for q:
                q->set_hhct_hhID(_hhct_hhID);
                q->set_hhct_hhSize(_hhct_hhSize); //1-true size
                q->set_hhct_hhPrevSUS_DSDR(_hhct_hhPrevSUS_DSDR);
                q->set_hhct_hhPrevSUS_DS(_hhct_hhPrevSUS_DS);
                q->set_hhct_hhPrevSUS_DR(_hhct_hhPrevSUS_DR);
                q->set_hhct_hhPrevADS(_hhct_hhPrevADS);
                q->set_hhct_hhPrevADR(_hhct_hhPrevADR);
                q->set_hhct_hhPrevLDS(_hhct_hhPrevLDS);
                q->set_hhct_hhPrevLDR(_hhct_hhPrevLDR);
                q->set_hhct_nkidsLt6InHH(_hhct_nkidsLt6InHH);
                q->set_hhct_nkidsLt16InHH(_hhct_nkidsLt16InHH);
                
                q->model_hhct_hhMember(rng);
                
                if ((q->isHhct_hhMember()==true) && (q->isHhct_indexCase()==true))
                    throw logic_error(badHhStatus);
            }}
    }
    
    return false;
}
/*
 Model household contact tracing among household members
 */
bool Person:: model_hhct_hhMember(mt19937 &rng){
    //save personal attributes for the hh member
    _hhct_hhMember=true;
    _hhct_hhMember_timeMarked=TNOW();
    _hhct_age=_age;
    _hhct_time=TNOW();
    _hhct_year=YNOW();
    _hhct_dsState=_DSstate;
    _hhct_drState=_DRstate;
    _hhct_dsTransTime=_tTransDS;
    _hhct_drTransTime=_tTransDR;
    _hhct_dsTransRoute=_nTransRouteDS;
    _hhct_drTransRoute=_nTransRouteDR;
    
    //    cout<<"member ID= "<<_id<<"   DS: "<<  _DSstate<<"  "<<_DRstate<< " at hhct, time "<<TNOW()<<endl;
    
    //We exclude those who are under treatment from screening
    if(_DSstate==DS_TRDS || _DSstate==DS_TRDR || _DRstate==DR_TRDR || _DRstate==DR_TRDS)
        return false;
    
    //Screen everyone else for active TB
    //diagnosed with ADS?
    if(_DSstate== DS_A && rng_unif_decimal(rng)<__hhct_testSens_atb) {
        _tDiagnosis_DS=TNOW();
        _hhct_diagnosedWithADS=true;
        _stats->record_nHhct_DS_atbDiagnosed();
        enterTRDS_DS(rng); //receiving correct treatment
        return false;
    }
    //diagnosed with ADR?
    if(_DRstate==DR_A && rng_unif_decimal(rng)<__hhct_testSens_atb) {
        _tDiagnosis_DR=TNOW();
        _hhct_diagnosedWithADR=true;
        _stats->record_nHhct_DR_atbDiagnosed();
        enterTRDR_DR(rng);//receiving correct treatment
        return false;
    }
    //(proceed if person is not actively infected or active infection was missed)
    //model PT for kids
    double pRecivePT=0;
    //baseline scenario:
    //    if (_age<16)  pRecivePT= __hhct_ptCoverage_0To15;
    //    if (_age<6)    pRecivePT= __hhct_ptCoverage_0To5;
    //TODO: @JP: here are the two scenarios: we only need to model one of them at a time
    //scenario 1: PT for <5 only
    //if (_age<6)    pRecivePT= 1;
    //scenario 2: PT for <15 only
    if (_age<16)  pRecivePT= 1;
    
    if(rng_unif_decimal(rng)<pRecivePT){
        getOnPT(rng);
        return true;
    }
    
    //otherwise, screen for LTB (including those with ATB who were isdiagnosed)
    //baseline
    //    if( !(_DSstate==DS_SUS && _DRstate==DR_SUS) &&   // the same as: (_DSstate==DS_EL || _DSstate==DS_LL || _DSstate==DS_A || _DSstate==DS_REC ||    _DRstate==DR_EL || _DRstate==DR_LL || _DRstate==DR_A || _DRstate==DR_REC )
    //       (rng_unif_decimal(rng)<__hhct_testSens_ltb) ){
    //        if (_DSstate>DS_SUS){
    //            _hhct_diagnosedWithLRDS=true;
    //            _stats->record_nHhct_DS_ltbDiagnosed();}
    //        if (_DRstate>DR_SUS){
    //            _hhct_diagnosedWithLRDR=true;
    //            _stats->record_nHhct_DR_ltbDiagnosed();}
    //        //
    //        getOnPT(rng);
    //        return true;
    //    }
    
    return false;
}


/*
 Start prevantive therapy
 */
void Person::getOnPT(mt19937 &rng){
    _stats->record_nPt_started();//annual number of people starting pt
    //
    _pt_regimen=__pt_regimen;
    _pt_start_time=TNOW();
    _pt_start_year=YNOW();
    _pt_end_time=TNOW()+__pt_duration;
    _bIsOnPt=true;
    _bHasBeenOnPt=true;
    _pt_successCode_latent=0; //pt fails among those with active TB and PLACEBO arm
    _pt_successCode_rec=0;
    
    //
    //kids may be susceptible for DS/DR
    //we model both suceess codes in case that someone on pt moves from one state to another (SUSDS->ELDS)
    if (_pt_regimen==ISONAIZID || _pt_regimen==DELAMANID){
        // (_DSstate==DS_EL || _DSstate==DS_LL || _DRstate==DR_EL || _DRstate==DR_LL)
        if (rng_unif_decimal(rng)<__pt_probSuccess_latent) _pt_successCode_latent=1;
        else _pt_successCode_latent=0;
        //  applicable for (_DSstate==DS_REC || _DRstate==DR_REC)
        if (rng_unif_decimal(rng)<__pt_probSuccess_rec) _pt_successCode_rec=1;
        else _pt_successCode_rec=0;
    }
}
/*
 End PT and model recovery among LLTB patients if PT was successful
 */
void Person::getOffPT(mt19937 &rng){
    _bIsOnPt=false;//we retain all other pt parameters for reporting
    _stats->record_nPt_completed();//annual number of people completing pt
    
    //For those in latent state
    if (_DSstate==DS_EL || _DSstate==DS_LL || _DRstate==DR_EL || _DRstate==DR_LL){
        if (_pt_successCode_latent==1){ //move to recovered state
            
            if (_pt_regimen==PLACEBO) throw logic_error(errPlaceboPt); //placebo cant be successful
            
            //isonizid:
            if (_pt_regimen==ISONAIZID){
                if (_DSstate==DS_EL || _DSstate==DS_LL){ //ELDS/LLDS
                    _tProgressToADS_ELDS=-1;
                    _tProgressToLLDS_ELDS=-1;
                    _tProgressToADS_LLDS=-1;
                    enterRECDS(rng,true);
                }}
            //delamanid:
            if (_pt_regimen==DELAMANID){ //delamanid
                if (_DSstate==DS_EL || _DSstate==DS_LL) {//ELDS/LLDS
                    _tProgressToADS_ELDS=-1; //reset these parameters
                    _tProgressToLLDS_ELDS=-1;
                    _tProgressToADS_LLDS=-1;
                    enterRECDS(rng,true);
                }
                if (_DRstate==DR_EL || _DRstate==DR_LL) {//ELDS/LLDS
                    _tProgressToADR_ELDR=-1;
                    _tProgressToLLDR_ELDR=-1;
                    _tProgressToADR_LLDR=-1;
                    enterRECDR(rng,true);
                }
            }
        }}
    //For those in recovered state
    //if (_DSstate==DS_REC || _DRstate==DR_REC) //they stay there the same way
    
    //if PT was unsuccessful, it wont change the natural history
}

vector<long double> Person::return_pt_individualLevelOutputs(){
    vector<long double> vRes;
    vRes.push_back(YNOW());
    vRes.push_back(_id); //unique ID
    
    //current attributes
    vRes.push_back(_age);
    // vRes.push_back(_hh->getId()); //can generate an error if this person doesnt belong to a hh in 2030
    vRes.push_back(_death_time ); //if they died between 2020 and 2050, we save their death info
    vRes.push_back(_death_route );
    vRes.push_back(_death_dsState );
    vRes.push_back(_death_drState );
    vRes.push_back(_DSstate);//current DS state
    vRes.push_back(_DRstate); //current DR state
    vRes.push_back(_lastDSstate); //last DS state (this will allow us to find out if they experienced fastProg, reactivation, relapse, etc)
    vRes.push_back(_lastDRstate);
    
    //household attributes saved at the time of HHCT
    vRes.push_back(_hhct_indexCase);
    vRes.push_back(_hhct_indexCase_timeMarked);
    vRes.push_back(_hhct_hhMember);
    vRes.push_back(_hhct_hhMember_timeMarked);
    
    vRes.push_back(_hhct_time);
    vRes.push_back(_hhct_year);
    vRes.push_back(_hhct_hhID);
    vRes.push_back(_hhct_hhSize);
    vRes.push_back(_hhct_nkidsLt6InHH);
    vRes.push_back(_hhct_nkidsLt16InHH);
    
    vRes.push_back(_hhct_hhPrevSUS_DSDR);
    vRes.push_back(_hhct_hhPrevSUS_DS);
    vRes.push_back(_hhct_hhPrevSUS_DR);
    vRes.push_back(_hhct_hhPrevLDS);
    vRes.push_back(_hhct_hhPrevLDR);
    vRes.push_back(_hhct_hhPrevADS);
    vRes.push_back(_hhct_hhPrevADR);
    
    //personal attributes saved at the time of HHCT
    vRes.push_back(_hhct_age);
    vRes.push_back(_hhct_dsState);
    vRes.push_back(_hhct_drState);
    vRes.push_back(_hhct_diagnosedWithADS);
    vRes.push_back(_hhct_diagnosedWithADR);
    vRes.push_back(_hhct_diagnosedWithLRDS);
    vRes.push_back(_hhct_diagnosedWithLRDR);
    vRes.push_back(_hhct_dsTransTime);
    vRes.push_back(_hhct_drTransTime);
    vRes.push_back(_hhct_dsTransRoute);
    vRes.push_back(_hhct_drTransRoute);
    
    
    //Personal attributes saved at the time of PT start
    vRes.push_back(_bHasBeenOnPt);
    vRes.push_back(_bIsOnPt);
    vRes.push_back(_pt_start_time);
    vRes.push_back(_pt_start_year);
    vRes.push_back(_pt_end_time);
    vRes.push_back(_pt_regimen);
    vRes.push_back(_pt_successCode_latent);
    vRes.push_back(_pt_successCode_rec);
    
    //Events recorded for the first time after starting PT
    vRes.push_back(_postPt_first_dsInc_time); //time of first DS incidence after PT
    vRes.push_back(_postPt_first_dsInc_lastDsState);
    vRes.push_back(_postPt_first_dsInc_lastDrState);
    vRes.push_back(_postPt_first_dsInc_transRoute);
    vRes.push_back(_postPt_first_dsInc_transTime);
    
    vRes.push_back(_postPt_first_drInc_time);
    vRes.push_back(_postPt_first_drInc_lastDsState);
    vRes.push_back(_postPt_first_drInc_lastDrState);
    vRes.push_back(_postPt_first_drInc_transRoute);
    vRes.push_back(_postPt_first_drInc_transTime);
    
    return vRes;
}


void Person::debug(){
    //    if(_DSstate>0 && _nTransRouteDS<0)
    //        cout<<"stop";
    //if(_postPt_first_dsInc_time>-1 && _postPt_first_dsInc_transRoute<0)
    //    cout<<"stop";
    //if(_postPt_first_dsInc_time>-1 && _postPt_first_dsInc_lastDsState==0)
    //cout<<"stop";
    //    if(_DRstate>0 && _nTransRouteDR<0)       cout<<"stop"; //this can happen
    //    if(_postPt_first_drInc_time>-1 && _postPt_first_drInc_transRoute<0)//           cout<<"stop";
    //    if(_postPt_first_drInc_time>-1 && _postPt_first_drInc_lastDrState==0)//              cout<<"stop";
    
}
