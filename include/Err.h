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

    /**
     * @brief Exception wrapper for internal simulation and configuration errors.
     */
    struct Err {
        string msg;

        Err() : msg() {
        }

        Err(const string &_msg) : msg(_msg) {
        };
    };

    /**
     * @brief Logs an informational message to standard output.
     * Appends file name and line number automatically.
     * @param msg The message to display.
     */
#define INFO(msg) std::cout << std::endl << __FILE__<< " (" << __LINE__ << "): " << msg << std::endl;

    /**
     * @brief Logs a warning message to standard error stream.
     * Appends file name and line number automatically.
     * @param msg The warning message to display.
     */
#define WARN(msg) std::cerr << std::endl << __FILE__<< " (" << __LINE__ << "): warning: " << msg;

    /**
     * @brief Logs an error message and throws an exception.
     * Halts execution by throwing a chof::Err exception.
     * @param msg The error message detailing the failure.
     */
#define ERROR(msg) { std::cerr << std::endl << std::string(__FILE__) + " ("  + std::to_string(__LINE__) + "): error: " + msg  << std::endl << std::flush; throw chof::Err( std::string(__FILE__) + " ("  + std::to_string(__LINE__) + "): error: " + msg );}

    /**
     * @brief Logs an internal system error and throws an exception.
     * Used for critical algorithmic failures (e.g., out of bounds, logic flaws).
     * @param msg The internal error message.
     */
#define INTERNAL(msg) { std::cerr << std::endl << std::string(__FILE__) + " ("  + std::to_string(__LINE__) + "): error: " + msg  << std::endl << std::flush;  throw chof::Err(std::string(__FILE__) + " (" + std::to_string(__LINE__) + "): internal error: " + msg); }
}
