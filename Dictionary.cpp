#include "Dictionary.hpp"

Dictionary::Dictionary(unsigned long n) : n(n) {

}

void Dictionary::ingest(const std::string &sentence, bool doUpdateProbabilities) {
    //std::cout << ">>>" << sentence << "<<<" << std::endl;

    std::vector<std::string> sentenceWordStrings = split(sentence, ' ');

    // Words
    std::vector<Word *> sentenceWords;
    sentenceWords.push_back(beginWord.get());
    for (auto sentenceWordString:sentenceWordStrings) {
        auto search = words.find(sentenceWordString);
        if (search == words.end()) {
            unsigned long id = idCounter++;
            std::unique_ptr<Word> word = std::make_unique<Word>(id, sentenceWordString);
            sentenceWords.push_back(word.get());
            words[sentenceWordString] = std::move(word);
        } else {
            sentenceWords.push_back(search->second.get());
        }
    }
    sentenceWords.push_back(endWord.get());

    // Forget possible empty sentences
    if (sentenceWords.size() > 2) {
        // Do not loop to endWord, not necessary
        for (unsigned long i = 0; i < sentenceWords.size(); ++i) {
            sentenceWords[i]->updateGraph(sentenceWords, i, n);
        }
    }

    // Update probabilities
    if ( doUpdateProbabilities ) {
        updateProbabilities();
    }
}

void Dictionary::updateProbabilities() {
    unsigned long count = beginWord->getGram()->getCount();
    for (auto &word:words) {
        count += word.second->getGram()->getCount();
    }
    beginWord->updateProbabilities(count);
    for (auto &word:words) {
        word.second->updateProbabilities(count);
    }
}

std::string Dictionary::toString() {
    std::stringstream ss;
    ss << beginWord->toString();
    for (auto &word:words) {
        ss << word.second->toString();
    }
    return ss.str();
}

std::string Dictionary::generate(std::string seed) {
    std::vector<const Word *> sentence{beginWord.get()};

    if (!seed.empty()) {
        std::vector<std::string> seedStrings = split(seed, ' ');
        for(auto s:seedStrings) {
            auto search = words.find(s);
            if (search != words.end()) {
                sentence.push_back(search->second.get());
            }
        }
    }

    const Word *word = nullptr;
    do {
        // Try to find n-gram first then (n-1)-gram etc... until we find something
        const Word *newWord = nullptr;
        unsigned long start = sentence.size() > n ? sentence.size() - n : 0;
        for(unsigned long i = start; i < sentence.size(); ++i) {
            const Word* w = sentence[i]->next(sentence, i + 1);
            if ( w != nullptr ) {
                newWord = w;
                break;
            }
        }

        if ( newWord ) {
            sentence.push_back(newWord);
        }
        word = newWord;
    } while ( word != nullptr && word->getId() != endWord->getId());

    std::string sentenceText = "";
    for (auto w:sentence) {
        sentenceText += w->getText() + " ";
    }

    // Make punctuation back into actual punctuation
    sentenceText = std::regex_replace(sentenceText, std::regex(" ([.,:;!?])"), "$1");

    return sentenceText;
}