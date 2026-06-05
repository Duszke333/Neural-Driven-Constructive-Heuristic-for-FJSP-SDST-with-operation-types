#pragma once
#include <boost/program_options.hpp>
#include <string>
#include <ostream>

namespace po = boost::program_options;

/**
 * @brief Global configuration structure holding parsed Command Line Interface (CLI) arguments.
 * * This struct dictates the execution mode (train/test/generate) and provides
 * all necessary hyperparameters for the CMA-ES optimizer and data generators.
 */
struct Config {
    // ===============
    // EXECUTION MODES
    // ===============
    bool mode_gen_rand = false; ///< Flag for random synthetic data generation mode.
    bool mode_train = false; ///< Flag for training (optimization) mode.
    bool mode_test = false; ///< Flag for testing (evaluation/inference) mode.
    bool mode_gen_brand = false; ///< Flag for Brandimarte benchmark generation mode.

    bool generate = false; ///< General flag indicating any generation task.
    bool train = false; ///< General flag indicating training.
    bool test = false; ///< General flag indicating testing.

    // =================
    // DIRECTORIES & I/O
    // =================
    std::string output_dir; ///< Main directory for saving results and models.
    std::string files_dir; ///< Directory containing input datasets.
    std::string training_output_dir; ///< Directory containing pre-trained models (for testing).

    int seed; ///< Global RNG seed for reproducibility.

    // =========================
    // NEURAL NETWORK & TRAINING
    // =========================
    int val_set_size; ///< Number of instances held out for validation (TSS).
    int batch_size; ///< Size of the mini-batch during training.
    int layer1; ///< Number of neurons in the first hidden layer.
    int layer2; ///< Number of neurons in the second hidden layer.

    // ==================
    // OPTIMIZER (CMA-ES)
    // ==================
    int population; ///< Number of candidates per generation (lambda).
    int max_evals; ///< Absolute maximum number of objective function evaluations.
    double sigma; ///< Initial step size (standard deviation) for CMA-ES.
    bool schedules; ///< If true, saves the full schedules to CSV during testing.
    bool graphics; ///< If true, triggers JobshopDrawer to render Gantt charts.
    int time_limit; ///< Max execution time in seconds.

    // ===================
    // SYNTHETIC GENERATOR
    // ===================
    int machines; ///< Number of machines to generate.
    int operation_types; ///< Number of operation types to generate.
    int jobs_min; ///< Minimum number of jobs.
    int jobs_max; ///< Maximum number of jobs.
    int job_len_min; ///< Minimum operations per job.
    int job_len_max; ///< Maximum operations per job.
    int num_alt_min; ///< Minimum alternative machines per operation.
    int num_alt_max; ///< Maximum alternative machines per operation.
    int t_min; ///< Minimum base processing time.
    int t_max; ///< Maximum base processing time.
    int set_size; ///< Total number of instances to generate.
    int common_seed; ///< Seed for shared environment parameters.

    // =====================
    // BRANDIMARTE GENERATOR
    // =====================
    int brandimarte; ///< Brandimarte instance ID to generate (e.g., 1-10).
};


std::ostream &operator<<(std::ostream &os, const Config &c);

Config parse_command_line(int argc, char *argv[]);
