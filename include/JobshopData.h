#pragma once
#include  <string>
#include <vector>
#include "DataConcept.h"


namespace jobshop {
    using namespace std;
    using namespace chof;


    /**
     * @brief Structure representing the entire Flexible Job Shop Problem instance.
     * Contains all necessary data to evaluate and schedule operations, including
     * processing times and sequence-dependent setup times.
     */
    struct JobshopData {
        struct JType {
            int idx = -1; ///< Unique identifier of the job.
            vector<int> Ops; ///< Ordered sequence of operation types required to complete this job.

            bool operator==(const JType &o) const {
                return idx == o.idx;
            }

            bool operator!=(const JType &o) const {
                return !(*this == o);
            }

            JType() {
            }

            JType(int _idx) : idx(_idx) {
            }

            JType(int _idx, const vector<int> &ops) : idx(_idx), Ops(ops) {
            }
        };

        /**
         * @brief Represents a single scheduling decision (a block on a Gantt chart).
         */
        struct Dec {
            int m = -1; ///< Assigned machine ID.
            int start_t = -1; ///< Start time of the operation.
            int end_t = -1; ///< End time of the operation.
            int j = -1; ///< Assigned job ID.
            int iop = -1; ///< Index of the operation within the job j's sequence.

            Dec() {
            }

            Dec(int _m, int _o, int _e, int _j, int _iop) : m(_m), start_t(_o), end_t(_e), j(_j), iop(_iop) {
            }
        };

        /**
         * @brief Holds the generated solution (schedule) for this problem instance.
         * * Compliant with DataConcept interface.
         * @see DataConcept
         */
        struct SolutionType {
            double obj; ///< Objective function value (e.g., makespan).
            vector<Dec> Decs; ///< List of all scheduling decisions made.
            int cluster = -1; ///< Optional cluster ID for grouped heuristics.

            double getObj() const {
                return obj;
            }

            void setObj(double _obj) {
                obj = _obj;
            }

            void setCluster(int c) {
                cluster = c;
            }
        };

        // DataConcept interface methods
        double getObj() const {
            return Solution.getObj();
        }

        const SolutionType &getSolution() {
            return Solution;
        }

        void setSolution(const SolutionType &_solution) {
            Solution = _solution;
        }

        string name; ///< Name or identifier of the instance (e.g., from a file).

        int numJ; ///< Total number of jobs.
        int numO; ///< Total number of unique operation types.
        int numM; ///< Total number of available machines.

        vector<JType> Jobs; ///< List of all jobs in the problem.

        /**
         * @brief Processing times matrix.
         * Indexed as [OperationType][Machine].
         * A value of 0 indicates that the operation cannot be processed on the given machine.
         */
        vector<vector<int> > OMtime;

        /**
         * @brief Sequence-Dependent Setup Times (SDST) 3D matrix.
         * Indexed as [Machine][PreviousOperationType][NextOperationType].
         */
        vector<vector<vector<int> > > setupTimes;

        SolutionType Solution; ///< Current best solution for this instance.

        JobshopData() {
        }

        /**
         * @brief Prints the schedule in a human-readable CSV format.
         * @param os Output stream to write the schedule to.
         */
        void printSolution(std::ostream &os) {
            auto SortedDecs = Solution.Decs;
            sort(SortedDecs.begin(), SortedDecs.end(), [](auto &A, auto &B) {
                return A.m < B.m || A.m == B.m && A.start_t < B.start_t;
            });

            os << "machine; job; operation number; operation_type; start_time; end_time " << endl;
            for (auto &Dec: SortedDecs) {
                os << Dec.m << "; " << Dec.j << ";" << Dec.iop << ";" << Jobs[Dec.j].Ops[Dec.iop] << "; " << Dec.start_t
                        << "; " << Dec.end_t << endl;
            }
        }
    };

    static_assert(SolutionConcept<JobshopData::SolutionType>);
    static_assert(DataConcept<JobshopData>);
}
