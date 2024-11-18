/* re2parser.hh -- Parser transforming re2 regular expressions to their corresponding automata representations.
 */

#ifndef MATA_RE2PARSER_HH
#define MATA_RE2PARSER_HH

#include <string>

#include "mata/nfa/nfa.hh"


// Encoding for the regular expression
// FIXME: Use enum class re2::Regexp::ParseFlags from re2/regexp.h instead. It is not possible to include it here. Need to fix cmake.
enum class Encoding {
    UTF8 = 0,
    Latin1 = 1 << 5
};


/**
 * @brief Parser from regular expression to automata.
 *
 * Currently supported automata types are NFA and AFA.
 */
namespace mata::parser {
    void create_nfa(nfa::Nfa* nfa, const std::string &pattern, bool use_epsilon = false, mata::Symbol epsilon_value = 306,
                    bool use_reduce = true, const Encoding encoding = Encoding::Latin1);
}

#endif // MATA_RE2PARSER_HH
