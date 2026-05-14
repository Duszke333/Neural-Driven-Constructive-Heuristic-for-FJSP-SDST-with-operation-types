/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Params.h
 * Author: tsliwins
 *
 * Created on 10 marca 2021, 15:42
 */
#pragma once

#include <vector>
#include <string>

#include "ReadConfig.h"


using namespace std;

using namespace ReadConfig;


/**
 * Ważne, żeby enum'y były w przestrzeni nazw ReadConfig
 *
 * Każdy enum musi być zdefiniowany tak jak w przykładach poniżej
 *
 * Dodatkowo dla kazdego enuma musi być zdefiniowana funkcja enum_name, której
 * argument jest typu takiego jak definiowany enum i zwraca wektor nazw kolejnych enumów
 *
 * Oczywiście przy tym podejściu enumy muszą mieć kolejne wertości zaczynając od 0
 */
namespace ReadConfig {
    enum class PsiStrategy {
        noNPI = 0, evenly, greedy, deathRatio, file, greedySpatial
    };

    inline const vector<string> &enum_names(PsiStrategy) {
        static vector<string> names{"noNPI", "evenly", "greedy", "deathRatio", "file", "greedySpatial"};
        return names;
    }

    enum class UStrategy {
        no = 0, evenly, ageGreedy, file
    };

    inline vector<string> &enum_names(UStrategy) {
        static vector<string> names{"no", "evenly", "ageGreedy", "file"};
        return names;
    }

    enum class CASE {
        Poland = 0, Ohio, Test
    };

    inline const vector<string> &enum_names(CASE) {
        static vector<string> names{"Poland", "Ohio", "Test"};
        return names;
    }

    enum class SpatialLevel {
        community = 0, county, voivodeship, country
    };

    inline const vector<string> &enum_names(SpatialLevel) {
        static vector<string> names{"community", "county", "voivodeship", "country"};
        return names;
    }

    enum class InitialCasesState {
        evenly = 0, proportionalSpatial, proportional, proportionalSpatialAge
    };

    inline const vector<string> &enum_names(InitialCasesState) {
        static vector<string> names{"evenly", "proportionalSpatial", "proportional", "proportionalSpatialAge"};
        return names;
    }

    enum class DecAggregation {
        decWeeks = 0, decWeeksGroups, decWeeksCounties, decWeeksCountiesGroups
    };

    inline const vector<string> &enum_names(DecAggregation) {
        static vector<string> names{"decWeeks", "decWeeksGroups", "decWeeksCounties", "decWeeksCountiesGroups"};
        return names;
    }
}

//struct Params {
//    // [SPREAD_MODEL]
//    int weeks;
//    int PsiD;
//    int An; // 0 oznacza automatyczne ustawienie An na podstawie odczytanych danych
//    int Sn;
//    double threshold;
//
//    double beta;
//    double mi;
//    double sigma;
//    double ro;
//    double gammaA;
//    double gammaI;
//
//    double nju;
//
//    //[OBJECTIVE]
//    bool psi_opt;
//    bool minSum;
//    bool minDeath; //yes=minimalizacja zgonow, false=minimalizacja zakazen
//
//    bool u_opt;
//    double u_weekly; // = 200000
//
//    //[CONSTRAINTS]
//    double minAvgPsi;
//    double minPsi;
//
//    //[SIMULATION]
//    PsiStrategy psiStrategy;
//
//    string psiControlInputFileName; // = ../optymalizacja/county_minSumI3000000_WCG_full_controls.csv
//
//
//    UStrategy uStrategy;
//    string uControlInputFileName; // = ../optymalizacja/county_mi
//
//    double minRequiredPsi;
//    double percentageMostInfectedRegions; // = 1.0 ; not implemented yet
//
//
//    //[TEST_CASE]
//
//    CASE Case;
//
//    SpatialLevel spatialLevel;
//
//    string countyDictionaryFile; // = ../PolandData/powiatyDic.csv
//    string communityDictionaryFile; // = ../PolandData/gminyDic.csv
//    string voivodeshipDictionaryFile; // = ../PolandData/wojewodztwaDic.csv
//    string mobilityFile; // = ../PolandData/mobilnoscPowiatyFixedUTF.csv
//    string mobilityWorkingFile; // = ../PolandData/przeplywy_pracujacy.csv
//    string mobilitySchoolFile; // = ../PolandData/przeplywy_uczniowie.csv
//    string initialCases; // = ../PolandData/zarazenia.csv  ;puste pole oznacza rozklad rownomierny, wypelnione oznacza proporcjonalnie do przypadkow z pliku
//
//    InitialCasesState initialCasesState;
//
//    vector<double> initialCasesAgeProportions; // = 0.333,0.333,0.333 ; jak rozkladaja sie przypadki startowe w grupach wiekowych, powinno sie sumowac do 1
//    bool setIsolatedRegions; // = no
//
//    double youngMobilityFactor; // = 1.0 ; 0.85
//    double oldMobilityFactor; // = 0.8 ; 0.8
//    double middMobilityFactor; // = 1.1 ; 0.8
//    double youngMobilityIntercityFactor; // = 1.0
//    double oldMobilityIntercityFactor; // = 1.0
//    double youngMobilityFromMiddFactor; // = 0.1
//
//    double E0Factor; // = 0.0065  ;0.0030
//    double I0Factor; // = 0.0036 ; 0.0012
//    double A0Factor; // = 0.0182 ; 0.004
//    double R0Factor; // = 0.1; 0.069
//    double D0Factor; // = 0.0011  ; 0.0001
//    double asymptomaticMobilityFactor; // = 1.0
//    vector<double> symptomaticMobilityFactor; // = 1.0,1.0,1.0 ; 0.8,0.8,0.8
//    vector<double> dI; // = 0.000000723,0.000154,0.00727
//
//    //[TASKS]
//    bool simulation; // = yes
//    bool optimization; // = no
//    bool multithreaded; // = yes
//
//    //[OPTIMIZATION]
//    int max_fevals; // = 3000000
//    bool stopFTolerance; // = no
//    double ftolerance; //= 1E-7  ; przy domyslnych ustawieniach to jest sprawdzane co okolo 9272 krokow
//    double sigma_cmaes; // = 0.2  ; wstepna dlugosc kroku optymalizatora, bardzo ważna: rozwiązanie optymalne powinno być w zakresie x0 +/- sigma
//    int lambda; // = 96
//    int seed; // = 13
//    int maxTime; // = 50000
//    bool progressCallback; // = yes
//    int constraintPenaltyWeight; // = 1000
//
//    DecAggregation decAggregation;
//
//
//    //[OUTPUT]
//    string diseaseSpreadFilename;
//    string controlFilename;
//    string statsFilename;
//    bool autoGenPrefixToFilenames; //automatycznie dodaje prefixy do nazwy plikow odzwierciedlajace najwazniejsze parametry


//        // [SPREAD_MODEL]
//        string section_name;
//        section_name = "SPREAD_MODEL";
//        CM.READ(weeks);
//        CM.READ(PsiD);
//        CM.READ(An); // 0 oznacza automatyczne ustawienie An na podstawie odczytanych danych
//        CM.READ(Sn);
//        CM.READ(threshold);
//
//        CM.READ(beta);
//        CM.READ(mi);
//        CM.READ(sigma);
//        CM.READ(ro);
//        CM.READ(gammaA);
//        CM.READ(gammaI);
//
//        CM.READ(nju);
//
//        //[OBJECTIVE]
//        section_name = "OBJECTIVE";
//        CM.READ(psi_opt);
//        CM.READ(minSum);
//        CM.READ(minDeath); //yes=minimalizacja zgonow, false=minimalizacja zakazen
//
//        CM.READ(u_opt);
//
//
//        //[CONSTRAINTS]
//        section_name = "CONSTRAINTS";
//        CM.READ(minAvgPsi);
//        CM.READ(minPsi);
//        CM.READ(u_weekly); // = 200000
//
//        //[SIMULATION]
//        section_name = "SIMULATION";
//        CM.READ(psiStrategy);
//
//        CM.READ(psiControlInputFileName); // = ../optymalizacja/county_minSumI3000000_WCG_full_controls.csv
//
//
//        CM.READ(uStrategy);
//        CM.READ(uControlInputFileName); // = ../optymalizacja/county_mi
//
//        CM.READ(minRequiredPsi);
//        CM.READ(percentageMostInfectedRegions); // = 1.0 ; not implemented yet
//
//
//        //[TEST_CASE]
//        section_name = "TEST_CASE";
//
//        CM.read(section_name, "case", Case); // nazwa zmiennej inna niż nazwa parametru, więc trzeba tak odczytać
//        CM.READ(spatialLevel);
//
//        CM.READ(countyDictionaryFile); // = ../PolandData/powiatyDic.csv
//        CM.READ(communityDictionaryFile); // = ../PolandData/gminyDic.csv
//        CM.READ(voivodeshipDictionaryFile); // = ../PolandData/wojewodztwaDic.csv
//        CM.READ(mobilityFile); // = ../PolandData/mobilnoscPowiatyFixedUTF.csv
//        CM.READ(mobilityWorkingFile); // = ../PolandData/przeplywy_pracujacy.csv
//        CM.READ(mobilitySchoolFile); // = ../PolandData/przeplywy_uczniowie.csv
//        CM.READ(initialCases); // = ../PolandData/zarazenia.csv  ;puste pole oznacza rozklad rownomierny, wypelnione oznacza proporcjonalnie do przypadkow z pliku
//
//        CM.READ(initialCasesState);
//
//        CM.READ(initialCasesAgeProportions); // = 0.333,0.333,0.333 ; jak rozkladaja sie przypadki startowe w grupach wiekowych, powinno sie sumowac do 1
//        CM.READ(setIsolatedRegions); // = no
//
//        CM.READ(youngMobilityFactor); // = 1.0 ; 0.85
//        CM.READ(oldMobilityFactor); // = 0.8 ; 0.8
//        CM.READ(middMobilityFactor); // = 1.1 ; 0.8
//        CM.READ(youngMobilityIntercityFactor); // = 1.0
//        CM.READ(oldMobilityIntercityFactor); // = 1.0
//        CM.READ(youngMobilityFromMiddFactor); // = 0.1
//
//        CM.READ(E0Factor); // = 0.0065  ;0.0030
//        CM.READ(I0Factor); // = 0.0036 ; 0.0012
//        CM.READ(A0Factor); // = 0.0182 ; 0.004
//        CM.READ(R0Factor); // = 0.1; 0.069
//        CM.READ(D0Factor); // = 0.0011  ; 0.0001
//        CM.READ(asymptomaticMobilityFactor); // = 1.0
//        CM.READ(symptomaticMobilityFactor); // = 1.0,1.0,1.0 ; 0.8,0.8,0.8
//        CM.READ(dI); // = 0.000000723,0.000154,0.00727
//
//        //[TASKS]
//        section_name = "TASKS";
//        CM.READ(simulation); // = yes
//        CM.READ(optimization); // = no
//        CM.READ(multithreaded); // = yes
//
//        //[OPTIMIZATION]
//        section_name = "OPTIMIZATION";
//        CM.READ(max_fevals); // = 3000000
//        CM.READ(stopFTolerance); // = no
//        CM.READ(ftolerance); //= 1E-7  ; przy domyslnych ustawieniach to jest sprawdzane co okolo 9272 krokow
//        CM.READ(sigma_cmaes); // = 0.2  ; wstepna dlugosc kroku optymalizatora, bardzo ważna: rozwiązanie optymalne powinno być w zakresie x0 +/- sigma
//        CM.READ(lambda); // = 96
//        CM.READ(seed); // = 13
//        CM.READ(maxTime); // = 50000
//        CM.READ(progressCallback); // = yes
//        CM.READ(constraintPenaltyWeight); // = 1000
//        CM.READ(decAggregation);
//
//
//        //[OUTPUT]
//        section_name = "OUTPUT";
//        CM.READ(controlFilename);
//        CM.READ(statsFilename);
//        CM.READ(autoGenPrefixToFilenames); //automatycznie dodaje prefixy do nazwy plikow odzwierciedlajace najwazniejsze parametry


//};




