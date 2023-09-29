//
//  Household.cpp
//  src
//
//  Created by Trinity on 8/4/15.
//  Copyright (c) 2015 Trinity. All rights reserved.
//
#include "Household.h"
#include "Person.h"
#include "Params.h"
#include "GlobalVariables.h"
#include <iostream>
#include <random>
#include <array>
#include <memory>

using namespace std;


//---------------------------------------------------
//CLASS FUNCTIONS
//---------------------------------------------------
Household::Household() {
    _id=-1;
}
Household::Household(mt19937 &rng, std::shared_ptr<Params> mp, int id, int formationTime) {
    _id = id;
    _tFormation=formationTime;
    _remainingTime= globalVariables::__HOUSEHOLD_FOLLOWUP_PERIOD;
    _vMembers={};
    _hhct_isScreened=false;
 }
Household::~Household() {
    //no need to delete shared pointers    //    for (auto it : _vMembers) {     delete (it);  }
}
//Serialization:
bool Household::compare(const std::shared_ptr<Household>& h) const {
    //compare fix attributes:
    if (_id != h->_id) return false;
    if (_tFormation != h->_tFormation) return false;
    if (_remainingTime != h->_remainingTime) return false;
    if (_hhct_isScreened != h->_hhct_isScreened) return false;
     //Compare members (people):
    if (_vMembers.size() != h->_vMembers.size()) return false;
    int N=(int) _vMembers.size();
    for (int i=0;i<N;i++){
        if (!_vMembers[i]->compare(h->_vMembers[i])) return false;
    }
    return true;
}
//---------------------------------------------------
//HELPER FUNCTIONS
//---------------------------------------------------
void Household::removeDeadMembers(){//loops through all members, selects the ones marked as ded and removes them
    for (int i=(int) _vMembers.size()-1;i>=0;i--){
        if(_vMembers[i]->isMarkedDead()){
            //std::shared_ptr<Person> p=_vMembers[i]; //JP I think this is here in error
            _vMembers.erase( _vMembers.begin() + i ); //@JP should we erase the pointer or simply pop it back from the vector? 
            //@PK This reduces the use_count() for the shared pointer to 1, so this will delete
        }}
}
bool Household::removeMember(const int i){//remove a specific member
    _vMembers.erase( _vMembers.begin() + i );
    return true;
}

void Household::drain() {
    //Remove all members from the household
    _vMembers.erase(_vMembers.begin(), _vMembers.end());
}

void Household::addMember(const std::shared_ptr<Person> p){
    _vMembers.push_back(p);
}

int Household::getMemberId(const std::shared_ptr<Person> p) const {
    int N=(int) _vMembers.size();
    for (int i=0;i<N;i++){
        if (  _vMembers[i] == p){
            return i;
        }
    }
    string memberNotFoundErr = "Member not found in scenario for person."
    "\n- id: " + to_string(p->getId()) +
    "\n- time: " + to_string(p->TNOW());
    // "\n- scenario: " + to_string(p->getScenarioTesting());
    throw logic_error(memberNotFoundErr);
}
vector<std::shared_ptr<Person>> Household::getMembers() const {
    return _vMembers;
}

const array<int,6> Household::returnDistAgeCategory() const {
    //Return the age distrubution oas a function of categories:
    //[
    // Males less that 14, 
    // Females less than 14, 
    // Males less than 21, 
    // Females less than 21, 
    // Males 21 and Older, 
    // Females 21 and older
    //]
    array<int,6> ret {0,0,0,0,0,0};
    for (auto &member : _vMembers) {
        auto sex = member->getGender();
        auto age = member->getAge();
        if (age < 14) //child
            ret[0 + sex]++;
        else if (age < 21) //teen
            ret[2 + sex]++;
        else //adult
            ret[4 + sex]++;
    }
    return ret;
}

vector<int> Household::returnDistAge() const {
    //Return a vector containing the ages of all the people in the household
    vector<int> ret;
    for (const auto &member : _vMembers)
        ret.push_back(member->getAge());
    return ret;
}

//---------------------------------------------------
//DEBUG FUNCTIONS
//---------------------------------------------------
void Household::debug(){
    for (std::shared_ptr<Person> p: _vMembers){
        if (p->getstat()==nullptr)
            cout<<"stop here"<<endl;
    }
}
