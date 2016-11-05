#include <json/json.h>
#include <fstream>
#include "Dictionary.hpp"
#include "Parser.hpp"
#include "utils/color.hpp"

Dictionary::Dictionary(unsigned long n) : n(n) {
    std::unique_ptr<Word> beginSentence{std::make_unique<Word>(0, "<s>", "")};
    std::unique_ptr<Word> endSentence{std::make_unique<Word>(1, "</s>", "")};
    std::unique_ptr<Word> beginQuote{std::make_unique<Word>(2, "<q>", "\"")};
    std::unique_ptr<Word> endQuote{std::make_unique<Word>(3, "</q>", "\"")};
    std::unique_ptr<Word> beginParens{std::make_unique<Word>(4, "<p>", "(")};
    std::unique_ptr<Word> endParens{std::make_unique<Word>(5, "</p>", ")")};

    this->beginSentence = beginSentence.get();
    this->endSentence = endSentence.get();

    beginSentence->setAsBeginMarker(endSentence.get());
    endSentence->setAsEndMarker(beginSentence.get());
    beginQuote->setAsBeginMarker(endQuote.get());
    endQuote->setAsEndMarker(beginQuote.get());
    beginParens->setAsBeginMarker(endParens.get());
    endParens->setAsEndMarker(beginParens.get());
    wordMap[beginSentence->getInputText()] = std::move(beginSentence);
    wordMap[endSentence->getInputText()] = std::move(endSentence);
    wordMap[beginQuote->getInputText()] = std::move(beginQuote);
    wordMap[endQuote->getInputText()] = std::move(endQuote);
    wordMap[beginParens->getInputText()] = std::move(beginParens);
    wordMap[endParens->getInputText()] = std::move(endParens);
}


void Dictionary::setDebug() {
    setDebug(!debug_);
}

void Dictionary::setDebug(bool debug) {
    std::cout << "Debug: " << (debug ? "on" : "off") << std::endl;
    debug_ = debug;
}

void Dictionary::ingestFile(const std::string &filePath) {
    std::cout << "Loading text from " << filePath << " ..." << std::endl;

    std::string text;

    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            std::cerr << "File not found!" << std::endl;
            return;
        } else {
            while (file) {
                std::string line;
                std::getline(file, line);
                text += line + "\n";
            }
        }
    }

    if (text.empty()) {
        std::cerr << "Nothing to parse." << std::endl;
        exit(1);
    }

    std::cout << "Parsing/Ingesting..." << std::endl;
    Parser::parse(
        text,
        [this](std::vector<std::string> &words) {
            ingest(words);
        },
        [this]() {
            std::cout << "Updating probabilities..." << std::endl;
            updateProbabilities();
            std::cout << "Done!" << std::endl << std::endl;
        },
        debug_
    );
}

void Dictionary::open(const std::string &path) {
    Json::Value root;
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            std::cerr << "File not found!" << std::endl;
            return;
        } else {
            std::cout << "Loading dictionary: " << path << std::endl;
            try {
                file >> root;
            } catch (...) {
                std::cerr << "Invalid dictionary file!" << std::endl;
                return;
            }
        }
    }

    std::cout << "Learning dictionary..." << std::endl;

    try {
        const Json::Value n_json = root["n"];
        if (n_json != Json::nullValue && n_json.isIntegral()) {
            n = n_json.asUInt64();
        }

        const Json::Value words_json = root["words"];
        if (words_json != Json::nullValue && words_json.isArray()) {
            std::map<unsigned long, std::unique_ptr<Word>> wordsById{};

            // First pass add non-grammed words by id
            unsigned long numWords = words_json.size();
            std::cout << "    Adding " << numWords << " words..." << std::endl;
            for (int i = 0; i < numWords; ++i) {
                std::unique_ptr<Word> w = Word::fromJson(words_json[i]);
                auto search = wordsById.find(w->getId());
                if (search != wordsById.end()) {
                    std::cerr << "Duplicate word id: " << w->getId() << std::endl;
                    return;
                }
                wordsById[w->getId()] = std::move(w);
            }

            // Second pass, add grams
            std::cout << "    Adding grams..." << std::endl;
            for (int i = 0; i < words_json.size(); ++i) {
                const Json::Value word_json = words_json[i];
                wordsById[word_json["id"].asUInt64()]->fromJson(word_json, wordsById);
            }

            wordMap.clear();
            std::cout << "    Moving words..." << std::endl;
            for (auto &word:wordsById) {
                wordMap[word.second->getInputText()] = std::move(word.second);
            }
        }
    } catch (...) {
        std::cerr << "Error parsing dictionary! " << std::endl;
        return;
    }

    idCounter = wordMap.size() - 1;

    beginSentence = wordMap["<s>"].get();
    endSentence = wordMap["</s>"].get();
    beginSentence->setAsBeginMarker(endSentence);
    endSentence->setAsEndMarker(beginSentence);
    wordMap["<q>"]->setAsBeginMarker(wordMap["</q>"].get());
    wordMap["</q>"]->setAsEndMarker(wordMap["<q>"].get());
    wordMap["<p>"]->setAsBeginMarker(wordMap["</p>"].get());
    wordMap["</p>"]->setAsEndMarker(wordMap["<p>"].get());

    std::cout << "    Calculating probabilities..." << std::endl;
    updateProbabilities();

    std::cout << "Done!" << std::endl;
}

void Dictionary::save(const std::string &path) const {
    std::cout << "Saving dictionary to: " << path << std::endl;

    Json::Value root;

    root["n"] = static_cast<Json::UInt64>(n);
    root["words"] = Json::Value(Json::arrayValue);
    for (auto &word:wordMap) {
        root["words"].append(word.second->toJson());
    }

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "\t"; // Keep the file as small as possible
    builder["enableYAMLCompatibility"] = true;

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ofstream output(path);
    writer->write(root, &output);

    std::cout << "Saved!" << path << std::endl;
}

void Dictionary::input(const std::string &text) {
    std::vector<std::string> words = Parser::parseChunk(text, debug_);
    ingest(words);
    updateProbabilities();
}

void Dictionary::ingest(std::vector<std::string> &words, bool doUpdateProbabilities) {
    // Stack of markers to complete before ending the sentence
    std::stack<Word *> markerStack{};
    std::vector<Word *> sentenceWords;

    std::string lastWord;
    for (auto &wordString:words) {
        if (markerStack.size() == 0) {
            markerStack.push(beginSentence);
            sentenceWords.push_back(beginSentence);
        }

        Word *word;
        auto search = wordMap.find(wordString);
        if (search == wordMap.end()) {
            unsigned long id = ++idCounter;
            std::unique_ptr<Word> w = std::make_unique<Word>(id, wordString);
            word = w.get();
            wordMap[wordString] = std::move(w);
        } else {
            word = search->second.get();
        }

        sentenceWords.push_back(word);

        if (markerStack.size() == 1 && (wordString == "." || wordString == "!" || wordString == "?")) {
            // A sentence was finished, analyse it
            sentenceWords.push_back(endSentence);

            ingestSentence(sentenceWords, doUpdateProbabilities);

            sentenceWords.clear();
            std::stack<Word *>().swap(markerStack);

        } else if (word->isBeginMarker()) {
            markerStack.push(word);
        } else if (word->getId() == markerStack.top()->getEndMarker()->getId()) {
            markerStack.pop();
        }

        lastWord = word->getInputText();
    }

    if (markerStack.size() > 0 && sentenceWords.size() > 0) {
        while (markerStack.size() > 0) {
            sentenceWords.push_back(const_cast<Word *>(markerStack.top()->getEndMarker()));
            markerStack.pop();
        }
        ingestSentence(sentenceWords, doUpdateProbabilities);

    } else if (markerStack.size() > 0) {
        std::cerr << Color::FG_RED << "Marker stack left non-empty but sentence has zero length"
                  << Color::FG_DEFAULT
                  << std::endl;
    } else if (sentenceWords.size() > 0) {
        std::cerr << Color::FG_RED << "Sentence left non-empty but marker stack has zero length"
                  << Color::FG_DEFAULT
                  << std::endl;
    }
}

void Dictionary::ingestSentence(std::vector<Word *> &sentenceWords, bool doUpdateProbabilities) {
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
    unsigned long count = 0;
    for (auto &word:wordMap) {
        count += word.second->getGram()->getCount();
    }
    for (auto &word:wordMap) {
        word.second->updateProbabilities(count);
    }
}

std::vector<const std::string> Dictionary::nextCandidateWords(std::string seed) const {
    std::vector<const std::string> results;
    std::vector<const Word *> sentence{beginSentence};
    std::vector<std::string> seedStrings = Parser::parseChunk(seed);
    for (auto s:seedStrings) {
        auto search = wordMap.find(s);
        if (search == wordMap.end()) {
            return results;
        }
        sentence.push_back(search->second.get());
    }
    unsigned long start = sentence.size() > n - 1 ? sentence.size() - (n - 1) : 0;
    for (unsigned long i = start; i < sentence.size(); ++i) {
        std::vector<const Word *> candidates = sentence[i]->candidates(sentence, i + 1);
        for (auto c:candidates) {
            results.push_back(c->getOutputText());
        }
    }
    return results;
}

std::string Dictionary::nextMostProbableWord(std::string seed) const {
    std::vector<const Word *> sentence{beginSentence};
    std::vector<std::string> seedStrings = Parser::parseChunk(seed);
    for (auto s:seedStrings) {
        auto search = wordMap.find(s);
        if (search == wordMap.end()) {
            return "";
        }
        sentence.push_back(search->second.get());
    }
    const Word *newWord = nullptr;
    unsigned long start = sentence.size() > n - 1 ? sentence.size() - (n - 1) : 0;
    for (unsigned long i = start; i < sentence.size(); ++i) {
        const Word *w = sentence[i]->mostProbable(sentence, i + 1);
        if (w != nullptr) {
            newWord = w;
            break;
        }
    }
    return newWord ? newWord->getOutputText() : "";
}

std::string Dictionary::generate(std::string seed) const {
    // Stack of markers to complete before ending the sentence
    std::stack<const Word *> markerStack{};

    // Sentence, started by beginSentence marker
    std::vector<const Word *> sentence{beginSentence};

    // Pre seed
    if (!seed.empty()) {
        std::vector<std::string> seedStrings = Parser::parseChunk(seed);
        for (auto s:seedStrings) {
            auto search = wordMap.find(s);
            if (search != wordMap.end()) {
                sentence.push_back(search->second.get());
            }
        }

        if (debug_) {
            std::cout << "Raw seed: (" << seedStrings.size() << "): " << Color::FG_DARK_GRAY;
            for (auto w:seedStrings) {
                std::cout << w << " ";
            }
            std::cout << Color::FG_DEFAULT << std::endl;
            std::cout << "Sentence seed: (" << sentence.size() << "): " << Color::FG_DARK_GRAY;
            for (auto w:sentence) {
                std::cout << w->getInputText() << " ";
            }
            std::cout << Color::FG_DEFAULT << std::endl;
        }
    }

    // Add current sentence markers to the stack
    for (auto &word:sentence) {
        if (word->isBeginMarker()) {
            markerStack.push(word);
        }
    }

    const unsigned long seedSize = sentence.size();
    unsigned long retryPosition = seedSize;
    unsigned long backOffCount = 0;
    bool finishSentence = false;

    const Word *lastWord = nullptr;
    while (markerStack.size() > 0) {

        if (debug_) {
            std::cout << "Marker stack (size: " << markerStack.size() << "): " << markerStack.top()->getInputText()
                      << std::endl;
        }

        do {
            if (debug_) {
                std::cout << "  Searching new word for sentence: " << Color::FG_LIGHT_GRAY;
                for (auto w:sentence) {
                    std::cout << w->getInputText() << " ";
                }
                std::cout << Color::FG_DEFAULT << std::endl;
            }

            // Try to find n-gram first then (n-1)-gram etc... until we find something
            const Word *newWord = nullptr;
            unsigned long start = sentence.size() > n - 1 ? sentence.size() - (n - 1) : 0;
            for (unsigned long i = start; i < sentence.size(); ++i) {
                if (debug_) {
                    std::cout << Color::FG_LIGHT_GRAY << "    From: ";
                    for (unsigned long j = i; j < sentence.size(); ++j) {
                        std::cout << sentence[j]->getInputText() << " ";
                    }
                    std::cout << Color::FG_DEFAULT << std::endl;
                }

                const Word *w = sentence[i]->next(sentence, i + 1, markerStack, finishSentence, debug_);
                if (w != nullptr) {
                    newWord = w;
                    break;
                }
            }

            if (newWord) {
                if (debug_) {
                    std::cout << "  Found: " << Color::FG_GREEN << newWord->getInputText() << Color::FG_DEFAULT
                              << std::endl;
                }

                sentence.push_back(newWord);

                // If found word is a marker add it to the marker stack
                if (newWord->isBeginMarker()) {
                    markerStack.push(newWord);
                }

                // Push the retry cursor forward with the sentence
                if (sentence.size() > retryPosition) {
                    retryPosition = sentence.size();
                    backOffCount = 0;
                }

                // If we are past the max sentence size, finish as soon as possible
                if (sentence.size() > 10) {
                    finishSentence = true;
                }

            } else {
                if (debug_) {
                    std::cout << Color::FG_RED << "  Nothing found" << Color::FG_DEFAULT << std::endl;
                }

                ++backOffCount;
                long backOffPosition = static_cast<long>(retryPosition) - static_cast<long>(backOffCount);
                if (backOffPosition >= static_cast<long>(seedSize)) {
                    if (debug_) {
                        std::cout << Color::FG_BLUE << "  Backing off to position " << (retryPosition - backOffCount)
                                  << "/" << sentence.size() << Color::FG_DEFAULT << std::endl;
                    }

                    unsigned long removeCount = sentence.size() - backOffPosition;
                    for (unsigned long i = 0; i < removeCount; ++i) {
                        const Word *w = sentence.back();
                        sentence.pop_back();
                        // Remove from stack if it was a marker
                        if (w->getId() == markerStack.top()->getId()) {
                            markerStack.pop();

                            if (debug_) {
                                std::cout << Color::FG_BLUE << "  Removing " << w->getInputText() << " from stack"
                                          << Color::FG_DEFAULT << std::endl;
                            }
                        }
                    }

                    // Continue from here, we don't want to record the new sentence backoff point until we find somethinf
                    continue;

                } else {
                    if (debug_) {
                        std::cout << Color::FG_RED << "  Can't back off past seed, giving up!" << Color::FG_DEFAULT
                                  << std::endl;
                    }
                    break;
                }
            }

            lastWord = newWord;

        } while (lastWord == nullptr || lastWord->getId() != markerStack.top()->getEndMarker()->getId());

        if (debug_) {
            std::cout << "Popping marker stack: " << Color::FG_LIGHT_GRAY << markerStack.top()->getInputText()
                      << Color::FG_DEFAULT << std::endl;
        }

        markerStack.pop();

    } // end while markerStack > 0

    if (debug_) {
        std::cout << Color::FG_DARK_GRAY << "Raw sentence: ";
        for (auto w:sentence) {
            std::cout << w->getInputText() << " ";
        }
        std::cout << Color::FG_DEFAULT << std::endl;
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

std::string Dictionary::toString() const {
    std::stringstream ss;
    ss << beginSentence->toString();
    for (auto &word:wordMap) {
        ss << word.second->toString();
    }
    return ss.str();
}