#include <iostream>
#include "Word.hpp"

Word::Word(unsigned long id, const std::string text) : id(id), text(text), gram(this) {

}

unsigned long Word::getId() const {
    return id;
}

bool Word::isMarker() const {
    return beginMarker != nullptr || endMarker != nullptr;
}

bool Word::isBeginMarker() const {
    return endMarker != nullptr ;
}

bool Word::isEndMarker() const {
    return beginMarker != nullptr;
}

const Word* Word::getBeginMarker() const {
    return beginMarker;
}

void Word::setAsBeginMarker(const Word *end) {
    endMarker = end;
    beginMarker = nullptr;
}

const Word* Word::getEndMarker() const {
    return endMarker;
}

void Word::setAsEndMarker(const Word* begin) {
    beginMarker = begin;
    endMarker = nullptr;
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

const Word *Word::next(const std::vector<const Word *> &sentence, unsigned long position, const std::stack<const Word*> &markerStack) const {
    Gram *g = gram.next(sentence, position, markerStack);
    return g ? g->getWord() : nullptr;
}