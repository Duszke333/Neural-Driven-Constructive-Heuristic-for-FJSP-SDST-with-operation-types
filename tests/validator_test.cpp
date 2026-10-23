#include <cstdlib>
#include <iostream>
#include "ScheduleValidator.h"

using namespace jobshop;

namespace {
    int failures = 0;

    void expect(bool ok, const char *what) {
        if (!ok) {
            std::cerr << "FAIL: " << what << std::endl;
            failures++;
        }
    }

    // 2 jobs, 2 machines, 2 op types; type 0: 3 on m0, type 1: 2 on m1; setup between different types = 1, same type = 0.
    JobshopData makeValid() {
        JobshopData D;
        D.numJ = 2;
        D.numO = 2;
        D.numM = 2;
        D.Jobs = {JobshopData::JType(0, {0, 1}), JobshopData::JType(1, {0, 1})};
        D.OMtime = {{3, 0}, {0, 2}};
        D.setupTimes.assign(2, std::vector<std::vector<int> >(2, std::vector<int>(2, 1)));
        for (int m = 0; m < 2; m++)
            for (int t = 0; t < 2; t++) D.setupTimes[m][t][t] = 0;
        D.Solution.Decs = {
            JobshopData::Dec(0, 0, 3, 0, 0), JobshopData::Dec(0, 3, 6, 1, 0),
            JobshopData::Dec(1, 3, 5, 0, 1), JobshopData::Dec(1, 6, 8, 1, 1)
        };
        D.Solution.Decs[0].setup_start = 0;
        D.Solution.Decs[1].setup_start = 3;
        D.Solution.Decs[2].setup_start = 0;
        D.Solution.Decs[3].setup_start = 5;
        for (auto &d: D.Solution.Decs) d.setup_t = 0;
        D.Solution.setObj(8);
        return D;
    }
}

int main() {
    const SetupMode mode = SetupMode::Real;
    expect(validateSchedule(makeValid(), mode).empty(), "valid schedule accepted");

    auto D = makeValid();
    D.Solution.Decs.pop_back();
    expect(!validateSchedule(D, mode).empty(), "missing operation");

    D = makeValid();
    D.Solution.Decs[3].end_t = 9;
    expect(!validateSchedule(D, mode).empty(), "wrong duration");

    D = makeValid();
    D.Solution.Decs[2].m = 0;
    expect(!validateSchedule(D, mode).empty(), "ineligible machine");

    D = makeValid();
    D.Solution.Decs[2].start_t = 2;
    D.Solution.Decs[2].end_t = 4;
    expect(!validateSchedule(D, mode).empty(), "job precedence violated");

    D = makeValid();
    D.Solution.Decs[1].start_t = 2;
    D.Solution.Decs[1].end_t = 5;
    expect(!validateSchedule(D, mode).empty(), "machine overlap");

    D = makeValid();
    D.Solution.setObj(7);
    expect(!validateSchedule(D, mode).empty(), "objective differs from makespan");

    D = makeValid();
    D.Solution.Decs[3].setup_t = 1;
    expect(!validateSchedule(D, mode).empty(), "recorded setup mismatch");

    // Mode-sensitivity: a gap-less schedule with a real setup is invalid under Real but valid under Zero.
    D = makeValid();
    D.OMtime = {{3, 3}, {3, 2}};
    D.Jobs = {JobshopData::JType(0, {0}), JobshopData::JType(1, {1})};
    D.numJ = 2;
    D.Solution.Decs = {JobshopData::Dec(0, 0, 3, 0, 0), JobshopData::Dec(0, 3, 6, 1, 0)};
    D.Solution.Decs[0].setup_start = 0;
    D.Solution.Decs[1].setup_start = 3;
    D.Solution.setObj(6);
    expect(!validateSchedule(D, SetupMode::Real).empty(), "setup ignored in real mode");
    expect(validateSchedule(D, SetupMode::Zero).empty(), "zero mode accepts gap-less schedule");

    if (failures == 0) std::cout << "validator-test: all passed" << std::endl;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
