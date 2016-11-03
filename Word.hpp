#ifndef SHINGLES_WORD_HPP
#define SHINGLES_WORD_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <json/json.h>
#include "Gram.hpp"

class Word {
public:
    Word() = delete;
    Word(unsigned long id, const std::string text);
    Word(unsigned long id, const std::string inputText, const std::string outputText);
    unsigned long getId() const;
    const std::string getInputText() const;
    const std::string getOutputText() const;
    const Gram* getGram() const;
    const std::string toString() const;
    const Json::Value toJson() const;
    static std::unique_ptr<Word> fromJson(const Json::Value word_json);
    void fromJson(const Json::Value word_json, const std::map<unsigned long, std::unique_ptr<Word>> &wordsById);

    bool isMarker() const;
    bool isBeginMarker() const;
    bool isEndMarker() const;
    const Word* getBeginMarker() const;
    const Word* getEndMarker() const;
    void setAsBeginMarker(const Word *endMarker);
    void setAsEndMarker(const Word* beginMarker);

    void updateGraph(const std::vector<Word*>& sentence, unsigned long position, unsigned long n);
    void updateProbabilities(unsigned long wordCount);
    const Word* next(const std::vector<const Word*>& sentence, unsigned long n, const std::stack<const Word*> &markerStack, bool debug = false) const;
private:
    unsigned long id;
    const Word* beginMarker{};
    const Word* endMarker{};
    std::string inputText;
    std::string outputText;
    Gram gram;
};


#endif //SHINGLES_WORD_HPP
