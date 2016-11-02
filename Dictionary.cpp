#include <json/json.h>
#include <fstream>
#include "Dictionary.hpp"

Dictionary::Dictionary(unsigned long n) : n(n) {
    beginSentence->setAsBeginMarker(endSentence.get());
    endSentence->setAsEndMarker(beginSentence.get());
    beginQuote->setAsBeginMarker(endQuote.get());
    endQuote->setAsEndMarker(beginQuote.get());
    beginParens->setAsBeginMarker(endParens.get());
    endParens->setAsEndMarker(beginParens.get());
}

void Dictionary::save(const std::string &path) const {
    Json::Value root;
    //for (auto &word:learnedWords) {
    //    root["words"][word->getOutputText()] = static_cast<Json::UInt64>(word->getId());//.append(jsonWord);
    //}

    root["words"] = Json::Value(Json::arrayValue);
    for(auto &word:wordMap) {
        root["words"].append( word.second->toJson() );
    }

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "\t"; // Keep the file as small as possible
    builder["enableYAMLCompatibility"] = true;

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ofstream output(path);
    writer->write(root, &output);
}

void Dictionary::ingest(const std::string &sentence, bool doUpdateProbabilities) {
    //std::cout << ">>>" << sentence << "<<<" << std::endl;

    std::vector<std::string> sentenceWordStrings = split(sentence, ' ');

    // Words
    std::vector<Word *> sentenceWords;
    sentenceWords.push_back(beginSentence.get());
    for (auto sentenceWordString:sentenceWordStrings) {
        auto search = wordMap.find(sentenceWordString);
        if (search == wordMap.end()) {
            unsigned long id = idCounter++;
            std::unique_ptr<Word> word = std::make_unique<Word>(id, sentenceWordString);
            sentenceWords.push_back(word.get());
            wordMap[sentenceWordString] = word.get();
            learnedWords.push_back(std::move(word));
        } else {
            sentenceWords.push_back(search->second);
        }
    }
    sentenceWords.push_back(endSentence.get());

    // Forget possible empty sentences
    if (sentenceWords.size() > 2) {
        // Do not loop to endSentence, not necessary
        for (unsigned long i = 0; i < sentenceWords.size(); ++i) {
            sentenceWords[i]->updateGraph(sentenceWords, i, n);
        }
    }

    // Update probabilities
    if (doUpdateProbabilities) {
        updateProbabilities();
    }
}

void Dictionary::updateProbabilities() {
    unsigned long count = 0;//beginSentence->getGram()->getCount();
    for (auto &word:learnedWords) {
        count += word->getGram()->getCount();
    }
    beginSentence->updateProbabilities(count);
    for (auto &word:learnedWords) {
        word->updateProbabilities(count);
    }
}

std::string Dictionary::toString() {
    std::stringstream ss;
    ss << beginSentence->toString();
    for (auto &word:learnedWords) {
        ss << word->toString();
    }
    return ss.str();
}

std::string Dictionary::generate(std::string seed) {
    // Stack of markers to complete before ending the sentence
    std::stack<const Word *> markerStack{};

    // Sentence, started by beginSentence marker
    std::vector<const Word *> sentence{beginSentence.get()};

    // Pre seed
    if (!seed.empty()) {
        std::vector<std::string> seedStrings = split(seed, ' ');
        for (auto s:seedStrings) {
            auto search = wordMap.find(s);
            if (search != wordMap.end()) {
                sentence.push_back(search->second);
            }
        }
    }

    // Add current sentence markers to the stack
    for (auto &word:sentence) {
        if (word->isBeginMarker()) {
            markerStack.push(word);
        }
    }

    const Word *lastWord = nullptr;
    while (markerStack.size() > 0) {
        int nullCount = 0;
        do {
            // Try to find n-gram first then (n-1)-gram etc... until we find something
            const Word *newWord = nullptr;
            unsigned long start = sentence.size() > n ? sentence.size() - n : 0;
            for (unsigned long i = start; i < sentence.size(); ++i) {
                const Word *w = sentence[i]->next(sentence, i + 1, markerStack);
                if (w != nullptr) {
                    newWord = w;
                    break;
                }
            }

            if (newWord) {
                sentence.push_back(newWord);
                // If found word is a marker add it to the marker stack
                if (newWord->isBeginMarker()) {
                    markerStack.push(newWord);
                }
            } else {
                ++nullCount;
            }

            //TODO: Separate filling a sentence/marker and finding a new word
            //TODO: Back off one word if we can't seem to find
            if (nullCount > 100) {
                std::cerr << "\nNo end in sight! Stack:" << markerStack.top()->getOutputText() << std::endl;
                break;
            }

            lastWord = newWord;

        } while (lastWord == nullptr || lastWord->getId() != markerStack.top()->getEndMarker()->getId());

        markerStack.pop();
    }

    std::string sentenceText("");
    for (auto w:sentence) {
        if (!w->getOutputText().empty()) {
            sentenceText += w->getOutputText() + " ";
        }
    }
    std::transform(sentenceText.begin(), sentenceText.begin() + 1, sentenceText.begin(), ::toupper);

    // Make punctuation back into actual punctuation
    sentenceText = std::regex_replace(sentenceText, std::regex(" ([.,:;!?])"), "$1");
    sentenceText = std::regex_replace(sentenceText, std::regex(" ([']) "), "$1");
    sentenceText = std::regex_replace(sentenceText, std::regex("\" (.+?) \""), "\"$1\"");
    sentenceText = std::regex_replace(sentenceText, std::regex("\\( (.+?) \\)"), "\\($1\\)");

    return sentenceText;
}