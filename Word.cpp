#include <iostream>
#include <utility>
#include "Word.hpp"

Word::Word(unsigned long id, std::string text) :
    id(id), inputText(text), outputText(text), gram(this) {

}

Word::Word(unsigned long id, std::string inputText, std::string outputText) :
    id(id),
    inputText(std::move(inputText)),
    outputText(std::move(outputText)),
    gram(this) {

}

unsigned long Word::getId() const {
    return id;
}

bool Word::isMarker() const {
    return beginMarker != nullptr || endMarker != nullptr;
}

bool Word::isBeginMarker() const {
    return endMarker != nullptr;
}

bool Word::isEndMarker() const {
    return beginMarker != nullptr;
}

const Word *Word::getBeginMarker() const {
    return beginMarker;
}

void Word::setAsBeginMarker(const Word *end) {
    endMarker   = end;
    beginMarker = nullptr;
}

const Word *Word::getEndMarker() const {
    return endMarker;
}

void Word::setAsEndMarker(const Word *begin) {
    beginMarker = begin;
    endMarker   = nullptr;
}

const std::string Word::getInputText() const {
    return inputText;
}

const std::string Word::getOutputText() const {
    return outputText;
}

const Gram *Word::getGram() const {
    return &gram;
}

const std::string Word::toString() const {
    std::stringstream ss;
    ss << gram.toString();
    return ss.str();
}

const Json::Value Word::toJson() const {
    Json::Value wordJson;
    wordJson["id"]     = static_cast<Json::UInt64>(id);
    wordJson["input"]  = inputText;
    wordJson["output"] = outputText;
    wordJson["gram"]   = gram.toJson();
    return wordJson;
}

void Word::fromJson(const Json::Value &word_json, std::unordered_map<unsigned long, std::unique_ptr<Word>> &wordsById) {
    const Json::Value gram_json = word_json["gram"];
    if (gram_json.empty()) {
        std::cerr << "Missing word gram" << std::endl;
        throw;
    }
    gram.fromJson(gram_json, wordsById);
};

/**
 * @static
 * @return
 */
std::unique_ptr<Word> Word::fromJson(const Json::Value &word_json) {
    const Json::Value id_json = word_json["id"];
    if (id_json.empty()) {
        std::cerr << "Missing word id" << std::endl;
        throw;
    }
    const Json::Value inputText_json = word_json["input"];
    if (inputText_json.empty()) {
        std::cerr << "Missing word inputText" << std::endl;
        throw;
    }
    const Json::Value outputText_json = word_json["output"];
    if (outputText_json.empty()) {
        std::cerr << "Missing word outputText" << std::endl;
        throw;
    }

    return std::make_unique<Word>(id_json.asUInt64(), inputText_json.asString(), outputText_json.asString());
};

void Word::updateGraph(const std::vector<Word *> &sentence, unsigned long position, unsigned long n) {
    gram.update(sentence, position, n);
}

void Word::updateProbabilities(unsigned long wordCount) {
    gram.computeProbability(wordCount);
}

std::vector<const Word *> Word::candidates(const std::vector<const Word *> &sentence, unsigned long position) const {
    std::vector<const Gram *> gramCandidates = gram.candidates(sentence, position);
    std::vector<const Word *> wordCandidates{gramCandidates.size()};

    for (const auto &g:gramCandidates) {
        wordCandidates.push_back(g->getWord());
    }

    return wordCandidates;
}

const Word *Word::mostProbable(const std::vector<const Word *> &sentence, unsigned long position) const {
    const Gram *g = gram.mostProbable(sentence, position);
    return g ? g->getWord() : nullptr;
}

const Word *Word::nextWord(
    const std::vector<const Word *> &sentence, unsigned long position,
    const std::stack<const Word *> &markerStack, const std::vector<const Word *> topic,
    bool finishSentence, bool debug
) const {
    const Gram *g = gram.next(sentence, position, markerStack, topic, finishSentence, debug);
    return g ? g->getWord() : nullptr;
}

const Gram *Word::nextGram(
    const std::vector<const Word *> &sentence, unsigned long position,
    const std::stack<const Word *> &markerStack, const std::vector<const Word *> topic,
    bool finishSentence, bool debug
) const {
    return gram.next(sentence, position, markerStack, topic, finishSentence, debug);
}
