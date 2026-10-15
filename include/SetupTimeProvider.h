#pragma once
#include <algorithm>
#include <string>
#include "JobshopData.h"
#include "Err.h"

namespace jobshop {
    /**
     * @brief Source of the setup-time values seen by the simulator and by the neural network.
     */
    enum class SetupMode {
        Real, ///< Values from JobshopData::setupTimes.
        Zero, ///< Every setup takes 0 (classic FJSP).
        One ///< Every real transition (previous type exists and differs) takes 1.
    };

    /**
     * @brief Parses the --setup_mode CLI value ("real", "zero", "one").
     */
    inline SetupMode parseSetupMode(const std::string &s) {
        if (s == "real") return SetupMode::Real;
        if (s == "zero") return SetupMode::Zero;
        if (s == "one") return SetupMode::One;
        ERROR("Unknown --setup_mode '" + s + "' (expected real, zero or one)");
        return SetupMode::Real;
    }

    /**
     * @brief The single place that reads setup times. Both the schedule simulation and the NN input go through
     * it, so mocking the mode changes them consistently.
     */
    struct SetupTimeProvider {
        SetupMode mode = SetupMode::Real;

        /**
         * @param Data Problem instance.
         * @param m Machine.
         * @param prevType Type of the last operation on the machine, -1 if the machine is still empty.
         * @param nextType Type of the operation being placed.
         * @return Setup time of the transition (0 for the first operation on a machine).
         */
        int operator()(const JobshopData &Data, int m, int prevType, int nextType) const {
            if (prevType < 0) return 0;
            switch (mode) {
                case SetupMode::Zero:
                    return 0;
                case SetupMode::One:
                    return prevType == nextType ? 0 : 1;
                case SetupMode::Real:
                default:
                    if (Data.setupTimes.empty()) return 0;
                    return Data.setupTimes[m][prevType][nextType];
            }
        }
    };
}
