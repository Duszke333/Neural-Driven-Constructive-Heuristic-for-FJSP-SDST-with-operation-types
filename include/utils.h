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

    /**
     * @brief Returns current date and time formatted as a string (YYYY-MM-DD HH:MM:SS).
     * @return Formatted string representing current local time.
     */
    string getHumanReadableDateTime();

    /**
     * @brief Moves an element from one position to another within a std::vector using iterators.
     * @tparam T Type of elements in the vector.
     * @param _from Iterator pointing to the element to be moved.
     * @param _to Iterator pointing to the target position.
     */
    template<typename T>
    void moveElement(typename vector<T>::iterator _from, typename vector<T>::iterator _to) {
        if (_from < _to) {
            std::rotate(_from, _from + 1, _to);
        } else if (_from > _to) {
            std::rotate(_to, _from, _from + 1);
        }
    }

    /**
     * @brief Opens a file stream and creates any missing parent directories automatically.
     * * Useful for ensuring output directories exist before attempting to save files (e.g., CSV logs).
     * @tparam FileStream Stream type (e.g., std::ofstream, std::ifstream).
     * @param filePath Full path to the target file.
     * @param mode Open mode flags (defaults to std::ios_base::out for ofstream, in for ifstream).
     * @return Opened file stream ready for I/O operations.
     */
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
    }

    /**
     * @brief Normalizes a value from a given min-max range to the [0, 1] range.
     * @param minVal Minimum expected bound of the input.
     * @param maxVal Maximum expected bound of the input.
     * @param val The value to scale.
     * @return Scaled value in [0, 1].
     */
    inline float scale1(float minVal, float maxVal, float val) {
        return ((val - minVal) / (maxVal - minVal));
    }

    /**
     * @brief Normalizes a value from a given min-max range to the [-1, 1] range.
     * @param minVal Minimum expected bound of the input.
     * @param maxVal Maximum expected bound of the input.
     * @param val The value to scale.
     * @return Scaled value in [-1, 1].
     */
    inline float scale2(float minVal, float maxVal, float val) {
        return -1.0f + ((val - minVal) / (maxVal - minVal)) * 2;
    }

    /**
     * @brief Fast Hyperbolic Tangent (Tanh) activation function approximation.
     * * Bounds output strictly to [-1, 1]. Uses polynomial interpolation for speed.
     * @param x Input value.
     * @return Activation value in range [-1, 1].
     */
    inline float scaleTanh(float x) {
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

    /**
     * @brief Fast Hyperbolic Tangent (Tanh) approximation with a slight linear slope out of bounds.
     * * Similar to scaleTanh, but introduces a small gradient (0.01) outside the shift bounds
     * (like Leaky ReLU) to prevent vanishing gradients during optimization.
     * @param x Input value.
     * @return Activation value centered around [-1, 1] but slightly exceeding bounds for extreme inputs.
     */
    inline float scaleTanh2(float x) {
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

    /**
     * @brief Third-order polynomial approximation of Tanh.
     * * Provides smoother gradients for backpropagation and evolutionary algorithms.
     * * Also includes a tiny gradient leak outside the effective range.
     * @param x Input value.
     * @return Activation value predominantly in range [-1, 1].
     */
    inline float scaleTanh3(float x) {
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

    /**
     * @brief Fast Sigmoid activation function approximation.
     * * Bounds output strictly to [0, 1] using Smoothstep polynomial interpolation
     * instead of the computationally expensive exp() function.
     * @param x Input value.
     * @return Activation value in range [0, 1].
     */
    inline float scaleSigm(float x) {
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

    /**
     * @brief Clamps values strictly to the range [-1, 1] without smooth interpolation.
     * @param x Input value.
     * @return Clamped value.
     */
    inline float scaleZet(float x) {
        return max(min(1.0f, x), -1.0f);
    }

    /**
     * @brief Rectified Linear Unit (ReLU) activation function.
     * @param x Input value.
     * @return Max of 0 and x.
     */
    inline float scaleReLU(float x) {
        return max(0.f, x);
    }

    /**
     * @brief Gaussian activation function (Bell curve).
     * @param x Input value.
     * @return Value mapped to a bell curve peaking at x=0 (output 1) falling to 0 at extremes.
     */
    inline float scaleGauss(float x) {
        float th = scaleTanh(x);
        return 1.f - th * th;
    }

    /**
     * @brief Removes surrounding double quotes from a string, if they exist.
     * @param str Reference to the string to be processed. Modified in-place.
     */
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

    /**
     * @brief Reads a CSV file and loads its contents into a 2D string matrix.
     * * Values are automatically unquoted. Semicolon is used as the delimiter.
     * @param fn Path to the CSV file.
     * @param Table Output matrix where rows and columns will be stored.
     */
    void importCSV(string fn, vector<vector<string> > &Table);

    /**
     * @brief Converts an integer to a string padded with leading zeros to match a specific width.
     * @param n The integer to convert.
     * @param width The target length of the resulting string.
     * @return Padded string representation of the integer.
     */
    inline std::string to_string(int n, int width) {
        std::ostringstream oss;
        oss.width(width);
        oss.fill('0');
        oss << n;
        return oss.str();
    }

    /**
     * @brief Converts a numeric value to a string with a fixed floating-point precision.
     * @tparam T Type of the numeric value (e.g. double, float).
     * @param a_value The numeric value to convert.
     * @param n Number of decimal places (precision). Defaults to 6.
     * @return String representation of the value with fixed precision.
     */
    template<typename T>
    std::string to_string_with_precision(const T a_value, const int n = 6) {
        std::ostringstream out;
        out.precision(n);
        out << std::fixed << a_value;
        return std::move(out).str();
    }

    /**
     * @brief Fast approximation of the atan2(y, x) function.
     * * Useful for extremely fast angle calculations where perfect precision is not required.
     * @param x X-coordinate.
     * @param y Y-coordinate.
     * @return Approximated angle in radians.
     */
    float atan2_fast(const float x, const float y);
}
