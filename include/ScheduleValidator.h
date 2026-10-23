#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "JobshopData.h"
#include "SetupTimeProvider.h"

namespace jobshop {
    /**
     * @brief Checks the schedule stored in Data.Solution against the problem definition.
     * Verifies that every operation is scheduled once, durations match, operations of a job follow each other,
     * machines process one operation at a time and respect the setup time of the given mode, and that the
     * objective equals the makespan.
     * @return Empty string if the schedule is valid, otherwise a description of the first violation.
     */
    inline std::string validateSchedule(const JobshopData &Data, SetupMode mode) {
        using std::to_string;
        SetupTimeProvider Setup{mode};
        const auto &Decs = Data.Solution.Decs;

        size_t total = 0;
        for (auto &J: Data.Jobs) total += J.Ops.size();
        if (Decs.size() != total) return "wrong number of scheduled operations";

        std::vector<std::vector<int> > JobEnd(Data.numJ);
        std::vector<std::vector<char> > Seen(Data.numJ);
        for (auto &J: Data.Jobs) {
            JobEnd[J.idx].assign(J.Ops.size(), -1);
            Seen[J.idx].assign(J.Ops.size(), 0);
        }
        int makespan = 0;
        for (auto &D: Decs) {
            if (D.j < 0 || D.j >= Data.numJ || D.iop < 0 || D.iop >= (int) Data.Jobs[D.j].Ops.size() ||
                D.m < 0 || D.m >= Data.numM)
                return "decision out of range";
            if (Seen[D.j][D.iop]) return "operation scheduled twice (job " + to_string(D.j) + ")";
            Seen[D.j][D.iop] = 1;
            JobEnd[D.j][D.iop] = D.end_t;
            int o = Data.Jobs[D.j].Ops[D.iop];
            if (Data.OMtime[o][D.m] <= 0) return "machine not eligible (job " + to_string(D.j) + ")";
            if (D.start_t < 0) return "negative start time (job " + to_string(D.j) + ")";
            if (D.end_t - D.start_t != Data.OMtime[o][D.m]) return "wrong duration (job " + to_string(D.j) + ")";
            makespan = std::max(makespan, D.end_t);
        }
        for (auto &D: Decs) {
            if (D.iop > 0 && D.start_t < JobEnd[D.j][D.iop - 1])
                return "job precedence violated (job " + to_string(D.j) + ")";
        }

        std::vector<std::vector<JobshopData::Dec> > PerMachine(Data.numM);
        for (auto &D: Decs) PerMachine[D.m].push_back(D);
        for (int m = 0; m < Data.numM; m++) {
            auto &V = PerMachine[m];
            std::sort(V.begin(), V.end(), [](auto &A, auto &B) { return A.start_t < B.start_t; });
            int prevEnd = 0, prevType = -1;
            for (auto &D: V) {
                int o = Data.Jobs[D.j].Ops[D.iop];
                int s = Setup(Data, m, prevType, o);
                if (D.start_t < prevEnd + s) return "setup/overlap violated on machine " + to_string(m);
                if (D.setup_start != prevEnd)
                    return "setup does not start when machine becomes free on machine " + to_string(m);
                if (D.setup_t != s) return "recorded setup differs from the setup matrix on machine " + to_string(m);
                prevEnd = D.end_t;
                prevType = o;
            }
        }
        if (std::llround(Data.Solution.getObj()) != makespan) return "objective differs from makespan";
        return "";
    }
}
