#include <fstream>
#include <vector>
#include "optionparser.h"
#include "linenoise.h"
#include "utils/split.hpp"
#include "utils/color.hpp"
#include "Parser.hpp"
#include "Dictionary.hpp"

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
    UNKNOWN, HELP, NGRAM, DICTIONARY, FILE_INPUT, INTERACTIVE
};
const option::Descriptor usage[] = {
        {UNKNOWN,     0, "",  "",            Arg::None,     "USAGE: shingles [options]\n\nOptions:"},
        {HELP,        0, "h", "help",        Arg::None,     "  -h, --help  \tPrint usage and exit."},
        {NGRAM,       0, "n", "ngram",       Arg::Numeric,  "  -n, --ngram  \tn-gram depth."},
        {DICTIONARY,  0, "d", "dictionary",  Arg::Required, "  -d <file>, --dictionary=<file>  \tLoad the dictionary JSON file."},
        {FILE_INPUT,  0, "f", "file",        Arg::Required, "  -f <file>, --file=<file>  \tIngest the file."},
        {INTERACTIVE, 0, "i", "interactive", Arg::None,     "  -i, --interactive  \tInteractive console."},
        {0,           0, 0,   0,             0,             0}
};

/*char *hints(const char *buf, int *color, int *bold) {
    if (!strcasecmp(buf, "git remote add")) {
        *color = 35;
        *bold = 0;
        return " <name> <url>";
    }
    return NULL;
}*/

/*void completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc, "hello");
        linenoiseAddCompletion(lc, "hallo");
        linenoiseAddCompletion(lc, "hello there");
    }
}*/

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
    std::unique_ptr<Dictionary> dictionary{nullptr};
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

    if (options[FILE_INPUT]) {
        dictionary->ingestFile(options[FILE_INPUT].arg);
    }

    if (options[INTERACTIVE]) {
        char *line;
        linenoiseHistorySetMaxLen(32);
        //linenoiseSetHintsCallback(hints);
        //linenoiseSetCompletionCallback(completion);
        while ((line = linenoise("shingles: ")) != NULL) {
            const std::string line_str = std::string(line);
            if (line_str[0] == ':') {
                std::vector<std::string> input = split(line_str.substr(1, std::string::npos), ' ');
                if (input.size() > 0) {
                    const std::string &command = input[0];
                    std::vector<std::string> arguments = std::vector<std::string>(input.cbegin() + 1, input.cend());
                    const std::string arguments_str =
                            arguments.size() > 0 ? line_str.substr(command.size() + 2, std::string::npos) : "";

                    if (command == "s" || command == "save") {
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
                std::cout << dictionary->generate(line_str.substr(1, std::string::npos)) << std::endl;

            } else {
                if (line_str.size() > 0) {
                    dictionary->input(line_str);
                }
                std::cout << dictionary->generate() << std::endl;
            }

            linenoiseHistoryAdd(line);
            linenoiseFree(line);
        }
    }

    return 0;
}

