#ifndef SHINGLES_DICTIONARY_HPP
#define SHINGLES_DICTIONARY_HPP

#include <algorithm>
#include <string>
#include <regex>
#include <vector>
#include <iostream>
#include <map>
#include <random>
#include "utils/split.hpp"
#include "Word.hpp"

class Dictionary {
public:
    Dictionary(unsigned long n = 3);
    void ingest(const std::string& sentence, bool doUpdateProbabilities = false);
    void updateProbabilities();
    std::string generate(std::string seed = "");
    std::string toString();

private:
    unsigned long n;
    unsigned long idCounter{2}; // 0-1 reserved for begin & end words
    std::unique_ptr<Word> beginWord{std::make_unique<Word>(0, "<s>")};
    std::unique_ptr<Word> endWord{std::make_unique<Word>(1, "</s>")};
    std::map<std::string,std::unique_ptr<Word>> words{};
};


#endif //SHINGLES_DICTIONARY_HPP
