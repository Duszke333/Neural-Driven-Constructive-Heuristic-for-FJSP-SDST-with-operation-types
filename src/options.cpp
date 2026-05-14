//
// Created by tsliwins on 25.11.2025.
//

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include "options.h"
#include "Err.h" // Assuming this exists as per original file

// Helper to check if a value > 0
void check_positive(const std::string &name, int value) {
    if (value <= 0) throw std::runtime_error("Error: Parameter '" + name + "' must be > 0.");
}

// Helper to check min <= max
void check_range(const std::string &name_min, int min_val, const std::string &name_max, int max_val) {
    if (min_val > max_val)
        throw std::runtime_error("Error: '" + name_min + "' (" + std::to_string(min_val) +
                                 ") cannot be greater than '" + name_max + "' (" + std::to_string(max_val) + ").");
}

// Helper to check if a specific token exists in unrecognized options (for sub-mode detection)
bool has_option(const std::vector<std::string> &opts, const std::string &tag) {
    for (const auto &opt: opts) {
        if (opt == tag || opt.find(tag + "=") == 0) return true;
    }
    return false;
}

Config parse_command_line(int argc, char *argv[]) {
    Config config;

    // -------------------------------------------------------------------------
    // 1. DEFINE OPTIONS PER MODE
    // -------------------------------------------------------------------------
    // Note: We can reuse variable references (&config.member) safely because only
    // ONE parser will actually write to them at runtime.

    // --- Global Options (Used to select the mode) ---
    po::options_description global_opts("Global Options");
    global_opts.add_options()
            ("help,h", "Produce help message")
            ("generate", po::bool_switch(&config.generate), "Mode: Synthetic Data Generation")
            ("train", po::bool_switch(&config.train), "Mode: Training")
            ("test", po::bool_switch(&config.test), "Mode: Testing");

    // --- Mode 1: Synthetic Generation ---
    po::options_description gen_opts("Mode 1 Options (--generate)");
    gen_opts.add_options()
            ("output_dir", po::value(&config.output_dir), "Output directory")
            ("machines", po::value(&config.machines), "Number of machines")
            ("operation_types", po::value(&config.operation_types), "Number of operation types")
            ("jobs_min", po::value(&config.jobs_min), "Min jobs")
            ("jobs_max", po::value(&config.jobs_max), "Max jobs")
            ("job_len_min", po::value(&config.job_len_min), "Min job length")
            ("job_len_max", po::value(&config.job_len_max), "Max job length")
            ("num_alt_min", po::value(&config.num_alt_min), "Min alternative machines")
            ("num_alt_max", po::value(&config.num_alt_max), "Max alternative machines")
            ("t_min", po::value(&config.t_min), "Min processing time")
            ("t_max", po::value(&config.t_max), "Max processing time")
            ("set_size", po::value(&config.set_size), "Number of problems to generate")
            ("common_seed", po::value(&config.common_seed)->default_value(1), "Seed for common factory setup")
            ("seed", po::value(&config.seed)->default_value(1), "Random seed");

    // --- Mode 2: Brandimarte Generation ---
    po::options_description brand_opts("Mode 2 Options (--generate --brandimarte=ID)");
    brand_opts.add_options()
            ("brandimarte", po::value(&config.brandimarte), "Brandimarte ID (1-15)")
            ("output_dir", po::value(&config.output_dir), "Output directory")
            ("set_size", po::value(&config.set_size), "Number of problems to generate")
            ("seed", po::value(&config.seed)->default_value(1), "Random seed");

    // --- Mode 3: Training ---
    po::options_description train_opts("Mode 3 Options (--train)");
    train_opts.add_options()
            ("files_dir", po::value(&config.files_dir), "Directory containing files for training")
            ("output_dir", po::value(&config.output_dir), "Output directory for statistics and NN weights")
            ("val_set_size", po::value(&config.val_set_size)->default_value(10000), "Validation set size")
            ("layer1", po::value(&config.layer1)->default_value(32), "Layer 1 size")
            ("layer2", po::value(&config.layer2)->default_value(16), "Layer 2 size")
            ("batch_size", po::value(&config.batch_size)->default_value(50), "Batch size")
            ("population", po::value(&config.population)->default_value(192), "Population size")
            ("max_evals", po::value(&config.max_evals)->default_value(500000), "Max evaluations")
            ("sigma", po::value(&config.sigma)->default_value(0.1), "Sigma")
            ("seed", po::value(&config.seed)->default_value(1), "Random seed");

    // --- Mode 4: Testing ---
    po::options_description test_opts("Mode 4 Options (--test)");
    test_opts.add_options()
            ("files_dir", po::value(&config.files_dir), "Directory containing files for testing")
            ("training_output_dir", po::value(&config.training_output_dir), "Directory with saved NN weights")
            ("output_dir", po::value(&config.output_dir), "Output directory")
            ("population", po::value(&config.population)->default_value(192), "Population size")
            ("max_evals", po::value(&config.max_evals)->default_value(500000), "Max evaluations")
            ("sigma", po::value(&config.sigma)->default_value(0.1), "Sigma")
            ("schedules", po::bool_switch(&config.schedules), "Save schedules")
            ("graphics", po::bool_switch(&config.graphics), "Show graphics")
            ("time_limit", po::value(&config.time_limit)->default_value(60), "Time limit (s)")
            ("seed", po::value(&config.seed)->default_value(1), "Random seed");

    // -------------------------------------------------------------------------
    // 2. PARSE GLOBAL OPTIONS
    // -------------------------------------------------------------------------
    po::variables_map vm_global;
    try {
        // Parse only the global flags first.
        // 'allow_unregistered' lets mode-specific options pass through without error.
        po::parsed_options parsed = po::command_line_parser(argc, argv)
                .options(global_opts)
                .allow_unregistered()
                .run();
        po::store(parsed, vm_global);
        po::notify(vm_global);

        // Check Help
        if (vm_global.count("help")) {
            std::cout << "Usage: program [mode] [options]\n\n";
            std::cout << global_opts << "\n";
            std::cout << gen_opts << "\n";
            std::cout << brand_opts << "\n";
            std::cout << train_opts << "\n";
            std::cout << test_opts << "\n";
            exit(0);
        }

        // Collect tokens NOT handled by global parser (the mode-specific ones)
        std::vector<std::string> unrecognized = po::collect_unrecognized(parsed.options, po::include_positional);

        // -------------------------------------------------------------------------
        // 3. DETERMINE MODE AND PARSE SPECIFICS
        // -------------------------------------------------------------------------

        // Identify which mode is active
        bool is_brandimarte = has_option(unrecognized, "--brandimarte");

        config.mode_gen_rand = config.generate && !is_brandimarte;
        config.mode_gen_brand = config.generate && is_brandimarte;
        config.mode_train = config.train;
        config.mode_test = config.test;

        // Enforce Single Mode Selection
        int modes_active = (config.generate ? 1 : 0) + (config.train ? 1 : 0) + (config.test ? 1 : 0);
        if (modes_active == 0) throw std::runtime_error("No mode selected. Use --generate, --train, or --test.");
        if (modes_active > 1) throw std::runtime_error("Multiple modes selected. Please choose only one.");

        po::variables_map vm_local;

        if (config.mode_gen_rand) {
            // Parse using Mode 1 rules
            po::store(po::command_line_parser(unrecognized).options(gen_opts).run(), vm_local);
            po::notify(vm_local);

            // Validation
            if (!vm_local.count("machines")) throw std::runtime_error("Missing --machines");
            if (!vm_local.count("set_size")) throw std::runtime_error("Missing --set_size");
            check_positive("machines", config.machines);
            check_positive("operation_types", config.operation_types);
            check_range("jobs_min", config.jobs_min, "jobs_max", config.jobs_max);
            check_range("job_len_min", config.job_len_min, "job_len_max", config.job_len_max);
            check_range("num_alt_min", config.num_alt_min, "num_alt_max", config.num_alt_max);
            check_range("t_min", config.t_min, "t_max", config.t_max);
        } else if (config.mode_gen_brand) {
            // Parse using Mode 2 rules
            po::store(po::command_line_parser(unrecognized).options(brand_opts).run(), vm_local);
            po::notify(vm_local);

            // Validation
            if (!vm_local.count("brandimarte")) throw std::runtime_error("Missing --brandimarte ID");
            if (!vm_local.count("set_size")) throw std::runtime_error("Missing --set_size");
            if (config.brandimarte < 1 || config.brandimarte > 15)
                throw std::runtime_error("Brandimarte ID must be between 1 and 15.");
        } else if (config.mode_train) {
            // Parse using Mode 3 rules
            po::store(po::command_line_parser(unrecognized).options(train_opts).run(), vm_local);
            po::notify(vm_local);

            // Validation
            if (!vm_local.count("files_dir")) throw std::runtime_error("Missing --files_dir");
            check_positive("val_set_size", config.val_set_size);
            check_positive("layer1", config.layer1);
            check_positive("batch_size", config.batch_size);
        } else if (config.mode_test) {
            // Parse using Mode 4 rules
            po::store(po::command_line_parser(unrecognized).options(test_opts).run(), vm_local);
            po::notify(vm_local);

            // Validation
            if (!vm_local.count("files_dir")) throw std::runtime_error("Missing --files_dir");
            if (!vm_local.count("training_output_dir")) throw std::runtime_error("Missing --training_output_dir");
            check_positive("population", config.population);
        }
    } catch (const std::exception &e) {
        ERROR("Command Line Error: " + std::string(e.what()) + "\nUse --help to see valid options.");
        exit(1);
    }

    return config;
}

// -----------------------------------------------------------------------------
// Output Stream Implementation (Preserved Logic)
// -----------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const Config &c) {
    os << "========================================\n";
    os << "         CONFIGURATION SUMMARY          \n";
    os << "========================================\n";

    if (c.mode_gen_rand) {
        os << "[Mode: Synthetic Generation]\n";
        os << "  Output Directory : " << c.output_dir << "\n";
        os << "  Set Size         : " << c.set_size << "\n";
        os << "  Machines         : " << c.machines << "\n";
        os << "  Operation Types  : " << c.operation_types << "\n";
        os << "  Jobs             : [" << c.jobs_min << ", " << c.jobs_max << "]\n";
        os << "  Job Length       : [" << c.job_len_min << ", " << c.job_len_max << "]\n";
        os << "  Alt. Machines    : [" << c.num_alt_min << ", " << c.num_alt_max << "]\n";
        os << "  Processing Time  : [" << c.t_min << ", " << c.t_max << "]\n";
        os << "  Common Seed      : " << c.common_seed << "\n";
    } else if (c.mode_gen_brand) {
        os << "[Mode: Brandimarte Generation]\n";
        os << "  Output Directory : " << c.output_dir << "\n";
        os << "  Problem Instance : MK" << c.brandimarte << "\n";
        os << "  Set Size         : " << c.set_size << "\n";
    } else if (c.mode_train) {
        os << "[Mode: Training]\n";
        os << "  Train Files Dir  : " << c.files_dir << "\n";
        os << "  Output Directory : " << c.output_dir << "\n";
        os << "  Val Set Size     : " << c.val_set_size << "\n";
        os << "  Batch Size       : " << c.batch_size << "\n";
        os << "  Layers           : " << c.layer1 << " -> " << c.layer2 << "\n";
    } else if (c.mode_test) {
        os << "[Mode: Testing]\n";
        os << "  Test Files Dir   : " << c.files_dir << "\n";
        os << "  Model Weights    : " << c.training_output_dir << "\n";
        os << "  Output Directory : " << c.output_dir << "\n";
        os << "  Schedules        : " << (c.schedules ? "Enabled" : "Disabled") << "\n";
        os << "  Time Limit       : " << c.time_limit << "s\n";
    }

    // Common Settings
    if (c.mode_train || c.mode_test) {
        os << "----------------------------------------\n";
        os << "  Population       : " << c.population << "\n";
        os << "  Max Evals        : " << c.max_evals << "\n";
        os << "  Sigma            : " << c.sigma << "\n";
    }

    os << "  Seed             : " << c.seed << "\n";
    os << "========================================\n";
    return os;
}
