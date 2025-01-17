/**
 * NOTE: Input automata, that are of type `NFA-bits` are mintermized!
 *  - If you want to skip mintermization, set the variable `MINTERMIZE_AUTOMATA` below to `false`
 */

#include "utils/utils.hh"

#include "mata/nfa/nfa.hh"
#include "mata/nfa/plumbing.hh"
#include "mata/nfa/algorithms.hh"

#include <iostream>
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

    Nfa det;
    TIME_BEGIN(determinize);
    // > START OF PROFILED CODE
    // Only determinize and its callees will be measured
    mata::nfa::plumbing::determinize(&det, aut);
    TIME_END(determinize);

    // Nfa det_boost;
    // TIME_BEGIN(determinize_boost);
    // // > START OF PROFILED CODE
    // // Only determinize_boost and its callees will be measured
    // mata::nfa::plumbing::determinize_boost(&det_boost, aut);
    // TIME_END(determinize_boost);


    return EXIT_SUCCESS;
}
