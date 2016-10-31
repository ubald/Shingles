#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <regex>
#include <sstream>
#include "Dictionary.hpp"

std::string loadFile(std::string path) {
    std::string text;

    {
        std::ifstream file(path);
        if (!file) {
            std::cerr << "File not found!" << std::endl;
        } else {
            while (file) {
                std::string line;
                std::getline(file, line);
                text += line + "\n";
            }
        }
    }

    return text;
}

std::vector<std::string> parseText(std::string text) {
    std::string textBuffer = text;

    // Remove extra paragraphs
    std::regex paragraph_re("[\r\n]");
    textBuffer = std::regex_replace(textBuffer, paragraph_re, " ");

    // Remove double spaces
    std::regex doubleSpace_re("  ");
    while (textBuffer.find("  ") != std::string::npos) {
        textBuffer = std::regex_replace(textBuffer, doubleSpace_re, " ");
    }

    // Sentences
    std::string sentence_re_string = "([\\.\\!\\?\\n][\'\"]?) *";

    // Extract sentences
    std::regex sentence_re(sentence_re_string);
    textBuffer = std::regex_replace(textBuffer, sentence_re, "$1\n");

    // Extract sub-sentences
    std::vector<std::pair<std::string,std::string>> subSentenceMarkerPairs{
            std::make_pair("\\(", "\\)"),
            std::make_pair("\"", "\""),
            std::make_pair("“", "”"),
            std::make_pair("«", "»"),
            std::make_pair("--", "--")
    };
    for ( auto marker : subSentenceMarkerPairs ) {
        std::regex subSentence_re(marker.first + "(.+?)" + marker.second);
        textBuffer = std::regex_replace(textBuffer, subSentence_re, "\n$1\n");
    }

    std::vector<std::string> sentences;
    std::stringstream ss;
    ss.str(textBuffer);
    std::string sentence;
    while (std::getline(ss, sentence, '\n')) {
        sentence = std::regex_replace(sentence, std::regex("^ +| +$|( ) +"), "$1");
        if (!sentence.empty()) {
            sentences.push_back(sentence);
        }
    }

    return sentences;
}

int main() {

    std::cout << "Loading text..." << std::endl;
    const std::string &text = loadFile("books/test.txt");
    if (text.empty()) {
        std::cerr << "Nothing to parse." << std::endl;
        exit(1);
    }

    std::cout << "Parsing sentences..." << std::endl;
    std::vector<std::string> sentences = parseText(text);

    std::cout << "Ingesting sentences..." << std::endl;
    std::unique_ptr<Dictionary> dictionary = std::make_unique<Dictionary>(4);
    for (auto sentence: sentences) {
        dictionary->ingest(sentence);
    }

    std::cout << "Updating probabilities..." << std::endl;
    dictionary->updateProbabilities();

    //std::cout << dictionary->toString() << std::endl;

    std::cout << "Done!" << std::endl << std::endl;
    std::cout << "Press any key to generate a sentence..." << std::endl;

    std::string input;
    while (true) {
        std::getline(std::cin, input);
        std::cout << dictionary->generate() << std::endl;
    }
    return 0;
}

