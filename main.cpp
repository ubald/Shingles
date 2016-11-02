#include <fstream>
#include "Parser.hpp"
#include "Dictionary.hpp"

std::string loadFile(std::string path) {
    std::string text;

    {
        std::ifstream file(path, std::ios::binary);
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

int main() {

    std::cout << "Loading text..." << std::endl;
    const std::string &text = loadFile("input.txt");
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
    std::unique_ptr<Dictionary> dictionary = std::make_unique<Dictionary>(3);
    for (auto sentence: sentences) {
        dictionary->ingest(sentence);
    }

    std::cout << "Updating probabilities..." << std::endl;
    dictionary->updateProbabilities();

    std::cout << "Saving dictionary..." << std::endl;
    dictionary->save("dictionary.json");
    //std::cout << dictionary->toString() << std::endl;

    std::cout << "Done!" << std::endl << std::endl;

    //for (int i = 0; i < 10; ++i) {
    //    std::cout << dictionary->generate() << std::endl;
    //}

    //return 0;

    std::cout << "Press any key to generate a sentence..." << std::endl;

    std::string input;
    while (true) {
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        std::cout << dictionary->generate(input) << std::endl;
    }
    return 0;
}

