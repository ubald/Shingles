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
    ss << std::string(depth * 4, ' ') << word->getId() << ":" << word->getOutputText() << " (" << count << "-" << probability << ")" << std::endl;
    for (auto &gram:grams) {
        ss << gram.second->toString();
    }
    return ss.str();
}

const Json::Value Gram::toJson() const {
    Json::Value gramJson;
    gramJson["word"] = static_cast<Json::UInt64>(word->getId());
    gramJson["count"] = static_cast<Json::UInt64>(count);
    //gramJson["probability"] = probability;
    gramJson["grams"] = Json::Value(Json::arrayValue);
    for (auto &gram:grams) {
        gramJson["grams"].append(gram.second->toJson()) ;
    }
    return gramJson;
}

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
                 const std::stack<const Word *> &markerStack) const {

    Gram *nextGram = nullptr;

    if (position < sentence.size()) {
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

        } else if (nextGram->word->isEndMarker() && nextGram->word->getBeginMarker()->getId() != markerStack.top()->getId() ) {
            nextGram = nullptr;
        }
    }

    return nextGram;
}