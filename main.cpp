#include <fstream>
#include <vector>
#include <chrono>
#include "optionparser.h"
#include "linenoise.h"
#include "utils/split.hpp"
#include "utils/color.hpp"
#include "Parser.hpp"
#include "Dictionary.hpp"

std::unique_ptr<Dictionary> dictionary{nullptr};

struct Arg : public option::Arg {
    static void printError(const char *msg1, const option::Option &opt, const char *msg2) {
        fprintf(stderr, "%s", msg1);
        fwrite(opt.name, opt.namelen, 1, stderr);
        fprintf(stderr, "%s", msg2);
    }

    static option::ArgStatus Unknown(const option::Option &option, bool msg) {
        if (msg) printError("Unknown option '", option, "'\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Required(const option::Option &option, bool msg) {
        if (option.arg != 0)
            return option::ARG_OK;

        if (msg) printError("Option '", option, "' requires an argument\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus NonEmpty(const option::Option &option, bool msg) {
        if (option.arg != 0 && option.arg[0] != 0)
            return option::ARG_OK;

        if (msg) printError("Option '", option, "' requires a non-empty argument\n");
        return option::ARG_ILLEGAL;
    }

    static option::ArgStatus Numeric(const option::Option &option, bool msg) {
        char *endptr = 0;
        if (option.arg != 0 && strtol(option.arg, &endptr, 10)) {};
        if (endptr != option.arg && *endptr == 0)
            return option::ARG_OK;

        if (msg) printError("Option '", option, "' requires a numeric argument\n");
        return option::ARG_ILLEGAL;
    }
};

enum optionIndex {
    UNKNOWN, HELP, NGRAM, DICTIONARY, FILE_INPUT, INTERACTIVE, VERBOSE
};
const option::Descriptor usage[] = {
        {UNKNOWN,     0, "",  "",            Arg::None,     "USAGE: shingles [options]\n\nOptions:"},
        {HELP,        0, "h", "help",        Arg::None,     "  -h, --help  \tPrint usage and exit."},
        {NGRAM,       0, "n", "ngram",       Arg::Numeric,  "  -n, --ngram  \tn-gram depth."},
        {DICTIONARY,  0, "d", "dictionary",  Arg::Required, "  -d <file>, --dictionary=<file>  \tLoad the dictionary JSON file."},
        {FILE_INPUT,  0, "f", "file",        Arg::Required, "  -f <file>, --file=<file>  \tIngest the file."},
        {INTERACTIVE, 0, "i", "interactive", Arg::None,     "  -i, --interactive  \tInteractive console."},
        {VERBOSE,     0, "v", "verbose",     Arg::None,     "  -v, --verbose  \tVerbose mode."},
        {0,           0, 0,   0,             0,             0}
};

char *hints(const char *buf, int *color, int *bold) {
    std::string buffer(buf);
    if ( !buffer.empty() ) {
        //TODO: Autocomplete current word if buffer.back() != ' '
        std::string hint = dictionary->nextMostProbableWord(buffer);
        if (!hint.empty()) {
            *color = Color::FG_DARK_GRAY;
            *bold = 0;
            return strdup(std::string((buffer.back() != ' ' ? " " : "") + hint).c_str());
        }
    }
    return NULL;
}

void completion(const char *buf, linenoiseCompletions *lc) {
    std::string buffer(buf);
    if ( !buffer.empty() ) {
        const std::vector<const std::string> words = dictionary->nextCandidateWords(buffer);
        for (auto w:words) {
            linenoiseAddCompletion(lc, (buffer + (buffer.back() != ' ' ? " " : "") + w).c_str());
        }
    }
}

int main(int argc, char *argv[]) {
    // skip program name argv[0] if present
    argc -= (argc > 0);
    argv += (argc > 0);
    option::Stats stats(usage, argc, argv);
    std::vector<option::Option> options(stats.options_max);
    std::vector<option::Option> buffer(stats.buffer_max);
    option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

    if (parse.error())
        return 1;

    if (options[HELP] || argc == 0) {
        option::printUsage(std::cout, usage);
        return 0;
    }

    for (option::Option *opt = options[UNKNOWN]; opt; opt = opt->next())
        std::cout << "Unknown option: " << opt->name << "\n";

    for (int i = 0; i < parse.nonOptionsCount(); ++i)
        std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

    // Create the dictionary

    if (options[DICTIONARY]) {
        dictionary = std::make_unique<Dictionary>();
        dictionary->open(options[DICTIONARY].arg);
    } else {
        int n = 3;
        if (options[NGRAM]) {
            n = std::stoi(options[NGRAM].arg);
        }
        dictionary = std::make_unique<Dictionary>(n);
        std::cout << "Created a " << n << "-gram dictionary" << std::endl;
    }

    if (options[VERBOSE]) {
        dictionary->setDebug(true);
    }

    if (options[FILE_INPUT]) {
        dictionary->ingestFile(options[FILE_INPUT].arg);
    }

    if (options[INTERACTIVE]) {
        std::cout << "User :h or :help to see a list of commands." << std::endl;

        char *line;
        linenoiseHistorySetMaxLen(32);
        linenoiseSetHintsCallback(hints);
        linenoiseSetCompletionCallback(completion);
        while ((line = linenoise("human: ")) != nullptr) {
            const std::string line_str = std::string(line);
            if (line_str[0] == ':') {
                std::vector<std::string> input = split(line_str.substr(1, std::string::npos), ' ');
                if (input.size() > 0) {
                    const std::string &command = input[0];
                    std::vector<std::string> arguments = std::vector<std::string>(input.cbegin() + 1, input.cend());
                    const std::string arguments_str =
                            arguments.size() > 0 ? line_str.substr(command.size() + 2, std::string::npos) : "";

                    if (command == "h" || command == "help") {
                        std::cout << ":h, :help                          This command" << std::endl;
                        std::cout << ":d, :debug                         Toggle (extremely) verbose debug output" << std::endl;
                        std::cout << ":s <filename>, :save <filename>    Save the dictionary file" << std::endl;
                        std::cout << ":o <filename>, :open <filename>    Open a dictionary file" << std::endl;
                        std::cout << ":i <filename>, :ingest <filename>  Ingest/learn a text file" << std::endl;
                        std::cout << std::endl;
                        std::cout << ">        Seed the sentence generation with text entered after the >" << std::endl;
                        std::cout << "<enter>  Generate a new sentence" << std::endl;
                        std::cout << "<tab>    Autocomplete" << std::endl;
                        std::cout << std::endl;
                        std::cout << "Otherwise, type some text then press <enter> to ingest/learn that text before generating a new sentence" << std::endl;
                        std::cout << std::endl;

                    } else if (command == "s" || command == "save") {
                        if (arguments.size() == 1) {
                            dictionary->save(arguments[0]);
                        } else {
                            std::cerr << "Invalid number of arguments" << std::endl;
                        }
                    } else if (command == "o" || command == "open") {
                        if (arguments.size() == 1) {
                            dictionary->open(arguments[0]);
                        } else {
                            std::cerr << "Invalid number of arguments" << std::endl;
                        }
                    } else if (command == "i" || command == "ingest") {
                        if (arguments.size() == 1) {
                            dictionary->ingestFile(arguments[0]);
                        } else {
                            std::cerr << "Invalid number of arguments" << std::endl;
                        }
                    } else if (command == "d" || command == "debug") {
                        dictionary->setDebug();
                    } else {
                        std::cerr << "Unknown command" << std::endl;
                    }
                } else {
                    std::cerr << "Please enter a command" << std::endl;
                }
            } else if (line_str[0] == '>') {
                std::string wisdom = dictionary->generate("", line_str.substr(1, std::string::npos));
                std::cout << Color::FG_MAGENTA << "shingles"
                          << Color::FG_DARK_GRAY << ": "
                          << Color::FG_DEFAULT
                          << wisdom
                          << std::endl;

            } else {
                if (line_str.size() > 0) {
                    dictionary->input(line_str);
                }
                std::string wisdom = dictionary->generate(line_str);
                std::cout<< Color::FG_MAGENTA << "shingles"
                         << Color::FG_DARK_GRAY << ": "
                         << Color::FG_DEFAULT
                         << wisdom
                         << std::endl;
            }

            linenoiseHistoryAdd(line);
            linenoiseFree(line);
        }
    }

    return 0;
}

