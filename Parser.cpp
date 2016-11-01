#include "Parser.hpp"

std::vector<std::string> Parser::parse(const std::string text) {
    std::string textBuffer = text;

    // Remove extra paragraphs
    std::regex paragraph_re("[\r\n]");
    textBuffer = std::regex_replace(textBuffer, paragraph_re, " ");

    // Make punctuation actual words
    textBuffer = std::regex_replace(textBuffer, std::regex("\\.\\.\\."), " … ");
    textBuffer = std::regex_replace(textBuffer, std::regex("([\\.\\,\\:\\;\\!\\?\\(\\)\"\'“”«»])"), " $1 ");

    // Remove double spaces
    std::regex doubleSpace_re("  ");
    while (textBuffer.find("  ") != std::string::npos) {
        textBuffer = std::regex_replace(textBuffer, doubleSpace_re, " ");
    }

    // Extract sentences
    std::regex sentence_re("([\\.\\!\\?]) ([^\'\"])");
    textBuffer = std::regex_replace(textBuffer, sentence_re, "$1\n$2");

    // Extract quotes
    std::vector<std::pair<std::string,std::string>> quoteMarkerPairs{
        std::make_pair("\"", "\""),
        std::make_pair("“", "”"),
        std::make_pair("«", "»")
    };
    for ( auto marker : quoteMarkerPairs ) {
        textBuffer = std::regex_replace(textBuffer, std::regex((marker.first + "(.+?)" + marker.second)), "<q> $1 </q>");
    }

    // Extract parens
    std::vector<std::pair<std::string,std::string>> parensMarkerPairs{
            std::make_pair("\\(", "\\)"),
            std::make_pair("\\[", "\\]"),
            std::make_pair("\\{", "\\}")
    };
    for ( auto marker : parensMarkerPairs ) {
        textBuffer = std::regex_replace(textBuffer, std::regex((marker.first + "(.+?)" + marker.second)), "<p> $1 </p>");
    }

    // To lowercase
    std::transform(textBuffer.begin(), textBuffer.end(), textBuffer.begin(), ::tolower);

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