#pragma once
#include <string>
#include "JobshopData.h"

namespace jobshop {
    using namespace std;

    /**
     * @brief Exports a dataset of Jobshop problem instances to a standard format.
     * @param DIO Vector of problem instances (JobshopData).
     * @param dir Destination directory path.
     * @param instancesFile Name of the output file.
     */
    void dataExport(const vector<JobshopData> &DIO, string dir, string instancesFile);

    /**
     * @brief Exports a dataset of Jobshop problem instances to an alternative format.
     * @param DIO Vector of problem instances (JobshopData).
     * @param dir Destination directory path.
     * @param instancesFile Name of the output file.
     */
    void dataExport2(const vector<JobshopData> &DIO, string dir, string instancesFile);

    /**
     * @brief Exports the dataset to the standard Flexible Job Shop (FJS) benchmark format.
     * @param DIOs Vector of problem instances.
     * @param dir Destination directory path.
     * @param fjsFileName Base name for the exported .fjs files.
     */
    void dataExport_fjs(const vector<JobshopData> &DIOs, string dir, string fjsFileName);
}
