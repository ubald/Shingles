#ifndef SHINGLES_WORD_HPP
#define SHINGLES_WORD_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Gram.hpp"

class Word {
public:
    Word() = delete;
    Word(unsigned long id, const std::string text);
    unsigned long getId() const;
    const std::string getText() const;
    const Gram* getGram() const;
    const std::string toString() const;

    void updateGraph(const std::vector<Word*>& sentence, unsigned long position, unsigned long n);
    void updateProbabilities(unsigned long wordCount);
    const Word* next(const std::vector<const Word*>& sentence, unsigned long n) const;
private:
    unsigned long id;
    std::string text;
    Gram gram;
};


#endif //SHINGLES_WORD_HPP
