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
    ss << std::string(depth * 4, ' ') << word->getText() << "(" << count << "-" << probability << ")" << std::endl;
    for (auto &gram:grams) {
        ss << gram.second->toString();
    }
    return ss.str();
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

Gram *Gram::next(const std::vector<const Word *> &sentence, unsigned long position) const {
    if ( position < sentence.size()  ) {
        auto search = grams.find(sentence[position]->getId());
        if (search != grams.end()) {
            if (position < sentence.size()) {
                return search->second->next(sentence, position + 1);
            } else {
                return search->second.get();
            }
        } else {
            return nullptr;
        }
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0, 1);
        double rnd = dis(gen);
        double probabilities = 0;
        for (auto &gram:grams) {
            double probability = gram.second->getProbability();
            if ( probabilities < rnd && rnd <= (probabilities + probability)) {
                return gram.second.get();
            }
            probabilities += probability;
        }
    }

    return nullptr;
}