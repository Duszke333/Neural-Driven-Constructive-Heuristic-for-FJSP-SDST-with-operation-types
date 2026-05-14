//
// Created by tsliwins on 01.02.23.
//

#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <numeric>
#include <sstream>
#include <functional>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "Err.h"
#include <system_error>
#include <algorithm>

namespace nnutils {
    using namespace std;


    string getHumanReadableDateTime();

    // template<class T>
    // void moveElement(std::vector<T> &vec, int from, int to) {
    //     if (from < to) {
    //         std::rotate(vec.begin() + from, vec.begin() + from + 1, vec.begin() + to);
    //     } else if (from > to) {
    //         std::rotate(vec.begin() + to, vec.begin() + from, vec.begin() + from + 1);
    //     }
    // }

    template<typename T>
    void moveElement(typename vector<T>::iterator _from, typename vector<T>::iterator _to) {
        if (_from < _to) {
            std::rotate(_from, _from + 1, _to);
        } else if (_from > _to) {
            std::rotate(_to, _from, _from + 1);
        }
    }

    // Function template to handle file opening with directory creation
    template<typename FileStream>
    FileStream openFileWithDirs(const std::string &filePath,
                                std::ios_base::openmode mode = (std::is_same_v<FileStream, std::ofstream>
                                                                    ? FileStream::out
                                                                    : FileStream::in)) {
        namespace fs = std::filesystem;


        try {
            // Extract directory part from the given file path
            fs::path fullPath(filePath);
            fs::path directory = fullPath.parent_path();

            if constexpr (std::is_same_v<FileStream, std::ofstream>) {
                // Create directories if they do not exist
                if (!directory.empty() && !fs::exists(directory)) {
                    if (!fs::create_directories(directory)) {
                        INTERNAL(string("Failed to create directories: ") + directory.c_str());
                    }
                }
            }

            FileStream fileStream(filePath, mode);
            if (!fileStream.is_open()) {
                std::error_code ec;
                ec.assign(errno, std::system_category());
                std::cerr << "Error: " << ec.message() << " (code " << ec.value() << ")\n";
                INTERNAL(
                    string("Failed to open file: ") + filePath.c_str() + " (" + ec.message() + ", " + std::to_string(
                        ec.value()) + ")");
            }

            return fileStream;
        } catch (const std::exception &e) {
            INTERNAL("Exception occurred: " + e.what());
        }

        return FileStream();
    }


    // // Helper function to combine hash values
    // template <typename T>
    // inline void hash_combine(std::size_t& seed, const T& value) {
    //     seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    // }
    //
    // // Variadic function to hash multiple variables
    // template <typename... Args>
    // std::size_t hash_values(const Args&... args) {
    //     std::size_t seed = 0;
    //     (hash_combine(seed, args), ...);  // Fold expression in C++17
    //     return seed;
    // }

    inline float scale1(float minVal, float maxVal, float val) {
        return ((val - minVal) / (maxVal - minVal));
    }

    inline float scale2(float minVal, float maxVal, float val) {
        return -1.0f + ((val - minVal) / (maxVal - minVal)) * 2;
    }


    inline float scaleTanh(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        static const float shift = 3.5f;
        static const float rshift = 1.0f / 3.5f;
        if (x >= 0.f) {
            if (x >= shift) return 1.0f;
            float tmp = (x - shift) * rshift;
            return 1.0f - tmp * tmp * tmp * tmp;
        } else if (x >= -shift) {
            float tmp = (x + shift) * rshift;
            return -1.0f + tmp * tmp * tmp * tmp;
        } else {
            return -1.0f;
        }
    }

    inline float scaleTanh2(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        static const float shift = 3.5f;
        static const float rshift = 1.0f / 3.5f;
        if (x >= 0.f) {
            if (x >= shift) return 1.0f + (x - shift) * 0.01;
            float tmp = (x - shift) * rshift;
            return 1.0f - tmp * tmp * tmp * tmp;
        } else if (x >= -shift) {
            float tmp = (x + shift) * rshift;
            return -1.0f + tmp * tmp * tmp * tmp;
        } else {
            return -1.0f - (shift - x) * 0.01;
        }
    }


    inline float scaleTanh3(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        static const float shift = 3.5f;
        static const float rshift = 1.0f / 3.5f;
        static const float range = 2.779f;
        static const float _0998 = 0.998f;
        if (x >= 0.f) {
            if (x >= range) return _0998 + (x - range) * 0.01f;
            float tmp = (x - shift) * rshift;
            return 1.0f - tmp * tmp * tmp * tmp;
        } else if (x >= -range) {
            float tmp = (x + shift) * rshift;
            return -1.0f + tmp * tmp * tmp * tmp;
        } else {
            return -_0998 + (x + range) * 0.01;
        }
    }


    // inline float scaleTanh(float x) {
    //     return tanhf(x);
    // }

    inline float scaleSigm(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        static const float shift = 3.5f;
        static const float rshift = 1.0f / 3.5f;
        if (x >= 0.f) {
            if (x >= shift) return 1.0f;
            float tmp = (x - shift) * rshift;
            return 1.0f - tmp * tmp * tmp * tmp / 2;
        } else if (x >= -shift) {
            float tmp = (x + shift) * rshift;
            return -1.0f + tmp * tmp * tmp * tmp / 2;
        } else {
            return 0.0f;
        }
    }

    inline float scaleZet(float x) {
        return max(min(1.0f, x), -1.0f);
    }


    inline float scaleReLU(float x) {
        //    static const float shift = 4.0f;
        //    static const float rshift = 0.25f;
        return max(0.f, x);
    }


    inline float scaleGauss(float x) {
        float th = scaleTanh(x);
        return 1.f - th * th;
    }

    inline void unquote(string &str) {
        if (str.size() > 1) {
            if (str.front() == '"' && str.back() == '"') {
                if (str.size() == 2) {
                    str.erase();
                } else {
                    str.erase(str.begin());
                    str.erase(str.end() - 1);
                }
            }
        }
    }

    void importCSV(string fn, vector<vector<string> > &Table);


    inline std::string to_string(int n, int width) {
        std::ostringstream oss;
        oss.width(width);
        oss.fill('0');
        oss << n;
        return oss.str();
    }

    template<typename T>
    std::string to_string_with_precision(const T a_value, const int n = 6) {
        std::ostringstream out;
        out.precision(n);
        out << std::fixed << a_value;
        return std::move(out).str();
    }

    float atan2_fast(const float x, const float y);
}