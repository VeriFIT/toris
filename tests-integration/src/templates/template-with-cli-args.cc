/**
 * TODO: Input automata, that are of type `NFA-bits` are mintermized!
 *  - If you want to skip mintermization, set the variable `MINTERMIZE_AUTOMATA` below to `false`
 */

// TODO: Modify when copying to target `/mata/tests-integration/src/utils/`.
#include "../utils/utils.hh"
#include "../utils/config.hh"

#include "mata/nfa/nfa.hh"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>

using namespace mata::nfa;

const bool MINTERMIZE_AUTOMATA = true;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Input file missing\n";
        return EXIT_FAILURE;
    }

    std::string filename = argv[1];

    Nfa aut;
    mata::OnTheFlyAlphabet alphabet{};
    if (load_automaton(filename, aut, alphabet, MINTERMIZE_AUTOMATA) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    // Setting precision of the times to fixed points and 4 decimal places
    std::cout << std::fixed << std::setprecision(4);

    TIME_BEGIN(tmp);

    /**************************************************
     * TODO: HERE COMES YOUR CODE YOU WANT TO PROFILE *
     *   - Use alphabet alph as source alphabet       *
     *   - Use Nfa aut as source automaton            *
     *   - e.g. complement(aut, alph);                *
     **************************************************/

    TIME_END(tmp);

    return EXIT_SUCCESS;
}
