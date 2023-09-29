//----
//---------------------------------------------------
//
//  ModParam.h
//
//  Header for reading mutable config data from text
//  file.
//
//  JPennington, June 2015
//
//
//---------------------------------------------------

#ifndef __MDRTB_PARAMS_H
#define __MDRTB_PARAMS_H 

#include "GlobalVariables.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <random>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>

class Params {
private:
    friend boost::serialization::access;
    template <class Archive>
    void serialize (Archive &ar, const unsigned int version ) {
        ar & double_map;
    }
    bool initFromFile( const std::string & );
    void typeListInit();
    std::string trim( const std::string &str,
                      const std::string &whitespace = " \t" ) const;
    std::unordered_map<std::string, double> double_map;

public:
    Params() = default;
    Params( const std::string & );
    Params( const Params & ) = delete;    //copy constructor (not needed)
    ~Params() = default;
    bool compare(const Params &mp) const; //comapres this person with mp to make sure all attributes are the same
    bool operator != (const Params& mp) const {
        return !compare(mp);
    }
    std::unordered_map<std::string, double> returnMap() const {return double_map;};
    
    void writeConfig( const std::string &, const std::string & ) const;


    //Name based getters
    double getD( const std::string & );
    //Name based setters
    void setD( const std::string &, const double );
    //use this function to see the state/index/value of the loaded parameters
    void viewLoaded() const;
    //returns a single list of all values
    std::vector<double> returnValueList() const;
    
};

#endif


