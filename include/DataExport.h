#pragma once
#include <string>
#include "JobshopData.h"

namespace jobshop {
    using namespace std;


    void dataExport(const vector<JobshopData> &DIO, string dir, string instancesFile);

    void dataExport2(const vector<JobshopData> &DIO, string dir, string instancesFile);

    void dataExport_fjs(const vector<JobshopData> &DIOs, string dir, string fjsFileName);
}
