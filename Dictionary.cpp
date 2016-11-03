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

    std::cout << "Parsing sentences..." << std::endl;
    std::vector<std::string> sentences = Parser::parse(text);

    {
        std::ofstream output("output.txt");
        for (std::string s:sentences) {
            output << s << "\n";
        }
    }

    std::cout << "Ingesting sentences..." << std::endl;
    for (auto &sentence: sentences) {
        ingest(sentence);
    }

    std::cout << "Updating probabilities..." << std::endl;
    updateProbabilities();

    std::cout << "Done!" << std::endl << std::endl;
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

    try {
        const Json::Value n_json = root["n"];
        if (n_json != Json::nullValue && n_json.isIntegral()) {
            n = n_json.asUInt64();
        }

        const Json::Value words_json = root["words"];
        if (words_json != Json::nullValue && words_json.isArray()) {
            std::map<unsigned long, std::unique_ptr<Word>> wordsById{};

            // First pass but non-grammed words by id
            for (int i = 0; i < words_json.size(); ++i) {
                std::unique_ptr<Word> w = Word::fromJson(words_json[i]);
                auto search = wordsById.find(w->getId());
                if (search != wordsById.end()) {
                    std::cerr << "Duplicate word id: " << w->getId() << std::endl;
                    return;
                }
                wordsById[w->getId()] = std::move(w);
            }

            // Second pass, add grams
            for (int i = 0; i < words_json.size(); ++i) {
                const Json::Value word_json = words_json[i];
                wordsById[word_json["id"].asUInt64()]->fromJson(word_json, wordsById);
            }

            wordMap.clear();
            for (auto &word:wordsById) {
                wordMap[word.second->getInputText()] = std::move(word.second);
            }
        }
    } catch (...) {
        std::cerr << "Error parsing dictionary! " << std::endl;
        return;
    }

    beginSentence = wordMap["<s>"].get();
    endSentence = wordMap["</s>"].get();
    beginSentence->setAsBeginMarker(endSentence);
    endSentence->setAsEndMarker(beginSentence);
    wordMap["<q>"]->setAsBeginMarker(wordMap["</q>"].get());
    wordMap["</q>"]->setAsEndMarker(wordMap["<q>"].get());
    wordMap["<p>"]->setAsBeginMarker(wordMap["</p>"].get());
    wordMap["</p>"]->setAsEndMarker(wordMap["<p>"].get());

    updateProbabilities();

    std::cout << "Done!" << std::endl;
}

void Dictionary::save(const std::string &path) const {
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

    std::cout << "Saved dictionary to: " << path << std::endl;
}

void Dictionary::input(const std::string &text) {
    std::vector<std::string> sentences = Parser::parse(text);
    for (auto sentence:sentences) {
        ingest(sentence);
    }
    updateProbabilities();
}

void Dictionary::ingest(const std::string &sentence, bool doUpdateProbabilities) {
    std::vector<std::string> sentenceWordStrings = split(sentence, ' ');

    // Words
    std::vector<Word *> sentenceWords;
    sentenceWords.push_back(beginSentence);
    for (auto sentenceWordString:sentenceWordStrings) {
        auto search = wordMap.find(sentenceWordString);
        if (search == wordMap.end()) {
            unsigned long id = ++idCounter;
            std::unique_ptr<Word> word = std::make_unique<Word>(id, sentenceWordString);
            sentenceWords.push_back(word.get());
            wordMap[sentenceWordString] = std::move(word);
            //learnedWords.push_back(std::move(word));
        } else {
            sentenceWords.push_back(search->second.get());
        }
    }
    sentenceWords.push_back(endSentence);

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

std::string Dictionary::toString() {
    std::stringstream ss;
    ss << beginSentence->toString();
    for (auto &word:wordMap) {
        ss << word.second->toString();
    }
    return ss.str();
}

std::string Dictionary::generate(std::string seed) {
    // Stack of markers to complete before ending the sentence
    std::stack<const Word *> markerStack{};

    // Sentence, started by beginSentence marker
    std::vector<const Word *> sentence{beginSentence};

    // Pre seed
    if (!seed.empty()) {
        std::vector<std::string> seedStrings = split(seed, ' ');
        for (auto s:seedStrings) {
            auto search = wordMap.find(s);
            if (search != wordMap.end()) {
                sentence.push_back(search->second.get());
            }
        }

        if (debug_) {
            std::cout << "Seed: (" << sentence.size() << ")";
            for (auto w:sentence) {
                std::cout << w->getInputText() << " ";
            }
            std::cout << std::endl;
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

        if (debug_) {
            std::cout << "Marker stack (size: " << markerStack.size() << "): " << markerStack.top()->getInputText()
                      << std::endl;
        }

        int nullCount = 0;
        do {
            if (debug_) {
                std::cout << "  Searching new word for sentence " << Color::FG_LIGHT_GRAY;
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

                const Word *w = sentence[i]->next(sentence, i + 1, markerStack, debug_);
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
            } else {
                ++nullCount;

                if (debug_) {
                    std::cout << Color::FG_RED << "  Nothing found (count: " << nullCount << ")" << Color::FG_DEFAULT
                              << std::endl;
                }
            }

            //TODO: Separate filling a sentence/marker and finding a new word
            //TODO: Back off one word if we can't seem to find
            if (nullCount > 100) {
                if (debug_) {
                    std::cout << Color::FG_RED << "  Giving up!" << Color::FG_DEFAULT << std::endl;
                }
                break;
            }

            lastWord = newWord;

        } while (lastWord == nullptr || lastWord->getId() != markerStack.top()->getEndMarker()->getId());

        markerStack.pop();
    }

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