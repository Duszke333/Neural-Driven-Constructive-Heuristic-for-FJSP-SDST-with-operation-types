#pragma once
#include <atomic>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

namespace chof {
    /**
     * @brief Snapshot of the training state sufficient to resume a CMA-ES run.
     *
     * The CMA-ES evolution paths are private in libcmaes and are not stored; on resume they restart
     * from zero (like a fresh run) while mean, step size, evaluation counters, best-so-far weights
     * and the batch-sampling RNG are restored.
     */
    struct TrainingCheckpoint {
        std::string fingerprint; ///< Identifies the training configuration; must match on resume.
        int stage = 0; ///< Index of the current outer CMA-ES stage (Config.NumEvals index).
        int fevals = 0; ///< Function evaluations done in the current stage.
        int niter = 0; ///< CMA-ES iterations done in the current stage.
        int lastBatch = 0; ///< Validation-cadence counter.
        double sigma = 0.0; ///< Effective step size (sigma scaled by mean per-coordinate std).
        double bestObj = 0.0; ///< Best validation objective seen so far.
        double elapsedSeconds = 0.0; ///< Training time accumulated over previous runs.
        std::vector<double> xmean; ///< CMA-ES distribution mean.
        std::vector<double> bestX; ///< Weights with the best validation score.
        std::string rngState; ///< Serialized std::mt19937 state.

        template<class Archive>
        void serialize(Archive &ar, const unsigned int) {
            ar & fingerprint & stage & fevals & niter & lastBatch & sigma & bestObj & elapsedSeconds
                    & xmean & bestX & rngState;
        }
    };

    /// @brief Saves a checkpoint atomically (temp file + rename), so a power loss cannot corrupt it.
    inline bool saveCheckpoint(const std::string &path, const TrainingCheckpoint &cp) {
        const std::string tmp = path + ".tmp";
        {
            std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
            if (!ofs.is_open()) return false;
            boost::archive::binary_oarchive oa(ofs);
            oa << cp;
            ofs.flush();
            if (!ofs.good()) return false;
        }
        std::error_code ec;
        std::filesystem::rename(tmp, path, ec);
        return !ec;
    }

    /// @brief Loads a checkpoint; returns false if the file is missing or unreadable.
    inline bool loadCheckpoint(const std::string &path, TrainingCheckpoint &cp) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open()) return false;
        try {
            boost::archive::binary_iarchive ia(ifs);
            ia >> cp;
        } catch (const std::exception &) {
            return false;
        }
        return true;
    }

    /// @brief Flag set by SIGINT/SIGTERM; training checkpoints and stops gracefully when it is set.
    inline std::atomic<bool> &stopRequested() {
        static std::atomic<bool> flag{false};
        return flag;
    }

    /// @brief Installs SIGINT/SIGTERM handlers that request a graceful stop.
    inline void installStopHandlers() {
        auto handler = [](int) { stopRequested().store(true); };
        std::signal(SIGINT, handler);
        std::signal(SIGTERM, handler);
    }
}
