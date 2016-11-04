#include <future>
#include <fstream>
#include <vector>
#include "utils/split.hpp"
#include "Parser.hpp"

std::vector<std::string> Parser::parse(const std::string text, bool debug) {
    //return Parser::parseChunk(text);
    std::string textBuffer = text;
    std::vector<std::future<std::vector<std::string>>> pool{};

    // Splitting text for parsing
    if (debug) {
        std::cout << "Splitting text into chunks..." << std::endl;
    }
    std::regex re("[\n]{2,}"); // Two or more line returns should not split sentences ;)
    std::sregex_token_iterator first{textBuffer.begin(), textBuffer.end(), re, -1};
    std::sregex_token_iterator last;
    std::vector<std::string> chunks{first, last};
    unsigned long numChunks = chunks.size();

    if (debug) {
        std::cout << "Threading " << numChunks << " chunk parsers..." << std::endl;
    }
    unsigned long processedChunks = 0;
    for (auto &chunk:chunks) {
        auto handle = std::async(std::launch::async, parseChunk, chunk, false);
        pool.push_back(std::move(handle));
    }

    if (debug) {
        std::cout << "Waiting on futures..." << std::endl;
    }
    std::vector<std::string> words{};
    while (processedChunks < numChunks) {
        for(auto &fut:pool) {
            if (fut.valid()) {
                auto w = fut.get();
                words.insert( words.end(), w.begin(), w.end() );
                ++processedChunks;
            }
        }
    }
    if (debug) {
        std::cout << "Done!" << std::endl;
    }

    return words;
}

void saveIntermediate(std::string name, std::string buff) {
    std::string file = "output-" + name + ".txt";
    std::ofstream output(file.c_str());
    output << buff << "\n";
}

std::vector<std::string> Parser::parseChunk(std::string textBuffer, bool debug) {

    // Remove crap
    if ( debug ) {
        std::cout << "Removing crap..." << std::endl;
    }
    textBuffer = std::regex_replace(textBuffer, std::regex("[\\x00-\\x1F_]"), " ");

    // Remove extra paragraphs
    //std::regex paragraph_re("[\r\n]");
    //textBuffer = std::regex_replace(textBuffer, paragraph_re, " ");

    // Make punctuation actual words
    if ( debug ) {
        std::cout << "Separating punctuation..." << std::endl;
    }
    textBuffer = std::regex_replace(textBuffer, std::regex("\\.\\.\\."), " … ");
    textBuffer = std::regex_replace(textBuffer, std::regex("([\\.\\,\\:\\;\\!\\?\\(\\)\"“”«»])"), " $1 ");

    // Remove double spaces
    if ( debug ) {
        std::cout << "Removing double spaces..." << std::endl;
    }
    std::regex doubleSpace_re("  ");
    while (textBuffer.find("  ") != std::string::npos) {
        textBuffer = std::regex_replace(textBuffer, doubleSpace_re, " ");
    }

    // Before starting to add markers, start by removing anything resembling one
    if ( debug ) {
        std::cout << "Removing marker-like structures..." << std::endl;
    }
    textBuffer = std::regex_replace(textBuffer, std::regex("<(.+?)>"), "($1)");
    textBuffer = std::regex_replace(textBuffer, std::regex("[<>]"), " ");

    // Extract quotes
    if ( debug ) {
        std::cout << "Extracting quotes..." << std::endl;
    }
    std::vector<std::pair<std::string,std::string>> quoteMarkerPairs{
        std::make_pair("\"", "\""),
        std::make_pair("“", "”"),
        std::make_pair("‘", "’"),
        std::make_pair("«", "»")
    };
    for ( auto marker : quoteMarkerPairs ) {
        textBuffer = std::regex_replace(textBuffer, std::regex(marker.first + "(.+?)" + marker.second), "<q> $1 </q>");
    }
    // Special case with spaces not to catch real apostrophes
    // We don't want it to be in the stray marker removal also
    //textBuffer = std::regex_replace(textBuffer, std::regex("(?:^| )'(.+?)'(?:$| )"), "<q> $1 </q>");
    textBuffer = std::regex_replace(textBuffer, std::regex("([^A-Za-z0-9])'(.+?)'([^A-Za-z0-9])"), "$1<q> $2 </q>$3");

    // Extract parens
    if ( debug ) {
        std::cout << "Extracting parens..." << std::endl;
    }
    std::vector<std::pair<std::string,std::string>> parensMarkerPairs{
            std::make_pair("\\(", "\\)"),
            std::make_pair("\\[", "\\]"),
            std::make_pair("\\{", "\\}")
    };
    for ( auto marker : parensMarkerPairs ) {
        textBuffer = std::regex_replace(textBuffer, std::regex(marker.first + "(.+?)" + marker.second), "<p> $1 </p>");
    }

    // Remove stray markers
    if ( debug ) {
        std::cout << "Removing stray markers..." << std::endl;
    }
    std::string strayMarkers = "";
    for ( auto marker : quoteMarkerPairs ) {
        strayMarkers += marker.first + marker.second;
    }
    for ( auto marker : parensMarkerPairs ) {
        strayMarkers += marker.first + marker.second;
    }
    textBuffer = std::regex_replace(textBuffer, std::regex("[" + strayMarkers + "]"), " ");

    // Extract sentences
    /*if ( debug ) {
        std::cout << "Extracting sentences..." << std::endl;
    }*/
    // Disable punctuation inside markers

    /*std::regex sentence_re("([\\.\\!\\?])( +(?:<\\/.+?>(?: |))*)");//"([\\.\\!\\?]) ((?!<\\/))");
    textBuffer = std::regex_replace(textBuffer, sentence_re, "$1$2\n");*/

    saveIntermediate("intermediate", textBuffer);

    // To lowercase
    if ( debug ) {
        std::cout << "Lowercasing..." << std::endl;
    }
    std::transform(textBuffer.begin(), textBuffer.end(), textBuffer.begin(), ::tolower);

    if ( debug ) {
        std::cout << "Vectorizing..." << std::endl;
    }
    std::vector<std::string> words{};
    std::stringstream ss;
    ss.str(textBuffer);
    std::string word;
    while (std::getline(ss, word, ' ')) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }

    return words;

    /*std::vector<std::string> sentences{};
    std::stringstream ss;
    ss.str(textBuffer);
    std::string sentence;
    while (std::getline(ss, sentence, '\n')) {
        sentence = std::regex_replace(sentence, std::regex("^ +| +$|( ) +"), "$1");
        if (!sentence.empty()) {
            sentences.push_back(sentence);
        }
    }

    if ( debug ) {
        std::cout << "Done!" << std::endl;
    }

    return sentences;*/
}