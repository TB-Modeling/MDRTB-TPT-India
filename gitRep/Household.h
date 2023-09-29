//
//  Household.h
//  src
//
//  Created by Trinity on 8/4/15.
//  Copyright (c) 2015 Trinity. All rights reserved.
//
#ifndef __mdrtb__Household__
#define __mdrtb__Household__
#include <cstdio>
#include <memory>
#include <random>
#include <vector>
#include <array>
#include <memory>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>

#include "Params.h"
#include "Person.h"

using namespace std;

class Person;
class Population;

class Household  {
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _id;
        ar & _tFormation;
        ar & _remainingTime;
        ar & _vMembers;
        ar & _hhct_isScreened;
    }
    int _id;      //holds the hh id number
    int _tFormation; //time of HH creation
    int _remainingTime; //time of HH dissolution
    bool _hhct_isScreened; //if household members were traced and screened
    vector<std::shared_ptr<Person>> _vMembers; //vector of hh members
    
public:
    
    //public:
    // CLASS FUNCTIONS-------------------------------------------------------------------------------
    Household();  //costructor
    Household(mt19937 &rng, std::shared_ptr<Params> mp, int id, int formationTime);//constructor for initial household
    ~Household();   //deconstructor
     //Serialization:
    bool compare(const shared_ptr<Household>& h) const; //comapres this person with h to make sure all attributes are the same
    bool operator == (std::shared_ptr<Household> &s) const {
        bool equivalent = compare(s);
        return equivalent;
    }
    bool operator != (shared_ptr<Household>& h) const {
        return !compare(h);
    }
    // HELPER FUNCTIONS--------------------------------------------------------
    shared_ptr<Person> operator [](int i) const {return _vMembers[i];}
    vector<shared_ptr<Person>> ::iterator begin()   {return _vMembers.begin();}
    vector<shared_ptr<Person>> ::iterator end()   {return _vMembers.end();}
    vector<shared_ptr<Person>> ::const_iterator cbegin() const  {return _vMembers.cbegin();}
    vector<shared_ptr<Person>> ::const_iterator cend()  const  {return _vMembers.cend();}
    //
    void addMember(const shared_ptr<Person> p); //add p as a new member to the end of member list
    void removeDeadMembers();
    void drain(); //remove all members from the household
    bool removeMember(const int i);//kills member p (unlink, copy by back, destroy, popout)
    int getMemberId(const shared_ptr<Person> p) const; //returns the id of this person in the household vector
    //
    vector<shared_ptr<Person>> getMembers() const;
    inline shared_ptr<Person> getMember( const int i ) const {return _vMembers[i];}
    //
    inline int getId() const {return _id;}     inline void setId(const int s){_id=s;}
    inline unsigned getSize() const {return _vMembers.size();}
    inline int getFormationTime() const {return _tFormation;}    inline int getRemainingTime() const {return _remainingTime;}
    inline void setRemainingTime(const int t){_remainingTime=t;}   inline void incRemainingTime(const int inc){_remainingTime=_remainingTime+inc;}
    inline void set_hhct_isScreened(const bool b){_hhct_isScreened=b;}   inline bool get_hhct_isScreened(){return _hhct_isScreened;}
    
    //REPORTING FUNCTIONS-------------------------------------------------------
    const array<int,6> returnDistAgeCategory() const;
    vector<int> returnDistAge() const;
    vector<int> returnDistAgeGender() const;
    int returnMedianAge() const;
    
     //DEBUG--------------------------------------------------------------------
    void debug();
};
#endif /* defined(__mdrtb__Household__) */
