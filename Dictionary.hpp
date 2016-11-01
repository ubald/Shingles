#ifndef SHINGLES_DICTIONARY_HPP
#define SHINGLES_DICTIONARY_HPP

#include <algorithm>
#include <string>
#include <regex>
#include <vector>
#include <iostream>
#include <map>
#include <random>
#include <stack>
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
    unsigned long idCounter{5}; // 0-5 reserved for begin & end words
    std::unique_ptr<Word> beginSentence{std::make_unique<Word>(0, "")};
    std::unique_ptr<Word> endSentence{std::make_unique<Word>(1, "")};
    std::unique_ptr<Word> beginQuote{std::make_unique<Word>(2, "\"")};
    std::unique_ptr<Word> endQuote{std::make_unique<Word>(3, "\"")};
    std::unique_ptr<Word> beginParens{std::make_unique<Word>(4, "(")};
    std::unique_ptr<Word> endParens{std::make_unique<Word>(5, ")")};
    std::vector<std::unique_ptr<Word>> learnedWords{};
    std::map<std::string,Word*> wordMap{
            {"<s>", beginSentence.get()},
            {"</s>", beginSentence.get()},
            {"<q>", beginQuote.get()},
            {"</q>", endQuote.get()},
            {"<p>", beginParens.get()},
            {"</p>", endParens.get()}
    };
};


#endif //SHINGLES_DICTIONARY_HPP
