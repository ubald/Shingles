#ifndef SHINGLES_GRAM_HPP
#define SHINGLES_GRAM_HPP

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <random>
#include <memory>
#include <stack>
#include <json/json.h>

class Word;

class Gram {
public:
    Gram(const Word *word, unsigned int depth = 0);
    void update(const std::vector<Word *> &sentence, unsigned long position, unsigned long n);
    void computeProbability(unsigned long total);
    using candidates_t = std::vector<std::map<unsigned long, std::pair<unsigned long, const Word *>>>;
    std::vector<const Gram *> candidates(const std::vector<const Word *> &sentence, unsigned long position) const;
    const Gram *mostProbable(const std::vector<const Word *> &sentence, unsigned long position) const;
    const Gram *next(
        const std::vector<const Word *> &sentence,
        unsigned long position,
        const std::stack<const Word *> &markerStack,
        std::vector<const Word *> topic,
        bool finishSentence = false,
        bool debug = false
    ) const;
    const Word *getWord() const;
    const unsigned long getCount() const;
    const double getProbability() const;
    const std::string toString() const;
    const Json::Value toJson() const;
    void fromJson(const Json::Value &gram_json, std::unordered_map<unsigned long, std::unique_ptr<Word>> &wordsById);

private:
    Gram() = default;

    const Word                                     *word;
    unsigned long                                  count{0};
    double                                         probability{0};
    unsigned int                                   depth{0};
    //std::map<unsigned long, std::pair<unsigned long, const Word *>> words{};
    std::map<unsigned long, std::unique_ptr<Gram>> grams{};
};


#endif //SHINGLES_GRAPH_HPP
