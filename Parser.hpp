#ifndef SHINGLES_PARSER_HPP
#define SHINGLES_PARSER_HPP

#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <sstream>

class Parser {
public:
    Parser() = delete;
    static void parse(const std::string text, std::function<void(std::vector<std::string>&)> callback, std::function<void(void)> done, bool debug = false);
    static std::vector<std::string> parseChunk(std::string textBuffer, bool debug = false);
};


#endif //SHINGLES_PARSER_HPP
