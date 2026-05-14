#pragma once
#include <boost/program_options.hpp>
#include <string>
#include <ostream>

namespace po = boost::program_options;

struct Config {
    bool mode_gen_rand = false;
    bool mode_train = false;
    bool mode_test = false;
    bool mode_gen_brand = false;

    bool generate = false;
    bool train = false;
    bool test = false;
    std::string output_dir;
    std::string files_dir;
    std::string training_output_dir;


    // Mode flags (implicit based on which main switch is active)
    // std::string gen_dir;
    // std::string train_dir;
    // std::string test_dir;

    // Common / Misc
    // std::string run_name;
    int seed;

    // Training / Network
    int val_set_size;
    int batch_size;
    int layer1;
    int layer2;

    // Optimization / Testing
    int population;
    int max_evals;
    double sigma;
    bool schedules;
    bool graphics;
    int time_limit;

    // Generation (Synthetic)
    int machines;
    int operation_types;
    int jobs_min;
    int jobs_max;
    int job_len_min;
    int job_len_max;
    int num_alt_min;
    int num_alt_max;
    int t_min;
    int t_max;
    int set_size;
    int common_seed;

    // Generation (Brandimarte)
    int brandimarte;
};


std::ostream &operator<<(std::ostream &os, const Config &c);

Config parse_command_line(int argc, char *argv[]);
