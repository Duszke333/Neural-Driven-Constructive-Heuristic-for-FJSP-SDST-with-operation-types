/**
* @file main.cpp
 * @brief Main entry point for the Flexible Job Shop Scheduling heuristic optimizer.
 * * ==============================================================================
 * SUPPORTED CLI OPTIONS:
 * ==============================================================================
 * * [1. Generating Synthetic Data]
 * --generate --output_dir=gen_dir --machines=num --operation_types=num --jobs_min=num --jobs_max=num --job_len_min=num --job_len_max=num  --num_alt_min=num --num_alt_max=num --t_min=num --t_max=num --set_size=num [-common_seed=1] [--seed=1]
 * * [2. Generating Brandimarte Benchmarks]
 * --generate --output_dir=gen_dir --brandimarte=id --set_size=size [--seed=1]
 * * [3. Training (CMA-ES)]
 * --train --output_dir=output_dir --files_dir=train_files_dir --val_set_size=size [-layer1=32] [layer2=16] [--seed=1] [--population=192] [--batch=50] [--max_evals=500000] [--sigma=0.1]
 * * [4. Testing / Inference]
 * --test --output_dir=output_dir --files_dir=test_files_dir --training_output_dir=training_output_dir [--schedules] [--graphics] [--seed=1] [--population=192] [--max_evals=500000] [--sigma=0.1] [--time_limit=60]
 * ==============================================================================
 */

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "Testing.h"
#include "JobshopConstructionHeuristic.h"
#include "Generator.h"
#include "JobshopDrawer.h"
#include "DataExport.h"
#include <boost/program_options.hpp>
#include "options.h"
#include "io.h"

using namespace std;

#include  <filesystem>

/**
 * @brief Helper class to manage memory and lifetime of simulated command line arguments.
 * * Useful for parsing hardcoded command strings without memory leaks during internal testing.
 */
class CmdLineArgs {
public:
    /**
     * @brief Constructs simulated argc and argv from a single command line string.
     * @param commandLine The raw command string to parse.
     */
    explicit CmdLineArgs(const std::string &commandLine) {
        std::stringstream ss(commandLine);
        std::string segment;

        // 1. Split string by spaces into a vector of strings
        std::vector<std::string> args;
        while (ss >> segment) {
            args.push_back(segment);
        }

        // 2. Update argc
        _argc = static_cast<int>(args.size());

        // 3. Allocate memory for argv (array of pointers)
        // +1 for the terminating nullptr (standard convention)
        _argv = new char *[_argc + 1];

        // 4. Copy strings into mutable char buffers
        for (int i = 0; i < _argc; ++i) {
            _argv[i] = new char[args[i].size() + 1];
            std::strcpy(_argv[i], args[i].c_str());
        }
        _argv[_argc] = nullptr; // Null terminate the array
    }

    /**
     * @brief Destructor to clean up dynamically allocated string buffers safely.
     */
    ~CmdLineArgs() {
        if (_argv) {
            for (int i = 0; i < _argc; ++i) {
                delete[] _argv[i]; // Delete each string
            }
            delete[] _argv; // Delete the array of pointers
        }
    }

    // Accessors
    int argc() const { return _argc; }
    char **argv() const { return _argv; }

private:
    int _argc;
    char **_argv;
};

/**
 * @brief Main program entry point.
 * * Parses arguments and routes execution to the appropriate pipeline mode
 * (training, testing, or data generation).
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Execution status (0 on success).
 */
int main(int argc, char **argv) {
    // Parse command line arguments into the global Config structure
    Config Cfg = parse_command_line(argc, argv);

    // Using the overloaded << operator to print parsed configuration
    std::cout << Cfg << std::endl;

    // Route to the selected application mode
    if (Cfg.mode_train) {
        jobshop::train(Cfg);
    } else if (Cfg.mode_test) {
        jobshop::test(Cfg);
    } else if (Cfg.mode_gen_rand) {
        jobshop::generateRandom(Cfg);
    } else if (Cfg.mode_gen_brand) {
        jobshop::generateBrandimarte(Cfg);
    } else {
        ERROR("Mode unknown.")
    }
} // main
