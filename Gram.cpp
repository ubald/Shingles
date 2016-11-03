#include "Gram.hpp"
#include "Word.hpp"
#include "utils/color.hpp"

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
        auto search = grams.find(sentence[position]->getId());
        if (search != grams.end()) {
            if ( debug ) {
                std::cout << std::string(depth+6,' ') << "Gram " << Color::FG_CYAN << word->getInputText() << Color::FG_DEFAULT << " forwarding to " << Color::FG_CYAN << search->second->word->getInputText() << Color::FG_DEFAULT << std::endl;
            }

            nextGram = search->second->next(sentence, position + 1, markerStack, debug);
        } else if ( debug ) {
            std::cout << std::string(depth+6,' ') << "Gram " << Color::FG_CYAN << word->getInputText() << Color::FG_DEFAULT << " has no follower for " << Color::FG_CYAN << sentence[position]->getInputText() << Color::FG_DEFAULT << std::endl;
        }
    } else {
        if ( debug ) {
            std::cout << std::string(depth+6,' ') << "Gram " << Color::FG_CYAN << word->getInputText() << Color::FG_DEFAULT << " attempting to find a word from:" << std::endl;
            for (auto &gram:grams) {
                std::cout << std::string(depth+7,' ') << Color::FG_YELLOW << std::to_string(gram.second->getProbability()) << Color::FG_DEFAULT << "\t"  << Color::FG_LIGHT_GRAY << gram.second->getWord()->getInputText()  << Color::FG_DEFAULT<< std::endl;
            }
        }

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

        if ( debug ) {
            if (nextGram != nullptr) {
                std::cout << std::string(depth + 6, ' ') << "Found " << Color::FG_CYAN << nextGram->word->getInputText() << Color::FG_DEFAULT << std::endl;
            } else {
                std::cout << std::string(depth + 6, ' ') << Color::FG_RED << "Nothing Found!" << Color::FG_DEFAULT << std::endl;
            }
        }
    }

    if (nextGram != nullptr && nextGram->word->isMarker()) {
        if (nextGram->word->isBeginMarker() && nextGram->word->getId() == markerStack.top()->getId()) {
            if ( debug ) {
                std::cout << std::string(depth + 6, ' ') << Color::FG_RED << "Can't open stacked marker: " << markerStack.top()->getInputText() << Color::FG_DEFAULT << std::endl;
            }
            nextGram = nullptr;

        } else if (nextGram->word->isEndMarker() &&
                   nextGram->word->getBeginMarker()->getId() != markerStack.top()->getId()) {
            if ( debug ) {
                std::cout << std::string(depth + 6, ' ') << Color::FG_RED << "Can't close unstacked marker: " << nextGram->word->getInputText() << Color::FG_DEFAULT << std::endl;
            }
            nextGram = nullptr;
        }
    }

    if ( debug ) {
        if (nextGram != nullptr) {
            std::cout << std::string(depth + 6, ' ') << "Returning " << Color::FG_CYAN << nextGram->word->getInputText() << Color::FG_DEFAULT << std::endl;
        } else {
            std::cout << std::string(depth + 6, ' ') << Color::FG_RED << "Nothing to return!" << Color::FG_DEFAULT << std::endl;
        }
    }

    //TODO: If more than one possible gram, retry until not null

    return nextGram;
}