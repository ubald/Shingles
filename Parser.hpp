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
    static std::vector<std::string> parse(const std::string text, bool debug = false);

private:
    static std::vector<std::string> parseChunk(std::string textBuffer, bool debug = false);
};


#endif //SHINGLES_PARSER_HPP
