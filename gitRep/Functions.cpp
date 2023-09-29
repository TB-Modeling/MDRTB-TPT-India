//
//  Functions.cpp
//  src
//
//  Created by Trinity on 8/4/15.
//  Copyright (c) 2015 Trinity. All rights reserved.
//



#include "Driver.h"
#include "Population.h"
#include "Household.h"
#include "Person.h"
#include "Functions.h"
#include "GlobalVariables.h"

#include <sstream>
#include <vector>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <ctgmath>
#include <memory>
#include <thread>
#include <mutex>
#include <random>

using namespace std;
using namespace globalVariables;

//For the beta/uniform distribution draws
#include <boost/random.hpp>
 
namespace functions {
    //private:
    //This is a helper for the outputVector function
    unique_ptr<ofstream> _setupOutputVectorFile ( const string filename, const bool overwrite) {
        
        const auto base = overwrite ? ios_base::out : ios::app;
        
        //Rewrite using shared pointer removes the need for the calling function to clean up
        //Changed to unique_ptr because the ofstream destructor calls .close()
        return unique_ptr<ofstream> ( new ofstream (filename, base) );
    }
    
    void writeHeadFile( int param_count, const char **params ) {
        stringstream filename;
        filename << "head";
        //Skip the executable name; start with i = 1
        for (auto i = 1; i < param_count; i++ ) {
            filename << "_" << params[i];
        }
        writeFile("Created", filename.str(), true);
        cout << filename.str() << " written." << endl;
    }

    void writeSeedFile( std::shared_ptr<ThreadID> tID, unsigned seed ) {
        stringstream string_seed, filename;
        filename << "seed_" << tID->as_string();
        string_seed << seed;
        writeFile(string_seed.str(), filename.str(), true);
    }

    //Helper for creating ThreadID's
    std::shared_ptr<ThreadID> make_ThreadID (std::initializer_list<MI_ELEMENT> l) {
        return std::shared_ptr<ThreadID> (new ThreadID (l));
    }

    /*VMAP - 
     * This is a value mapping that maps 'value', which is a number in the range [minI, maxI],
     * into the range defined by [minO, maxO]. Notice the inclusive square brackets.
     */
    double vmap( double value, double minI, double maxI, double minO, double maxO) {
        const double outSize = maxO - minO;
        const double inSize = maxI - minI;
        
        return minO + (value - minI) * outSize / inSize;
    }

    std::pair<double, double> getUniValues ( const unsigned val, const unsigned size, const double zero, const double one ) {
        //val - number between 1 and size inclusive, produced by the hypercube
        //size - the number of divisions of the hypercube
        //zero - the lowest possible number
        //one - the highest possible number
        //
        //This function takes a value 'val' and a size 'size' and produces a pair of numbers to be
        //used as ends of a uniform distribution, from which we pull the value we are looking for.
        //
        //There are three situations:
        //Val is 1 - we want the lowest value of the distrubtuion to be 'zero', the highest to be 1 / 'size'
        //Val is size - we want the lowest value of the distribution to be ('val' - 1) / size, the highest is
        //              'one'
        //Val is anything else - we want the lowest value to be 'val' - 1 / size, and the highest to be 'val'/size
        
        const auto d_size = static_cast<double>(size);
        //This is the most common result; set it first
        double highValue=val / d_size, lowValue=(val - 1) / d_size;
        if (val == 1) {
            highValue = val / d_size;
            lowValue = zero;
            //Lowest value; between value and almost_zero
        } else if (val == size ) {
            lowValue = (val - 1) / d_size;
            highValue = one;
            //Highest value; between value and almost_one
        }
        return std::make_pair (lowValue, highValue);
    }

    
    //VECTOR MEMBERS------------------------------------------------------------------------------------------------------------------------------------
    int findIndex(const Person * p,const vector<Person> &vec){ //returns the index of person *p in a given queue
        const auto N=vec.size();
        for(auto i =0u;i<N;i++) {            
            if(&vec[i]== p) return i;        
        }
        return -1;
    }
    
    int findIndex(const Person * p,const vector<Person*> &vec){//returns the index of person *p in a given queue
        const auto N=vec.size();
        for(auto i =0u;i<N;i++) {           
            if(vec[i]== p) return i;        
        }
        return -1;
    }

    
    //returns the queue member at a random index
    Person * returnRandomMember(mt19937 &rng, const vector<Person*> &vec){
        const auto r = rng_unif_int(rng, (int) vec.size());
        return vec[r];
    }
    
    Person * returnRandomMember(mt19937 &rng,const vector<vector<Person*>> &vec){//retunrs the queue member at a specific index
        const auto r1 = rng_unif_int(rng,(int) vec.size());
        const auto r2=rng_unif_int(rng,(int) vec[r1].size());
        return vec[r1][r2];
    }
    
    int returnRandomMember(mt19937 &rng,vector<int> &vec){//retunrs the queue member at a specific index
        const auto r1 = rng_unif_int(rng,(int) vec.size());
        return vec[r1];
    }
    
    //------------------------------------------------------------------------------------------------------------------------------------
    void writeFile(const string s, const string filename, const bool overwrite){
        
        unique_ptr<ofstream> ofile = _setupOutputVectorFile ( filename, overwrite );
        *ofile<<s << endl;
        ofile->close();
    }
    
    //------------------------------------------------------------------------------------------------------------------------------------

    double returnRand_triang(mt19937 &rng, const double mode){//retunrs a double random number between [0,1] from trang dist
        const double r= rng_unif_decimal(rng); //uniform rand num
        //        double F = mode;// (c - a) / (b - a);
        if (r <= mode)
            return   (sqrt(r * mode)); //a + sqrt(U * (b - a) * (c - a));
            
        return   (1- sqrt((1-r)* (1-mode)));  //b - sqrt((1 - U) * (b - a) * (b - c));
    }

    double returnRand_triang(mt19937 &rng, const double a, const double c,const double b){//returns a double random number between [0,1] from trang dist
        const double U= rng_unif_decimal(rng); //uniform rand num
        const double F =  (c - a) / (b - a);
        if (U <= F)
            return   (a + sqrt(U * (b - a) * (c - a)));

        return   (b - sqrt((1 - U) * (b - a) * (b - c)));
    }
    int returnRand_multinom(mt19937 &rng,const vector<double> &vCumProbs){
        const double r=rng_unif_decimal(rng); //(0-1)
        auto idx=0;
        for (const auto &p : vCumProbs){
            if (r<=p) return idx;
            else idx++;
        }
        cerr << "Cumulative probability error: Tried to select random index from "
            "malformed cumulative probaiblity vector; no member of vector was"
            " equalto '1':" << endl;
        for (const auto &x:vCumProbs)
            cerr << x << " ";
        cerr << endl;
        throw logic_error("Cumulative Probability Error");
    }
    int returnMin(vector<int> &vec){
        int min=vec[0];
        for (auto &i:vec)
            if(i<min) min=i;
        return min;
    }
    int returnMax(vector<int> &vec){
        int max=vec[0];
        for (int i:vec)
            if(i>max) max=i;
        return max;
    }
    vector<int> returnRandomSample(mt19937 &rng,const int sampleSize, const int popSize){
        vector<int> vec;
        for (auto i=0;i<popSize;i++) vec.push_back(i);
        vector<int> res;
        for (auto i=0;i<sampleSize;i++){
            const auto r=rng_unif_int(rng,(int) vec.size());
            res.push_back(vec[r]);
            vec[r]=vec.back();
            vec.pop_back();
        }
        return res;
    }
    double returnLinearGrowth(const double min, const double max, const double dur, const double indx){
        const double res= (indx)*(max-min)/(dur);
        //cout<<res<<endl;
        return res;
    }
    //------------------------------------------------------------------------------------------------------------------------------------
    //VECTOR OPERATIONS ---------------------------------
    /*
     *double vectorMedian( vector<double> vec){ //In this case we want a copy; otherwise the original vector will be sorted
     *    sort(vec.begin(), vec.end());
     *    int n=vec.size()/2;
     *    return vec[n];
     *}
     */
    vector<double> vectorRelDiffPercent( const vector<double> &vec, const double dom){//computes the the Percent difference of vec1 and vec2 elemenst relative to vec2
        vector<double> res;
        const auto n=vec.size();
        for(auto i=0u; i<n;i++)
            res.push_back(100*(vec[i]-dom)/dom);
        return res;
    }
    vector<double> vectorSummaryStat( vector<double> vec){//In this case we want a copy; otherwise the original vector will be sorted
        vector<double> res;
        sort(vec.begin(), vec.end());
        int n=vec.size()/2;
        res.push_back(vec.front());//min
        res.push_back(vec.back());//max
        res.push_back(vectorAve(vec));//mean
        res.push_back(vec[n]);//median
        return res;
    }
    
    vector<double> returnCI_proportion(const vector<double> &vec){
        const double p=vectorAve(vec);
        const double n=vec.size();
        const double std=sqrt(p*(1-p)/n);
        vector<double> res={p-1.96*std,p+1.96*std};
        return res;
    }
    
    vector<double> returnCI_value(const vector<double> &vec){
        const double m=vectorAve(vec);//sample mean
        const double n=vec.size();
        double sum=0;
        for (const auto &i : vec){
            sum=sum+(i-m)*(i-m);
        }
        const double Sd=sqrt(sum/n); //sample std
        const double t=tDIST_05[n - 1]; //value from the t-distibution
        vector<double> res={m-t*Sd, m+t*Sd};
        return res;
    }
    
    int isWithinMeanCI_value(const vector<double> &vec,const double val){//checks wheather the given value is within the CI around the sample mean
        const double m=vectorAve(vec);//sample mean
        const double n=vec.size();
        double sum=0;
        for (const auto& i : vec){
            sum=sum+(i-m)*(i-m);
        }
        const double Sd=sqrt(sum/n); //sample std
        const double t=tDIST_05[n - 1]; //value from the t-distibution
        //
        if ((val> (m-t*Sd))&& (val< (m+t*Sd))) return 2;
        if (val< (m-t*Sd)) return 1;
        return 3;
    }
    
    int isWithinMeanCI_proportion(vector<double> &vec,const double val){
        const double p=vectorAve(vec);
        const double n=vec.size();
        const double std=sqrt(p*(1-p)/n);
        if ((val>(p-1.96*std))&& (val<(p+1.96*std))) return 2;
        if (val<(p-1.96*std)) return 1;
        return 3;
    }
    
    const std::string currentDateTime() {
        const time_t     now = time(0);
        struct tm  tstruct;
        char       buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
        return buf;
    }
    //====================================================================================================================================
    
/*
 *    //Beta Sampling
 *    void substitute_vars (std::mt19937 &rng, const std::vector<globalVariables::DistParam> &list, shared_ptr<Params> mp) {
 *        
 *        //Substitute values contained within the double_map with
 *        //random variables picked from a certain distribution.
 *        //A list of the variables, their shape values and scale factor
 *        //can be found in GlobalVariables::dist_params
 *
 *        This method will change the stored _mp object within Driver
 *        
 *
 *        using BoostBeta = boost::random::beta_distribution<double>;
 *
 *        if (__bPRINT)
 *            std::cout << "Dist Shuffle\n";
 *
 *        //Create the generic beta distribution to be used
 *        BoostBeta beta_dist;
 *
 *        for (const auto &entry : list) {
 *            switch (entry.dt) {
 *                case globalVariables::DistType::BETA:
 *                    {   //explicit block to denote the limited scope of this option
 *                        //Create a param object with the proper alpha and beta
 *                        BoostBeta::param_type beta_param ( entry.shape1, entry.shape2 );
 *
 *                        //Set the beta_dist parameters to alpha and beta
 *                        beta_dist.param(beta_param);
 *
 *                        //Set the contained variable to the new random variable
 *                        //This will fail if entry.name is not already in the
 *                        //double_map
 *                        mp->setD(entry.name, beta_dist(rng) / entry.scale);
 *                    }
 *                    break;
 *                case globalVariables::DistType::UNIFORM:
 *                    {
 *                        std::uniform_real_distribution<> uniform_dist(entry.shape1, entry.shape2);
 *                        mp->setD(entry.name, uniform_dist(rng)/ entry.scale);
 *                        break;
 *                    }
 *                case globalVariables::DistType::LOGNORMAL:
 *                    {
 *                        std::normal_distribution<double> normal_dist(entry.shape1, entry.shape2);
 *                        mp->setD(entry.name, exp(normal_dist(rng) / entry.scale));
 *                        break;
 *                    }
 *            }
 *        }
 *    }
 */
}
