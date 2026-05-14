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

namespace chof {
}


// # options:
// # generating
// --generate --output_dir=gen_dir --machines=num --operation_types=num --jobs_min=num --jobs_max=num --job_len_min=num --job_len_max=num  --num_alt_min=num --num_alt_max=num --t_min=num --t_max=num --set_size=num [-common_seed=1] [--seed=1]
// --generate --output_dir=gen_dir --brandimarte=id --set_size=size [--seed=1]
// # training
// --train --output_dir=output_dir --files_dir=train_files_dir --val_set_size=size [-layer1=32] [layer2=16] [--seed=1] [--population=192] [--batch=50] [--max_evals=500000] [--sigma=0.1]
// # testing
// --test --output_dir=ouput_dir --files_dir=test_files_dir --training_output_dir=training_output_dir [--schedules] [--graphics] [--seed=1] [--population=192] [--max_evals=500000] [--sigma=0.1] [--time_limit=60]


// Helper class to manage memory and lifetime of argv
class CmdLineArgs {
public:
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

    // Destructor to clean up memory automatically
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


int main(int argc, char **argv) {
    // //    Config Cfg = parse_command_line(argc, argv);
    //
    //    //  std::string cmd = "nd-fjsp-ch --generate=test_gen_dir_01 --machines=10 --operation_types=15 --jobs_min=20 --jobs_max=30 --job_len_min=5 --job_len_max=10 --num_alt_min=1 --num_alt_max=2 --t_min=10 --t_max=100 --set_size=50000";
    //
    //    // std::string cmd = "nd-fjsp-ch --generate=gen_dir --brandimarte=5 --set_size=25";
    //
    //      // std::string cmd = "nd-fjsp-ch --train=test_gen_dir_01 --run_name=train_01 --val_set_size=5000 --max_evals=5000";
    //
    //     // std::string cmd = "nd-fjsp-ch --test=test_dir_05 --run_name=train_01 --graphics --schedules --seed=15 --time_limit=6";
    //
    //
    //      std::string cmd1 = "nd-fjsp-ch --generate --output_dir=test_gen_dir_02 --machines=10 --operation_types=15 --jobs_min=20 --jobs_max=30 --job_len_min=5 --job_len_max=10 --num_alt_min=1 --num_alt_max=2 --t_min=10 --t_max=100 --set_size=50000";
    //     std::string cmd2 = "nd-fjsp-ch --generate --output_dir=test_gen_dir_brand --brandimarte=5 --set_size=55";
    //     // # training
    //     std::string cmd3 = "nd-fjsp-ch --train --output_dir=output_dir_ttt --files_dir=test_gen_dir_02 --val_set_size=10 --max_evals=5000";
    //     // # testing
    //     std::string cmd4 = "nd-fjsp-ch --test --output_dir=ouput_dir_test --files_dir=test_gen_dir_03 --training_output_dir=output_dir_ttt --time_limit=6 --schedules --graphics";
    //


    // 1. Convert string to argc/argv
    // CmdLineArgs parser(cmd4);
    //
    // Config Cfg = parse_command_line(parser.argc(), parser.argv());

    Config Cfg = parse_command_line(argc, argv);


    // Using the overloaded << operator
    std::cout << Cfg << std::endl;

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
