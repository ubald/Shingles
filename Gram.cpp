#include "Gram.hpp"
#include "Word.hpp"

Gram::Gram(const Word *word, unsigned int depth) : word(word), depth(depth) {

}

const Word *Gram::getWord() const {
    return word;
}

const unsigned long Gram::getCount() const {
    return count;
}

const double Gram::getProbability() const {
    return probability;
}

const std::string Gram::toString() const {
    std::stringstream ss;
    ss << std::string(depth * 4, ' ') << word->getId() << ":" << word->getOutputText() << " (" << count << "-"
       << probability << ")" << std::endl;
    for (auto &gram:grams) {
        ss << gram.second->toString();
    }
    return ss.str();
}

const Json::Value Gram::toJson() const {
    Json::Value gramJson;
    gramJson["word"] = static_cast<Json::UInt64>(word->getId());
    gramJson["count"] = static_cast<Json::UInt64>(count);
    gramJson["grams"] = Json::Value(Json::arrayValue);
    for (auto &gram:grams) {
        gramJson["grams"].append(gram.second->toJson());
    }
    return gramJson;
}

void Gram::fromJson(const Json::Value gram_json, const std::map<unsigned long, std::unique_ptr<Word>> &wordsById) {
    const Json::Value word_json = gram_json["word"];
    if (word_json == Json::nullValue) {
        std::cerr << "Missing gram word" << std::endl;
        throw;
    }
    unsigned long wordId = word_json.asUInt64();
    word = wordsById.at(wordId).get();

    const Json::Value count_json = gram_json["count"];
    if (count_json == Json::nullValue) {
        std::cerr << "Missing gram count" << std::endl;
        throw;
    }
    count = count_json.asUInt64();

    const Json::Value grams_json = gram_json["grams"];
    if (grams_json == Json::nullValue || grams_json.type() != Json::arrayValue) {
        std::cerr << "Missing gram grams" << std::endl;
        throw;
    }
    for (int i = 0; i < grams_json.size(); ++i) {
        std::unique_ptr<Gram> gram(new Gram()); // *new* because of private constructor
        gram->fromJson(grams_json[i], wordsById);
        grams[wordId] = std::move(gram);
    }
};

void Gram::update(const std::vector<Word *> &sentence, unsigned long position, unsigned long n) {
    // We've been seen one more time
    ++count;

    // Go deeper more than one "gram" remaining and not yet at the end of the sentence
    position++;
    if (n > 1 && position < sentence.size()) {
        Gram *gram_ptr;
        Word *word = sentence[position];
        auto search = grams.find(word->getId());
        if (search == grams.end()) {
            std::unique_ptr<Gram> gram = std::make_unique<Gram>(word, depth + 1);
            gram_ptr = gram.get();
            grams[word->getId()] = std::move(gram);
        } else {
            gram_ptr = search->second.get();
        }
        gram_ptr->update(sentence, position, n - 1);
    }


}

void Gram::computeProbability(unsigned long total) {
    probability = (double) count / (double) total;

    unsigned long count = 0;
    for (auto &gram:grams) {
        count += gram.second->count;
    }
    for (auto &gram:grams) {
        gram.second->computeProbability(count);
    }
}

Gram *Gram::next(const std::vector<const Word *> &sentence, unsigned long position,
                 const std::stack<const Word *> &markerStack, bool debug) const {

    Gram *nextGram = nullptr;

    if (position < sentence.size()) {
        if ( debug ) {
            std::cout << "    Gram";
        }


        auto search = grams.find(sentence[position]->getId());
        if (search != grams.end()) {
            nextGram = search->second->next(sentence, position + 1, markerStack);
        }
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0, 1);
        double rnd = dis(gen);
        double probabilities = 0;
        for (auto &gram:grams) {
            double probability = gram.second->getProbability();
            if (probabilities < rnd && rnd <= (probabilities + probability)) {
                nextGram = gram.second.get();
                break;
            }
            probabilities += probability;
        }
    }

    if (nextGram != nullptr && nextGram->word->isMarker()) {
        if (nextGram->word->isBeginMarker() && nextGram->word->getId() == markerStack.top()->getId()) {
            nextGram = nullptr;

        } else if (nextGram->word->isEndMarker() &&
                   nextGram->word->getBeginMarker()->getId() != markerStack.top()->getId()) {
            nextGram = nullptr;
        }
    }

    return nextGram;
}