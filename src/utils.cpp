#include <chrono>
#include <ctime>
#include <string>
#include "utils.h"

namespace nnutils {
    string getHumanReadableDateTime() {
        std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm local_time = *std::localtime(&now_c);

        std::ostringstream oss;
        oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

        return oss.str();
    }

    void importCSV(string fn, vector<vector<string> > &Table) {
        ifstream fin(fn);

        Table.clear();

        if (!fin) {
            throw "file read errror (" + fn + ")";
        }
        vector<string> row;
        string line, word, temp;

        while (!fin.eof()) {
            row.clear();

            // read an entire row and
            // store it in a string variable 'line'
            getline(fin, line);

            // used for breaking words
            stringstream s(line);
            // read every column data of a row and
            // store it in a string variable, 'word'
            while (getline(s, word, ';')) {
                // add all the column data
                // of a row to a vector
                unquote(word);
                row.push_back(word);
            }


            if (!row.empty()) {
                Table.push_back(row);
            }
        }
    }
}