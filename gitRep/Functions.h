//
//  Functions.h
//  src
//
//  Created by Trinity on 8/4/15.
//  Copyright (c) 2015 Trinity. All rights reserved.
//

#ifndef __MDRTB_FUNCTIONS_H
#define __MDRTB_FUNCTIONS_H

#include "Population.h"
#include "Household.h"
#include "Person.h"
#include "GlobalVariables.h"
#include "ThreadID.hpp"

#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <ctgmath>
#include <algorithm>
#include <iterator>
#include <memory> //for smart pointers
#include <random>
#include <thread>
#include <mutex>

using namespace std;
using namespace globalVariables;
using std::gamma_distribution;
using std::geometric_distribution;
using std::mt19937;
using std::uniform_int_distribution;
using std::uniform_real_distribution;

namespace functions {
//private:
//This is a helper for the outputVector function
unique_ptr<ofstream> _setupOutputVectorFile(
                                            const string filename,
                                            const bool overwrite);

void writeHeadFile( int param_count, const char **params );
void writeSeedFile( std::shared_ptr<ThreadID> tID, unsigned seed );
void writeFile(const string s, const string filename, const bool overwrite);

//Helper for making ThreadID's
std::shared_ptr<ThreadID> make_ThreadID(std::initializer_list<MI_ELEMENT>);

/* Pure random number generation -------------------------------------------------------------------------------------------------------------------- */
/* Random number of attempts to succeed along a geometric distribution
 *
 * @param rng: Random number generator
 * @param probabilityOfSuccess: The probability of each attempt to succeed
 * @param includeSuccessAttemptInTotal: Include the single access attempt
 * in the total returned? Default behavior is 'true'.
 *
 * @return int: Number of attempts until success actually occurred while
 * simulating failure and success along geometric distribution. Equivalent
 * to the number of failures only if includeSuccessAttemptInTotal is
 * 'false', else equivalent to number of 'failures + 1' ('1' being
 * representative of the successful attempt).
 * */
inline int rng_geo(
                   mt19937 &rng,
                   const double probabilityOfSuccess,
                   const bool includeSuccessAttemptInTotal = true
                   ) {
    if (probabilityOfSuccess == 0)
        return 1000000000; //we return a large positive number so that the time of event is set for a long time from now; use 1,000,000,000
    geometric_distribution<int> dist(probabilityOfSuccess);
    auto nFailuresUntilSuccess = dist(rng);
    int nAttempts = !includeSuccessAttemptInTotal ? nFailuresUntilSuccess : nFailuresUntilSuccess + 1;
    return nAttempts;
}
//this is for generating random uniform number [0-1):
inline double rng_unif_decimal(mt19937 &rng) {
    uniform_real_distribution<> dist(0, 1);
    return dist(rng);
}


/* this is for generating random integer number [0 to i) or [0 - i]:
 * @param closedInterval: if true, will be set to have a closed interval between [0-i]
 * */
inline int rng_unif_int(mt19937 &rng, const int i, const bool closedInterval=false) {
    if (i==0) 
        return 0;
    auto end = closedInterval ? i : i-1;
    uniform_int_distribution<> dist(0, end); //uniformly distributed on the closed interval [a, b]
    return dist(rng);
}

/* Gamma RNG
 * For floating-point 'shape', the value obtained is the sum of 'shape'
 * independent exponentially distributed random variables, each of which
 * has a mean of 'scale'.
 * */
inline double rng_gamma(
                        mt19937 &rng,
                        const double shape,
                        const double scale
                        ) {
    gamma_distribution<double> dist(shape, scale);
    return dist(rng);
}

//VECTOR MEMBERS------------------------------------------------------------------------------------------------------------------------------------
int findIndex(Person * p,vector<Person> vec);
int findIndex(Person * p,vector<Person*> vec);

Person * returnRandomMember(mt19937 &rng,vector<Person*> vec);
Person * returnRandomMember(mt19937 &rng,vector<vector<Person*>> vec);

int returnRandomMember(mt19937 &rng,vector<int> vec);
//Vector operations----------------------------------------------------------------------------------------------------------------------------------------
template <typename T>
vector<double> returnDist(const vector<T> vec, const vector<double> vBins){//bins[b0,b1,...bn]   counts[<=b0, b0<x<=b1,... bn-1 to bn, >bn]
    auto bins= vBins.size();
    vector<double> counts(bins+1,0);
    for (const auto& val : vec){
        auto idx=-1;
        for (auto j=0u;j<bins;j++){
            if (val<= vBins[j]) {
                idx=j;
                break;
            }}
        if (idx>-1)
            counts[idx]++;
        else
            counts.back()++;//biggest container last
    }
    auto N=vec.size();
    for (auto i=0u;i<counts.size();i++)
        counts[i]=counts[i]/N;
    return counts;
}

vector<double> vectorRelDiffPercent(const vector<double> &vec, const double dom);
vector<double> vectorSummaryStat( vector<double> vec);

vector<double> checkForNonZeroDifference(const vector<double> &vec);
//
double returnRand_triang(mt19937 &rng, const double mode);//retunrs a double random number between [0,1] from trang dist
double returnRand_triang(mt19937 &rng, const double a, const double c,const double b);//triang[a.c.b]
int returnRand_multinom(mt19937 &rng, const vector<double> &vCumProbs);//returns a random index associated with probabilities in
int returnMin(const vector<int> &vec);
int returnMax(const vector<int> &vec);
vector<int> returnRandomSample(mt19937 &rng,const int sampleSize, const int popSize);
double returnLinearGrowth(const double min, const double max, const double dur, const double indx);

vector<double> returnCI_proportion(const vector<double> &vec);//returns the CI around proportion value
vector<double> returnCI_value(const vector<double> &vec);//return CI around a number value
int isWithinMeanCI_value(const vector<double> &vec,const double val);//checks wheather the given value is within the CI around the sample mean
int isWithinMeanCI_proportion(const vector<double> &vec, const double val);

double vmap( double , double , double , double , double );
std::pair<double, double> getUniValues ( const unsigned, const unsigned, const double, const double );
/*
  Method to replace some values with distributed random
  variables.  The specific variables, the distribution type
  and the shape parameters are defined in:

  globalVariables::dist_params

  which is a vector of globalVariables::DistParam
*/

void substitute_vars (std::mt19937 &, const std::vector<globalVariables::DistParam> &, shared_ptr<Params>); 
const std::string currentDateTime();
//  vector<Params> returnRandomScenarioScenarios(mt19937 &rng, int sampleSize,string fileName);
// void thread_calibDSTB(mt19937 &rng, int id, Params * scenario, int reps, vector<vector<vector<double>>> &vvvResults,std::mutex &simLock);
// void multiTread_calibrateDSTB(mt19937 &rng, int sampleSize, vector<double> vCalibTarget, int nReps, double finalPrecision, double increment);
//------------------------------------------------------------------
//GENERAL
//------------------------------------------------------------------

//This was originally in Functions.cpp but we had a double copy and an int copy,
//which usually means it's a good candidate for templating.
template <typename T>
vector<vector<T>> vectorChangeElements_OneByOne(const vector<T> &vec,const T inc){//take an index element and increment it, then inc all other values relative to that one
    vector<vector<T>> res;
    const auto N=vec.size();
    for(auto i=0;i < N;i++){
        vector<T> temp=vec;
        temp[i]=temp[i]*(1+inc);
        res.push_back(temp);
        //
        temp=vec;
        temp[i]=temp[i]*(1-inc);
        res.push_back(temp);
    }
    return res;
}

template <typename T>
void validateVectorOperations(const vector<T> &vec1, const vector<T> &vec2){
    if (vec1.size()!=vec2.size()){
        cout<<"vectorOperationSizeError:"<<endl;
        cout << "- vec1.size(): " << vec1.size() << endl;
        cout << "- vec2.size(): " << vec2.size() << endl;
        throw logic_error(
            "vectorOperationSizeError: vectors don't have the same size!");
    }
}

template <typename T>
void vectorReset( vector<T> &vec){
    fill(vec.begin(),vec.end(),0);
}
template <typename T>
void vectorReset( vector<vector<T>> &vec){
    vector<T> temp(vec[0].size(),0);
    auto R=vec.size();
    vec.clear();
    for (auto i=0;i<R;i++){
        vec.push_back(temp);
    }
}
template <typename T>
vector<vector<T>> vectorTranspose(const vector<vector<T>> &vec){
    vector<vector<T>> res;
    auto rows=vec.size();
    auto cols=vec[0].size();
    for (auto i=0;i<cols;i++){
        vector<T> temp;
        for(auto j=0;j<rows;j++){
            temp.push_back(vec[j][i]);
        }
        res.push_back(temp);
    }
    return res;
}
template <typename T>
vector<T> vectorReturnCol(const vector<vector<T>> &vec,const int colId){//return type is double
    vector<T> temp;
    for(const auto &vv:vec){
        temp.push_back(vv[colId]);
    }
    return temp;
}

//------------------------------------------------------------------
//SUM: summation of vector's components
//------------------------------------------------------------------
template <typename T>
T vectorSum( const vector<T> &vec){
    T sum=0;
    for (const auto &inner: vec){
        sum = sum+inner;
    }
    return  sum;
}
template <typename T>
T vectorSum( const vector<T> &vec, const int i, const int j){
    T sum=0;
    for (auto n=i; n<j;n++){
        sum = sum+vec[n];
    }
    return  sum;
}
template <typename T>
T vectorSum( const vector<vector<T>> &vec){
    T sum=0;
    for (const auto &outer: vec){
        for (const auto &inner: outer){
            sum = sum+inner;
        }
    }
    return  sum;
}
template <typename T>
vector<T> vectorSumRows(const vector<vector<T>> &vec){//return type is double
    T cols=vec[0].size();
    //sum all rows
    vector<T> vSum(cols,0);
    for (const auto &outer: vec){
        // transform(vSum.begin(), vSum.end(), outer.begin(), vSum.end(), plus<I>());
        for(auto i=0;i<outer.size();i++){
            vSum[i] += outer[i] ;
        }
    }
    return vSum;
}
template <typename T>
vector<T> vectorSumCols(const vector<vector<T>> &vec){
    auto rows=vec.size();
    auto cols=vec[0].size();
    //sum all cols
    vector<T> vSum;
    for (const auto &outer: vec){
        T sum=0;
        for (const auto &inner:outer){
            sum += inner;
        }
        vSum.push_back(sum);
    }
    return vSum;
}
//------------------------------------------------------------------
//DIVISION/MULTIPLICATION: dividing/multiplying vector by an external value or vector
//------------------------------------------------------------------
template <typename T>
vector<T> vectorDivision( const vector<T> &vec, const int dom){
    T n=dom;
    vector<T>  res;
    for (const auto &num:vec){
        res.push_back(num/n);
    }
    return res;
}
template <typename T>
vector<vector<T>> vectorDivision( const vector<vector<T>> &vec, const int dom){
    vector<vector<T>> res;
    T n=dom;
    for (const auto &outer:vec){
        vector<double> temp;
        for (const auto &num:outer){
            temp.push_back(num/n);
        }
        res.push_back(temp);
    }
    return res;
}
template <typename T>
vector<T> vectorDivision( const vector<T> &nom, const vector<T> &dom){
    if (nom.size()!=dom.size()) cout<<"Vector devision failed: not the same size"<<endl;
    vector<T>  res;
    auto N=nom.size();
    for (auto i=0;i<N;i++){
        res.push_back(nom[i]/dom[i]);
    }
    return res;
}
template <typename T>
vector<T> vectorMultiplication(const vector<T> &vec, const double n){
    vector<T>  res;
    for (const auto &num:vec){
        res.push_back(num*n);
    }
    return res;
}
template <typename T>
vector<vector<T>> vectorMultiplication( const vector<vector<T>> &vec, const double n){
    vector<vector<T>>  res;
    int size=vec.size();
    for (auto i=0;i<size;i++){
        vector<double> temp;
        for (const auto &num:vec[i]){
            temp.push_back(num*n);
        }
        res.push_back(temp);
    }
    return res;
}
//------------------------------------------------------------------
//SUBSTRACTION:
//------------------------------------------------------------------
template <typename T>
vector<T> vectorSubstraction( const vector<T> &vec1, const vector<T> &vec2){
    vector<T> res;

    validateVectorOperations(vec1, vec2);
    
    int n=vec1.size();
    for(auto i=0; i<n;i++){
        res.push_back(vec1[i]-vec2[i]);
    }
    return res;
}
//------------------------------------------------------------------
//ADDITION:Adding a vector by a value/another vector
//------------------------------------------------------------------
template <typename T>
vector<T> vectorAddition(const vector<T> &vec, const double val){
    vector<T> res;
    auto n=vec.size();
    for(auto i=0; i<n;i++){
        res.push_back(vec[i]+val);
    }
    return res;
}
template <typename T>
vector<vector<T>> vectorAddition( const vector<vector<T>> &vec, const double val){
    vector<vector<T>> res;
    auto n=vec.size();
    for(auto i=0; i<n;i++){
        auto m=vec[i].size();
        vector<double> temp;
        for(auto j=0; j<m;j++){
            temp.push_back((vec[i][j]+val));
        }
        res.push_back(temp);
    }
    return res;
}
template <typename T>
vector<T> vectorAddition( const vector<T> &vec1, const vector<T> &vec2){
    vector<T> res;
    validateVectorOperations(vec1, vec2);
    auto n=vec1.size();
    for(auto i=0; i<n;i++){
        res.push_back((vec1[i]+vec2[i]));
    }
    return res;
}
template <typename T>
vector<T> vectorAddition( const vector<T> &vec1, const vector<T> &vec2, const vector<T> &vec3){//computes the the Percent difference of vec1 and vec2 elemenst relative to vec2
    vector<T> res;
    if ((vec1.size()!=vec2.size())||(vec1.size()!=vec3.size())){
        cout<<"Error: vectors don't have the same size!"<<endl;
    }
    auto n=vec1.size();
    for(auto i=0; i<n;i++){
        res.push_back((vec1[i]+vec2[i]+vec3[i]));
    }
    return res;
}
template <typename T>
vector<vector<T>> vectorAddition( vector<vector<T>> &vec1, const vector<vector<T>> &vec2){//computes the the Percent difference of vec1 and vec2 elemenst relative to vec2
    if (vec1.size()==0) return vec2;
    validateVectorOperations(vec1, vec2);
    auto N=vec1.size();
    for(auto i=0; i<N;i++){
        validateVectorOperations(vec1[i], vec2[i]);
        auto J=vec1[i].size();
        for (auto j=0;j<J;j++){
            vec1[i][j]=vec1[i][j]+vec2[i][j];
        }
    }
    return vec1;
}

//------------------------------------------------------------------
//AVERAGE:
//------------------------------------------------------------------
template <typename T>
double vectorAve( const vector<T> &vec){
    auto s=vec.size();
    if (s==0) return -1000;
    double sum=0;
    for (const auto &inner: vec){
        sum += inner;
    }
    return (double) sum/s;
}
template <typename T>
vector<vector<double>> vectorAve(const vector<vector<vector<T>>> &vec){//return type is double
    vector<vector<double>> vSum;
    vSum=vec[0];
    auto reps=vec.size();
    for (auto i=1;i<reps;i++)
        vSum=vectorAddition(vSum, vec[i]);
    vector<vector<double>> res=vectorDivision(vSum,reps);
    return res;
}
template <typename T>
vector<double> vectorAveRows(const vector<vector<T>> &vec){//return type is double
    auto rows=vec.size();
    if (rows==0) return {-1};
    auto cols=vec[0].size();
    //sum all rows
    vector<double> vSum(cols,0);
    for (const auto &outer: vec){
        // transform(vSum.begin(), vSum.end(), outer.begin(),  plus<double>());
        for(auto i=0;i<outer.size();i++){
            vSum[i] += outer[i] ;
        }}
    //  transform(vSum.begin(), vSum.end(), vSum.begin(), bind2nd(divides<double>(), rows) );
    vector<double>  res;
    for (const auto &num:vSum){
        res.push_back(num/rows);
    }
    return res;
}
template <typename T>
vector<double> vectorAveCols(const vector<vector<T>> &vec){
    auto rows=vec.size();
    if (rows==0) return {-1};
    auto cols=vec[1].size();
    //sum all cols
    vector<double> vSum;
    for (const auto &outer: vec){
        double sum=0;
        for (const auto &inner:outer){
            sum += inner;
        }
        vSum.push_back(sum);
    }
    //averageing:
    //transform(vSum.begin(), vSum.end(), vSum.begin(), bind2nd(divides<double>(), cols) );
    vector<double>  res;
    for (const auto &num:vSum){
        res.push_back(num/cols);
    }
    return res;
}
template <typename T>
vector<vector<double>> vectorAveRows(const vector<vector<vector<T>>> &vec){//return type is double
    vector<vector<double>> vRes;
    for (const auto &out:vec){
        vRes.push_back(vectorAveRows(out));
    }
    return vRes;
}
template <typename T>
vector<vector<double>> vectorAveCols(const vector<vector<vector<T>>> &vec){//return type is double
    vector<vector<double>> vRes;
    for (const auto &out:vec){
        vRes.push_back(vectorAveCols(out));
    }
    return vRes;
}
//------------------------------------------------------------------
//GENERAL
//------------------------------------------------------------------
template <typename T>
vector<double> vectorPercentImp( const vector<T> &vec, const double m){
    vector<double>  res;
    for (const auto &num:vec){
        res.push_back(100*(m-num)/m);
    }
    return res;
}
template <typename T>
vector<double> vectorRelDiff( const vector<T> &vec1num, const vector<T> &vec2Dom){//computes the the Percent difference of vec1 and vec2 elemenst relative to vec2
    vector<double> res;
    validateVectorOperations(vec1num, vec2Dom);
    auto n=vec1num.size();
    for(auto i=0; i<n;i++){
        if(vec2Dom[i]==0)
            res.push_back(0);
        else{
            res.push_back((vec1num[i]-vec2Dom[i])/vec2Dom[i]);
        }
    }
    return res;
}

template<typename T>
double vectorStd(const vector<T> &vec){  
    if(vec.size()==0) return 0;
    double sigma_squared = 0.0;
    double ave = vectorAve(vec);
    double n=vec.size();
    for (const auto &val:vec)
        sigma_squared += (val - ave)*(val - ave);
    
    return sqrt(sigma_squared/(n-1));
}
template <typename T>
double vectorMedian(vector<T> vec)    { //We want a copy, otherwise original vector will be sorted
    if(vec.size()==0) return 0;
    double median;
    auto n=vec.size();
    sort(vec.begin(), vec.end());
    if (n % 2 == 0)
        median = (vec[n / 2 - 1] + vec[n / 2]) / 2;
    else
        median = vec[n / 2];
    //
    return median;
}
//------------------------------------------------------------------
//READ VECTOR
//------------------------------------------------------------------

/* Function used to load the data from file into the model.
 * The data must be saved so it can be shared between runs
 */
template < typename T >
vector<vector<T>> readVector( const string filename, const bool skip_first = false ) {
    std::ifstream in(filename);
    std::vector<std::vector<T> > v;
    
    if (in) {
        std::string line;
        
        while (std::getline(in, line)) {
            v.push_back(std::vector<T>());
            
            // Break down the row into column values
            std::stringstream split(line);
            T value;
            
            auto first = true;
            while (split >> value) {
                if (first && skip_first) {
                    first = false;
                    continue;
                }
                v.back().push_back(value);
            }
        }
    }

    return v;
}

//------------------------------------------------------------------
//WRITE VECTOR
//------------------------------------------------------------------
template < typename T >
void writeVector(std::shared_ptr<ThreadID> tid, const vector< T > &vec, const string &filename, const bool overwrite=true){
    stringstream fname;fname<<filename<<"_"<<tid->as_string();
    unique_ptr<ofstream> ofile = _setupOutputVectorFile ( fname.str(), overwrite );
    ostream_iterator<T> output_iterator(*ofile, " ");
    copy(vec.begin(), vec.end(), output_iterator);
    *ofile<<"\n";
    ofile->close();
}

template < typename T >
void writeVector(std::shared_ptr<ThreadID> tid,const vector< vector <T> > &vec, const string &filename, const bool overwrite=true, const bool prepend_tID = true ){
    stringstream fname;
    fname << filename << "_" << tid->as_string();
    unique_ptr<ofstream> ofile = _setupOutputVectorFile ( fname.str(), overwrite );
    //Looking to eliminate the scientific notation used for the RNG seed; we need the 
    //fixed value, no approximation will do.
    const auto tid_outputs = tid->as_output();

    //The below line was used because the seed was being reduced to exponential notation
    //this effectively put 9 decimal points after every value, so that we had enough
    //precision for the seed.  Now we have a separate seed file, and since the addition of
    //all those zeros makes the output files enormous, I am going to remove this.
    
    //*ofile << std::fixed << std::setprecision(9);
    
    for (auto outer = vec.begin(); outer != vec.end(); outer ++) {
        if (outer != vec.begin())  //Eliminates the newline at the end of file
            *ofile << endl;
        //   *ofile << endl;
        ostream_iterator<T> output_iterator(*ofile , " ");
        //Output the tids to the file
        if (prepend_tID) {
            for (auto i = 0u; i < tid_outputs.size(); i++) {
                *ofile << tid_outputs[i].second << " ";
            }
        }
        copy(outer->begin(), outer->end(), output_iterator);
    }
    ofile->close();
}

template < typename T >
void writeCSVFile(
                  const int tid,
                  const vector <T> &vec,
                  const string &filename,
                  const bool overwrite=true
                  ){
    stringstream fname;fname<<filename<<"_"<<tid << ".csv";
    unique_ptr<ofstream> ofile = _setupOutputVectorFile ( fname.str(), overwrite );

    ostream_iterator<double> writer(*ofile, ",");
    copy(vec.cbegin(), vec.cend(), writer);
    *ofile << endl;
}

template < typename T >
void writeCSVFile(
                  const int tid,
                  const vector< string > &headers,
                  const vector< vector <T> > &vec,
                  const string &filename,
                  const bool overwrite=true
                  ){
    stringstream fname;fname<<filename<<"_"<<tid << ".csv";
    unique_ptr<ofstream> ofile = _setupOutputVectorFile ( fname.str(), overwrite );

    ostream_iterator<string> header_writer(*ofile, ",");
    copy(headers.begin(), headers.end(), header_writer);
    *ofile << endl;
    for (auto outer = vec.begin(); outer != vec.end(); outer ++) {
        if (outer != vec.begin())  //Eliminates the newline at the end of file
            *ofile << endl;
        //   *ofile << endl;
        ostream_iterator<T> output_iterator(*ofile , ",");
        copy(outer->begin(), outer->end(), output_iterator);
    }
}

template < typename T >
void writeVector(const long long int tid,const vector< vector< vector <T>>> &vec, const string &filename, const bool overwrite=true){
    stringstream fname;fname<<filename<<"_"<<tid;
    unique_ptr<ofstream> ofile = _setupOutputVectorFile ( fname.str(), overwrite );
    auto N1=vec.size();
    for (auto i=0;i<N1;i++) {
        vector<vector<T>> outer1=vec[i];
        if (i>0)  //Eliminates the newline at the end of file
            *ofile << endl;
        
        auto N2=outer1.size();
        for (auto j=0;j<N2;j++) {
            vector<T> outer2=outer1[j];
            if (j>0)  //Eliminates the newline at the end of file
                *ofile << endl;
            
            ostream_iterator<T> output_iterator(*ofile , " ");
            copy(outer2.begin(), outer2.end(), output_iterator);
        }
    }
}
template <typename T>
void writeVectorsSideBySide( const vector< vector <T> > &vec1, const vector< vector <T> > &vec2, const string &filename, const bool overwrite){
    if (vec1.size()==vec2.size()) {
        unique_ptr<ofstream> ofile = _setupOutputVectorFile ( filename, overwrite );
        auto N=vec1.size();
        for (auto i=0;i<N;i++){
            if (i!=0)  *ofile << endl;////Eliminates the newline at the end of file
            ostream_iterator<T> output_iterator(*ofile , " ");
            vector<T> * outer=&vec1[i];
            copy(outer->begin(), outer->end(), output_iterator);
            outer=&vec2[i];
            copy(outer->begin(), outer->end(), output_iterator);
        }
    }
    else
        cout<<"Error: vectors are not of the same size!"<<endl;
}
//------------------------------------------------------------------
//PRINT
//------------------------------------------------------------------
template < typename T >
void printVector(const vector<T> &vec){
    for (const auto &outer:vec){
        cout<<outer<<" ";
    }
    cout<<endl;
}
template < typename T >
void printVector(const vector<vector<T>> &vec){
    for(const auto &outer:vec){
        for (const auto &inner:outer){
            cout<<inner<<" ";
        }
        cout<<endl;
    }
    cout<<endl;
}
template < typename T >
void printVector(const vector<T> &vec, const string &s){
    cout<<s<<endl;
    for (const auto &outer:vec){
        cout<<outer<<" ";
    }
    cout<<endl;
}
template < typename T >
void printVector(const vector<vector<T>> &vec, const string &s){
    cout<<s<<endl;
    for(const auto &outer:vec){
        for (const auto &inner:outer){
            cout<<inner<<" ";
        }
        cout<<endl;
    }
    cout<<endl;
}


}
#endif //__MDRTB_FUNCTIONS_H
