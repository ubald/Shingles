#ifndef SHINGLES_SPLIT_HPP
#define SHINGLES_SPLIT_HPP

#include <string>
#include <iostream>
#include <sstream>

inline std::vector<std::string> split(std::string text, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss;
    ss.str(text);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

#endif //SHINGLES_SPLIT_HPP
