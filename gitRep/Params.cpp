//----
//---------------------------------------------------
//
//  ModParam.cpp
//
//  Function definitions for reading mutable config
//  data from text file.
//
//  JPennington, June 2015,2019
//
//
//---------------------------------------------------
#include "Params.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

//Constructor; loads up a config file and builds the Params object with it.
Params::Params( const std::string &filename ) {

    if ( !filename.empty() ) {
        //Load from file "filename"
        std::cout << "--Loading Config Data from " << filename << "--" << std::endl;
        bool success = initFromFile( filename );
        if ( !success ) {
            std::cerr << " Couldn't load data from file." << std::endl;
            throw std::runtime_error("No config file could be loaded, aborting");
        }
    }
}


//Compare two instances to see if they contain the same values
bool Params::compare(const Params &mp) const {
    return double_map == mp.returnMap();
}

//Do the loading from the file (called from the constructor)
bool Params::initFromFile( const std::string &filename ) {
    //Load from file "filename"
    std::vector<std::string> lines;
    std::string line;
    std::ifstream cfg_file( filename );
    if ( cfg_file.is_open() ) {

        //Loop through the lines of the file
        while ( getline( cfg_file, line ) ) {
            //If line is not a comment or blank
            if ( line[0] != '#' && line.find_first_not_of( ' ' ) != std::string::npos )
                lines.push_back( line );
        }

        cfg_file.close();

        //Process data into params_list object
        //for (unsigned int i = 0; i < lines.size(); i++ ) {
        for ( const auto &line : lines ) {
            //Find the = character, split line into two parts
            auto equals_pos     = line.find( "=" );
            std::string p_name  = line.substr( 0, equals_pos );
            std::string p_value = line.substr( equals_pos + 1, line.size() );

            p_value = trim( p_value );
            p_name  = trim( p_name );

            //Values will only be doubles
            double_map.emplace( p_name, std::stod( p_value ) );
        }

    } else {
        return false;
    }
    return true;
}

//View the data loaded in the Params object; all values are shown
void Params::viewLoaded() const {
    if ( double_map.size() == 0 ) {
        std::cout << "No config data loaded" << std::endl;
    } else {
        std::cout << std::endl
                  << "-----List of Parameters stored-----" << std::endl;
        for ( const auto &value : double_map ) {
            std::cout << value.first << " = " << value.second << std::endl;
        }
    }
}

//Static function used to trim the line from the config file, stripping all
//unneeded whitespace from the front and the back
std::string Params::trim( const std::string &str,
                            const std::string &whitespace ) const {
    const auto strBegin = str.find_first_not_of( whitespace );
    if ( strBegin == std::string::npos )
        return ""; // no content

    const auto strEnd   = str.find_last_not_of( whitespace );
    const auto strRange = strEnd - strBegin + 1;

    return str.substr( strBegin, strRange );
}


//Getters/Setters

double Params::getD( const std::string &var_name ) {
    //The call to getD should fail if the unordered_map doesn't
    //contain the parameter we are trying to get.
    auto does_contain = double_map.find(var_name);
    if (does_contain != double_map.end()) {
        return does_contain->second;
    } else {
        std::stringstream ss;
        ss << "Tried to get a non-existant parameter : " << var_name;
        throw std::logic_error(ss.str());
    }
}

void Params::setD( const std::string &var_name, const double value ) {
    //The call to setD should fail if the unordered_map doesn't
    //contain the parameter we are trying to set.
    auto does_contain = double_map.find(var_name);
    if (does_contain != double_map.end()) {
        does_contain->second = value;
    } else {
        std::stringstream ss;
        ss << "Tried to set a non-existant parameter : " << var_name;
        throw std::logic_error(ss.str());
    }
}

//Return a vector of doubles containing all the values in the map, no keys
std::vector<double> Params::returnValueList() const {
    std::vector<double> return_list;
    for ( const auto& param : double_map ) {
        return_list.push_back( param.second );
    }
    return return_list;
}

//Write the values in the Params object into a new file, with a new filename.
//const std::string &text in this case is a comment that you can add to the file
//to distinguish it from others by content.
void Params::writeConfig( const std::string &filename, const std::string &text = "" ) const {
    std::ofstream file( filename );
    file << "#HIV Model config file" << std::endl;

    if ( text != "" )
        file << "#" << text << std::endl;

    for ( const auto& param : double_map ) {
        file << std::endl
             << param.first << "=" << param.second << std::endl;
    }
}
