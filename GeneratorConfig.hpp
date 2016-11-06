#ifndef SHINGLES_GENERATORCONFIG_HPP
#define SHINGLES_GENERATORCONFIG_HPP

#include "Word.hpp"

namespace Shingles {
    struct GeneratorConfig {
        GeneratorConfig();
        GeneratorConfig(
            const std::vector<const Word *> &sentence,
            unsigned long position,
            const std::stack<const Word *> &markerStack,
            const std::vector<const Word *> topic,
            bool finishSentence = false,
            bool debug = false
        )
            : sentence(sentence),
              position(position),
              markerStack(markerStack),
              topic(topic),
              finishSentence(finishSentence),
              debug(debug) {}

        const std::vector<const Word *> &sentence{};
        unsigned long position{0};
        const std::stack<const Word *> &markerStack{};
        const std::vector<const Word *> topic{};
        bool finishSentence{false};
        bool debug{false};
    };
}

#endif //SHINGLES_GENERATORCONFIG_HPP
