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
    void ingestFile(const std::string& filePath);
    void input(const std::string &text);
    void ingest(std::vector<std::string> &words, bool doUpdateProbabilities = false);
    void ingestSentence(std::vector<Word*> &sentenceWords, bool doUpdateProbabilities = false);
    void updateProbabilities();
    std::vector<const std::string> nextCandidateWords(std::string seed = "") const;
    std::string nextMostProbableWord(std::string seed = "") const;
    std::string generate(std::string seed = "") const;
    void open(const std::string &path);
    void save(const std::string &path) const;
    std::string toString() const;
    void setDebug();
    void setDebug(bool debug);

private:
    bool debug_{false};
    unsigned long n;
    unsigned long idCounter{5}; // 0-5 reserved for begin & end words
    Word* beginSentence{nullptr};
    Word* endSentence{nullptr};
    std::map<std::string, std::unique_ptr<Word>> wordMap{};
};


#endif //SHINGLES_DICTIONARY_HPP
