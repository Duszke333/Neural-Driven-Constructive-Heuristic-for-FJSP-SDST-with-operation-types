/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Err.h
 * Author: tsliwins
 *
 * Created on 19 grudnia 2020, 17:07
 */

#pragma once
#include <string>
#include <iostream>

namespace chof {

    using namespace std;

    struct Err {
        string msg;

        Err() : msg() {}
        Err( const string &_msg ) : msg(_msg) {};



    };

#define INFO(msg) std::cout << std::endl << __FILE__<< " (" << __LINE__ << "): " << msg << std::endl;

#define WARN(msg) std::cerr << std::endl << __FILE__<< " (" << __LINE__ << "): warning: " << msg;


#define ERROR(msg) { std::cerr << std::endl << std::string(__FILE__) + " ("  + std::to_string(__LINE__) + "): error: " + msg  << std::endl << std::flush; throw chof::Err( std::string(__FILE__) + " ("  + std::to_string(__LINE__) + "): error: " + msg );}


#define INTERNAL(msg) { std::cerr << std::endl << std::string(__FILE__) + " ("  + std::to_string(__LINE__) + "): error: " + msg  << std::endl << std::flush;  throw chof::Err(std::string(__FILE__) + " (" + std::to_string(__LINE__) + "): internal error: " + msg); }
}