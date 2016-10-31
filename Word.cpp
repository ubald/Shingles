#include <iostream>
#include "Word.hpp"

Word::Word(unsigned long id, const std::string text) : id(id), text(text), gram(this) {

}

unsigned long Word::getId() const {
    return id;
}

const std::string Word::getText() const {
    return text;
}

const Gram* Word::getGram() const {
    return &gram;
}

const std::string Word::toString() const {
    std::stringstream ss;
    ss << gram.toString();
    return ss.str();
}

void Word::updateGraph(const std::vector<Word *> &sentence, unsigned long position, unsigned long n) {
    gram.update(sentence, position, n);
}

void Word::updateProbabilities(unsigned long wordCount) {
    gram.computeProbability(wordCount);
}

const Word *Word::next(const std::vector<const Word *> &sentence, unsigned long position) const {
    Gram *g = gram.next(sentence, position);
    return g ? g->getWord() : nullptr;
}