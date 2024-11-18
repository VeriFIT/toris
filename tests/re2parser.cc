#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "mata/parser/re2parser.hh"
#include "mata/nfa/builder.hh"
#include "mata/nfa/nfa.hh"

using namespace mata::nfa;

using Symbol = mata::Symbol;
using Word = std::vector<Symbol>;
using OnTheFlyAlphabet = mata::OnTheFlyAlphabet;

// Some example regexes were taken from RegExr under GPL v3: https://github.com/gskinner/regexr.

TEST_CASE("mata::Parser basic_parsing") {
    Nfa aut;

    SECTION("Empty expression") {
        mata::parser::create_nfa(&aut, "");
        REQUIRE(aut.final.size() == aut.initial.size());
        REQUIRE(aut.delta.empty());
        REQUIRE(!aut.is_lang_empty());
        REQUIRE(aut.is_in_lang(Word{}));
    }

    SECTION("Basic test")
    {
        mata::parser::create_nfa(&aut, "abcd");
        REQUIRE(!aut.delta.empty());
        REQUIRE(!aut.is_lang_empty());
        REQUIRE(!aut.is_in_lang(Word{'a','b','c'}));
        REQUIRE(aut.is_in_lang(Word{'a','b','c','d'}));
        REQUIRE(!aut.is_in_lang(Word{'a','b','c','d','d'}));
        REQUIRE(!aut.is_in_lang(Word{'a','d','c'}));
    }

    SECTION("Hex symbol encoding")
    {
        mata::parser::create_nfa(&aut, "\\x7f");
        REQUIRE(!aut.delta.empty());
        REQUIRE(!aut.is_lang_empty());
        REQUIRE(aut.is_in_lang(Word{127}));
    }

    SECTION("Wild cardinality")
    {
        mata::parser::create_nfa(&aut, ".*");
        REQUIRE(!aut.delta.empty());
        REQUIRE(!aut.is_lang_empty());
        REQUIRE(aut.is_in_lang(Word{'w','h','a','t','e','v','e','r'}));
        REQUIRE(aut.is_in_lang(Word{127}));
        REQUIRE(aut.is_in_lang(Word{0x7f}));
        REQUIRE(aut.is_in_lang(Word{}));
        OnTheFlyAlphabet alph{};
        REQUIRE(aut.is_universal(alph));
    }

    SECTION("Special character") {
        mata::parser::create_nfa(&aut, "\\t");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(aut.is_in_lang(Word{'\t'}));
        CHECK(!aut.is_in_lang(Word{'t'}));
        CHECK(!aut.is_in_lang(Word{}));
    }

    SECTION("Whitespace") {
        mata::parser::create_nfa(&aut, "a\\sb");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(aut.is_in_lang(Word{'a', '\t', 'b'}));
        CHECK(!aut.is_in_lang(Word{}));
    }

    SECTION("Iteration test")
    {
        mata::parser::create_nfa(&aut, "ab*cd*");
        REQUIRE(!aut.delta.empty());
        REQUIRE(!aut.is_lang_empty());
        REQUIRE(aut.is_in_lang(Word{'a','b','c'}));
        REQUIRE(aut.is_in_lang(Word{'a','b','c','d'}));
        REQUIRE(aut.is_in_lang(Word{'a','c','d'}));
        REQUIRE(aut.is_in_lang(Word{'a','b','b','c','d'}));
        REQUIRE(aut.is_in_lang(Word{'a','b','c','d','d'}));
        REQUIRE(!aut.is_in_lang(Word{'a','d','c'}));
    }

    SECTION("Additional parenthesis") {
        Nfa expected{2};
        expected.initial.insert(0);
        expected.final.insert(1);
        expected.delta.add(0, 'a', 0);
        expected.delta.add(0, 'b', 1);

        SECTION("No parenthesis") {
            mata::parser::create_nfa(&aut, "a*b");
        }

        SECTION("Around example parenthesis") {
            mata::parser::create_nfa(&aut, "(a*b)");
        }

        SECTION("Around variable 'a' parenthesis") {
            mata::parser::create_nfa(&aut, "(a)*b");
        }

        SECTION("Around variable 'b' parenthesis") {
            mata::parser::create_nfa(&aut, "a*(b)");
        }

        SECTION("Parenthesis after iteration") {
            mata::parser::create_nfa(&aut, "((a)*)b");
        }

        SECTION("Double parenthesis around 'b'") {
            mata::parser::create_nfa(&aut, "(a*(b))");
        }

        SECTION("Double parenthesis around 'a'") {
            mata::parser::create_nfa(&aut, "((a)*b)");
        }

        SECTION("Many parenthesis") {
            mata::parser::create_nfa(&aut, "(((a)*)b)");
        }

        SECTION("Double parenthesis") {
            mata::parser::create_nfa(&aut, "((a))*((b))");
        }

        SECTION("Double parenthesis after iteration") {
            mata::parser::create_nfa(&aut, "((((a))*))((b))");
        }

        SECTION("Many parenthesis with double parenthesis") {
            mata::parser::create_nfa(&aut, "(((((a))*))((b)))");
        }

        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'a','b'}));
        CHECK(aut.is_in_lang(Word{'a','a','b'}));
        CHECK(!aut.is_in_lang(Word{'b','a'}));
        CHECK(are_equivalent(aut, expected));
    }

    SECTION("Complex regex") {
        mata::parser::create_nfa(&aut, "(a+)|(e)(w*)(b+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'e', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
        CHECK(!aut.is_in_lang(Word{'a', 'e', 'b'}));
    }

    SECTION("Complex regex with additional plus") {
        mata::parser::create_nfa(&aut, "(a+)|(e)(w*)+(b+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'e', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Reduced complex regex with additional plus") {
        mata::parser::create_nfa(&aut, "(e)(w*)+(b+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'e', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Reduced complex regex with additional plus 2") {
        mata::parser::create_nfa(&aut, "(w*)+(b+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'w'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Reduced complex regex with additional plus 2.5") {
        mata::parser::create_nfa(&aut, "(w*)(b+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'w'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Reduced complex regex with additional plus 2.63") {
        mata::parser::create_nfa(&aut, "w*b+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'w'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Reduced complex regex with additional plus 2.75") {
        mata::parser::create_nfa(&aut, "w(b+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b'}));
        CHECK(!aut.is_in_lang(Word{'w', 'w', 'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'w'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Reduced complex regex with additional plus 2.85") {
        mata::parser::create_nfa(&aut, "w*(b+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'w'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Reduced complex regex with additional plus 3") {
        mata::parser::create_nfa(&aut, "(b+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Complex regex 2") {
        mata::parser::create_nfa(&aut, "(a+)|(e)(w*)(b*)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'e', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w'}));
        CHECK(aut.is_in_lang(Word{'e', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("Complex regex 2 with additional plus") {
        mata::parser::create_nfa(&aut, "(a+)|(e)(w*)+(b*)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'e', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w'}));
        CHECK(aut.is_in_lang(Word{'e', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'e', 'w', 'w', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'a', 'w', 'b'}));
    }

    SECTION("a+b+") {
        mata::parser::create_nfa(&aut, "a+b+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a', 'b', 'a'}));
    }

    SECTION("a+b+a*") {
        mata::parser::create_nfa(&aut, "a+b+a*");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b', 'a', 'a'}));
    }

    SECTION("a+(b+)a*") {
        mata::parser::create_nfa(&aut, "a+(b+)a*");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b', 'a', 'a'}));
    }

    SECTION("(a+(b+)a*)") {
        mata::parser::create_nfa(&aut, "(a+(b+)a*)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b', 'a', 'a'}));
    }

    SECTION("(a+b*a*)") {
        mata::parser::create_nfa(&aut, "(a+b*a*)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b', 'b', 'a', 'a'}));
    }

    SECTION("a+a+") {
        mata::parser::create_nfa(&aut, "a+a+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'a'}));
    }

    SECTION("(a+)a+") {
        mata::parser::create_nfa(&aut, "(a+)a+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'a'}));
    }

    SECTION("a(a+)") {
        mata::parser::create_nfa(&aut, "a(a+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'a'}));
    }

    SECTION("(a+)b") {
        mata::parser::create_nfa(&aut, "(a+)b");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a', 'b'}));
    }

    SECTION("b(a+)") {
        mata::parser::create_nfa(&aut, "b(a+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'a', 'a', 'a'}));
    }

    SECTION("b|(a+)") {
        mata::parser::create_nfa(&aut, "b|(a+)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'a'}));
        CHECK(!aut.is_in_lang(Word{'b', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a'}));
    }

    SECTION("b|a+") {
        mata::parser::create_nfa(&aut, "b|a+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'a'}));
        CHECK(!aut.is_in_lang(Word{'b', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a'}));
    }

    SECTION("b|a") {
        mata::parser::create_nfa(&aut, "b|a");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'a'}));
        CHECK(!aut.is_in_lang(Word{'b', 'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a', 'a'}));
    }

    SECTION("b|a*") {
        mata::parser::create_nfa(&aut, "b|a*");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'a'}));
        CHECK(!aut.is_in_lang(Word{'b', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'a'}));
    }

    SECTION("bba+") {
        mata::parser::create_nfa(&aut, "bba+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'b', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b', 'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b', 'a', 'a', 'a'}));
    }

    SECTION("b*ba+") {
        mata::parser::create_nfa(&aut, "b*ba+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b', 'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b', 'a', 'a', 'a'}));
    }

    SECTION("b*ca+") {
        mata::parser::create_nfa(&aut, "b*ca+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'c', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'c', 'a'}));
        CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'c', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b', 'c', 'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'c', 'a', 'a', 'a'}));
    }

    SECTION("[abcd]") {
        mata::parser::create_nfa(&aut, "[abcd]");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'c'}));
        CHECK(aut.is_in_lang(Word{'d'}));
        CHECK(!aut.is_in_lang(Word{'b', 'b'}));
    }

    SECTION("[abcd]*") {
        mata::parser::create_nfa(&aut, "[abcd]*");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'c'}));
        CHECK(aut.is_in_lang(Word{'d'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'c', 'c'}));
        CHECK(aut.is_in_lang(Word{'d', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c'}));
    }

    SECTION("[abcd]*e*") {
        mata::parser::create_nfa(&aut, "[abcd]*e*");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'c'}));
        CHECK(aut.is_in_lang(Word{'d'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'c', 'c'}));
        CHECK(aut.is_in_lang(Word{'d', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c'}));

        CHECK(aut.is_in_lang(Word{'a', 'e'}));
        CHECK(aut.is_in_lang(Word{'d', 'd', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'e', 'e'}));
    }

    SECTION("[abcd]*e+") {
        mata::parser::create_nfa(&aut, "[abcd]*e+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'c'}));
        CHECK(!aut.is_in_lang(Word{'d'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'b', 'b'}));
        CHECK(!aut.is_in_lang(Word{'c', 'c'}));
        CHECK(!aut.is_in_lang(Word{'d', 'd'}));
        CHECK(!aut.is_in_lang(Word{'a', 'd'}));
        CHECK(!aut.is_in_lang(Word{'a', 'd', 'c'}));

        CHECK(aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'e'}));
        CHECK(aut.is_in_lang(Word{'d', 'd', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'e', 'e'}));
    }

    SECTION("[abcd]*.*") {
        mata::parser::create_nfa(&aut, "[abcd]*.*");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'c'}));
        CHECK(aut.is_in_lang(Word{'d'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'c', 'c'}));
        CHECK(aut.is_in_lang(Word{'d', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c'}));

        CHECK(aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'e'}));
        CHECK(aut.is_in_lang(Word{'d', 'd', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'e', 'e'}));

        CHECK(aut.is_in_lang(Word{'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'g'}));
        CHECK(aut.is_in_lang(Word{'d', 'd', 'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'g', 'g'}));
    }

    SECTION("[abcd]*.+") {
        mata::parser::create_nfa(&aut, "[abcd]*.+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'c'}));
        CHECK(aut.is_in_lang(Word{'d'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'c', 'c'}));
        CHECK(aut.is_in_lang(Word{'d', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c'}));

        CHECK(aut.is_in_lang(Word{'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'e'}));
        CHECK(aut.is_in_lang(Word{'d', 'd', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'e'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'e', 'e'}));

        CHECK(aut.is_in_lang(Word{'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'g'}));
        CHECK(aut.is_in_lang(Word{'d', 'd', 'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'd', 'c', 'g', 'g'}));
    }

    SECTION("[a-c]+") {
        mata::parser::create_nfa(&aut, "[a-c]+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'c'}));
        CHECK(!aut.is_in_lang(Word{'d'}));
        CHECK(aut.is_in_lang(Word{'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'c', 'c'}));
        CHECK(!aut.is_in_lang(Word{'d', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'c'}));
    }

    SECTION("d[a-c]+") {
        mata::parser::create_nfa(&aut, "d[a-c]+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'c'}));
        CHECK(!aut.is_in_lang(Word{'d'}));
        CHECK(aut.is_in_lang(Word{'d', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'d', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'d', 'c', 'c'}));
        CHECK(!aut.is_in_lang(Word{'d', 'd'}));
        CHECK(aut.is_in_lang(Word{'d', 'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'d', 'a', 'b', 'c'}));
    }

    SECTION("d*[a-c]+") {
        mata::parser::create_nfa(&aut, "d*[a-c]+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a'}));
        CHECK(aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'c'}));
        CHECK(!aut.is_in_lang(Word{'d'}));
        CHECK(aut.is_in_lang(Word{'d', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'d', 'b', 'b'}));
        CHECK(aut.is_in_lang(Word{'d', 'c', 'c'}));
        CHECK(!aut.is_in_lang(Word{'d', 'd'}));
        CHECK(aut.is_in_lang(Word{'d', 'a', 'b'}));
        CHECK(aut.is_in_lang(Word{'d', 'a', 'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'d', 'd', 'a', 'b', 'c'}));
    }

    SECTION("[^a-c]") {
        mata::parser::create_nfa(&aut, "[^a-c]");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'c'}));
        CHECK(aut.is_in_lang(Word{'d'}));
        CHECK(!aut.is_in_lang(Word{'d', 'd'}));
        CHECK(aut.is_in_lang(Word{'e'}));
        CHECK(!aut.is_in_lang(Word{'e', 'e'}));
    }

    SECTION("(ha)+") {
        mata::parser::create_nfa(&aut, "(ha)+");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'h'}));
        CHECK(aut.is_in_lang(Word{'h', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'h'}));
        CHECK(aut.is_in_lang(Word{'h', 'a', 'h', 'a'}));
        CHECK(!aut.is_in_lang(Word{'h', 'a', 'h'}));
        CHECK(!aut.is_in_lang(Word{'h', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'h', 'a', 'h', 'a', 'h', 'a'}));
    }

    SECTION("(ha)*") {
        mata::parser::create_nfa(&aut, "(ha)*");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'a'}));
        CHECK(!aut.is_in_lang(Word{'h'}));
        CHECK(aut.is_in_lang(Word{'h', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'h'}));
        CHECK(aut.is_in_lang(Word{'h', 'a', 'h', 'a'}));
        CHECK(!aut.is_in_lang(Word{'h', 'a', 'h'}));
        CHECK(!aut.is_in_lang(Word{'h', 'a', 'a'}));
        CHECK(aut.is_in_lang(Word{'h', 'a', 'h', 'a', 'h', 'a'}));
    }

    SECTION("b\\w{2,3}") {
        mata::parser::create_nfa(&aut, "b\\w{2,3}");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(!aut.is_in_lang(Word{'b', 'e'}));
        CHECK(aut.is_in_lang(Word{'b', 'e', 'e'}));
        CHECK(aut.is_in_lang(Word{'b', 'e', 'e', 'r'}));
        CHECK(!aut.is_in_lang(Word{'b', 'e', 'e', 'r', 's'}));
    }

    SECTION("b\\w+?") {
        mata::parser::create_nfa(&aut, "b\\w+?");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(!aut.is_in_lang(Word{'b'}));
        CHECK(aut.is_in_lang(Word{'b', 'e'}));
        CHECK(!aut.is_in_lang(Word{'b', 'e', 'e'}));
        CHECK(!aut.is_in_lang(Word{'b', 'e', 'e', 'r'}));
        CHECK(!aut.is_in_lang(Word{'b', 'e', 'e', 'r', 's'}));
    }

    SECTION("b(a|e|i)d") {
        mata::parser::create_nfa(&aut, "b(a|e|i)d");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'b', 'a', 'd'}));
        CHECK(!aut.is_in_lang(Word{'b', 'u', 'd'}));
        CHECK(!aut.is_in_lang(Word{'b', 'o', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'e', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'i', 'd'}));
    }

    SECTION("[ab](c|d)") {
        mata::parser::create_nfa(&aut, "[ab](c|d)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'd'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'c', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'e'}));
        CHECK(!aut.is_in_lang(Word{'a', 'c', 'd'}));
    }

    SECTION("[ab](c|d)") {
        mata::parser::create_nfa(&aut, "[ab](c|d)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'd'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'c', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'e'}));
        CHECK(!aut.is_in_lang(Word{'a', 'c', 'd'}));
    }

    SECTION("[ab]+(c|d)") {
        mata::parser::create_nfa(&aut, "[ab]+(c|d)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'a', 'c'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'c', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'e'}));
        CHECK(!aut.is_in_lang(Word{'a', 'c', 'd'}));
    }

    SECTION("([ab])+(c|d)") {
        mata::parser::create_nfa(&aut, "([ab])+(c|d)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'a', 'c'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'c', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'e'}));
        CHECK(!aut.is_in_lang(Word{'a', 'c', 'd'}));
    }

    SECTION("(([ab])+)(c|d)") {
        mata::parser::create_nfa(&aut, "(([ab])+)(c|d)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'a', 'c'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'c', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'e'}));
        CHECK(!aut.is_in_lang(Word{'a', 'c', 'd'}));
    }

    SECTION("g|((([ab])+)(c|d))") {
        mata::parser::create_nfa(&aut, "(g|(([ab])+))(c|d)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'a', 'c'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'c', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'e'}));
        CHECK(!aut.is_in_lang(Word{'a', 'c', 'd'}));
        CHECK(aut.is_in_lang(Word{'g', 'c'}));
        CHECK(aut.is_in_lang(Word{'g', 'd'}));
    }

    SECTION("g|([ab])+(c|d)") {
        mata::parser::create_nfa(&aut, "g|([ab])+(c|d)");
        CHECK(!aut.delta.empty());
        CHECK(!aut.is_lang_empty());
        CHECK(!aut.is_in_lang(Word{}));
        CHECK(aut.is_in_lang(Word{'g'}));
        CHECK(aut.is_in_lang(Word{'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'd'}));
        CHECK(aut.is_in_lang(Word{'b', 'd'}));
        CHECK(aut.is_in_lang(Word{'a', 'a', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'c'}));
        CHECK(aut.is_in_lang(Word{'a', 'b', 'a', 'c'}));
        CHECK(!aut.is_in_lang(Word{'a', 'a'}));
        CHECK(!aut.is_in_lang(Word{'c', 'a'}));
        CHECK(!aut.is_in_lang(Word{'a', 'e'}));
        CHECK(!aut.is_in_lang(Word{'a', 'c', 'd'}));
    }

    SECTION("Star iteration") {
        Nfa expected{2};
        expected.initial.insert(0);
        expected.final.insert({0, 1});
        expected.delta.add(0, 'c', 0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(1, 'a', 1);

        SECTION("(((c)*)((a)*))") {
            mata::parser::create_nfa(&aut, "(((c)*)((a)*))");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(aut.is_in_lang(Word{}));
            CHECK(aut.is_in_lang(Word{'c'}));
            CHECK(aut.is_in_lang(Word{'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c'}));
            CHECK(aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            CHECK(are_equivalent(aut, expected));
        }

        SECTION("((c*)((a)*))") {
            mata::parser::create_nfa(&aut, "((c*)((a)*))");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(aut.is_in_lang(Word{}));
            CHECK(aut.is_in_lang(Word{'c'}));
            CHECK(aut.is_in_lang(Word{'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c'}));
            CHECK(aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            CHECK(are_equivalent(aut, expected));
        }

        SECTION("(c*(a*))") {
            mata::parser::create_nfa(&aut, "(c*(a*))");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(aut.is_in_lang(Word{}));
            CHECK(aut.is_in_lang(Word{'c'}));
            CHECK(aut.is_in_lang(Word{'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c'}));
            CHECK(aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            CHECK(are_equivalent(aut, expected));
        }

        SECTION("(c*a*)") {
            mata::parser::create_nfa(&aut, "(c*a*)");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(aut.is_in_lang(Word{}));
            CHECK(aut.is_in_lang(Word{'c'}));
            CHECK(aut.is_in_lang(Word{'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c'}));
            CHECK(aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            CHECK(are_equivalent(aut, expected));
        }

        SECTION("c*a*") {
            mata::parser::create_nfa(&aut, "c*a*");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(aut.is_in_lang(Word{}));
            CHECK(aut.is_in_lang(Word{'c'}));
            CHECK(aut.is_in_lang(Word{'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c'}));
            CHECK(aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            CHECK(are_equivalent(aut, expected));
        }

        SECTION("(((c)+)((a)+))") {
            mata::parser::create_nfa(&aut, "(((c)+)((a)+))");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(!aut.is_in_lang(Word{}));
            CHECK(!aut.is_in_lang(Word{'c'}));
            CHECK(!aut.is_in_lang(Word{'a'}));
            CHECK(!aut.is_in_lang(Word{'c', 'c'}));
            CHECK(!aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            Nfa expected_plus_iteration{ 3 };
            expected_plus_iteration.initial.insert(0);
            expected_plus_iteration.final.insert(2);
            expected_plus_iteration.delta.add(0, 'c', 1);
            expected_plus_iteration.delta.add(1, 'c', 1);
            expected_plus_iteration.delta.add(1, 'a', 2);
            expected_plus_iteration.delta.add(2, 'a', 2);
            CHECK(are_equivalent(aut, expected_plus_iteration));
        }

        SECTION("((c+)((a)+))") {
            mata::parser::create_nfa(&aut, "((c+)((a)+))");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(!aut.is_in_lang(Word{}));
            CHECK(!aut.is_in_lang(Word{'c'}));
            CHECK(!aut.is_in_lang(Word{'a'}));
            CHECK(!aut.is_in_lang(Word{'c', 'c'}));
            CHECK(!aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            Nfa expected_plus_iteration{ 3 };
            expected_plus_iteration.initial.insert(0);
            expected_plus_iteration.final.insert(2);
            expected_plus_iteration.delta.add(0, 'c', 1);
            expected_plus_iteration.delta.add(1, 'c', 1);
            expected_plus_iteration.delta.add(1, 'a', 2);
            expected_plus_iteration.delta.add(2, 'a', 2);
            CHECK(are_equivalent(aut, expected_plus_iteration));
        }

        SECTION("((c+)(a+))") {
            mata::parser::create_nfa(&aut, "((c+)(a+))");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(!aut.is_in_lang(Word{}));
            CHECK(!aut.is_in_lang(Word{'c'}));
            CHECK(!aut.is_in_lang(Word{'a'}));
            CHECK(!aut.is_in_lang(Word{'c', 'c'}));
            CHECK(!aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            Nfa expected_plus_iteration{ 3 };
            expected_plus_iteration.initial.insert(0);
            expected_plus_iteration.final.insert(2);
            expected_plus_iteration.delta.add(0, 'c', 1);
            expected_plus_iteration.delta.add(1, 'c', 1);
            expected_plus_iteration.delta.add(1, 'a', 2);
            expected_plus_iteration.delta.add(2, 'a', 2);
            CHECK(are_equivalent(aut, expected_plus_iteration));
        }

        SECTION("(c+)(a+)") {
            mata::parser::create_nfa(&aut, "(c+)(a+)");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(!aut.is_in_lang(Word{}));
            CHECK(!aut.is_in_lang(Word{'c'}));
            CHECK(!aut.is_in_lang(Word{'a'}));
            CHECK(!aut.is_in_lang(Word{'c', 'c'}));
            CHECK(!aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            Nfa expected_plus_iteration{ 3 };
            expected_plus_iteration.initial.insert(0);
            expected_plus_iteration.final.insert(2);
            expected_plus_iteration.delta.add(0, 'c', 1);
            expected_plus_iteration.delta.add(1, 'c', 1);
            expected_plus_iteration.delta.add(1, 'a', 2);
            expected_plus_iteration.delta.add(2, 'a', 2);
            CHECK(are_equivalent(aut, expected_plus_iteration));
        }

        SECTION("c+(a+)") {
            mata::parser::create_nfa(&aut, "c+(a+)");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(!aut.is_in_lang(Word{}));
            CHECK(!aut.is_in_lang(Word{'c'}));
            CHECK(!aut.is_in_lang(Word{'a'}));
            CHECK(!aut.is_in_lang(Word{'c', 'c'}));
            CHECK(!aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            Nfa expected_plus_iteration{ 3 };
            expected_plus_iteration.initial.insert(0);
            expected_plus_iteration.final.insert(2);
            expected_plus_iteration.delta.add(0, 'c', 1);
            expected_plus_iteration.delta.add(1, 'c', 1);
            expected_plus_iteration.delta.add(1, 'a', 2);
            expected_plus_iteration.delta.add(2, 'a', 2);
            CHECK(are_equivalent(aut, expected_plus_iteration));
        }

        SECTION("(c+)a+") {
            mata::parser::create_nfa(&aut, "(c+)a+");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(!aut.is_in_lang(Word{}));
            CHECK(!aut.is_in_lang(Word{'c'}));
            CHECK(!aut.is_in_lang(Word{'a'}));
            CHECK(!aut.is_in_lang(Word{'c', 'c'}));
            CHECK(!aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            Nfa expected_plus_iteration{ 3 };
            expected_plus_iteration.initial.insert(0);
            expected_plus_iteration.final.insert(2);
            expected_plus_iteration.delta.add(0, 'c', 1);
            expected_plus_iteration.delta.add(1, 'c', 1);
            expected_plus_iteration.delta.add(1, 'a', 2);
            expected_plus_iteration.delta.add(2, 'a', 2);
            CHECK(are_equivalent(aut, expected_plus_iteration));
        }

        SECTION("c+a+") {
            mata::parser::create_nfa(&aut, "c+a+");
            CHECK(!aut.delta.empty());
            CHECK(!aut.is_lang_empty());
            CHECK(!aut.is_in_lang(Word{}));
            CHECK(!aut.is_in_lang(Word{'c'}));
            CHECK(!aut.is_in_lang(Word{'a'}));
            CHECK(!aut.is_in_lang(Word{'c', 'c'}));
            CHECK(!aut.is_in_lang(Word{'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a'}));
            CHECK(!aut.is_in_lang(Word{'a', 'c'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'a', 'a'}));
            CHECK(aut.is_in_lang(Word{'c', 'c', 'a', 'a'}));
            Nfa expected_plus_iteration{ 3 };
            expected_plus_iteration.initial.insert(0);
            expected_plus_iteration.final.insert(2);
            expected_plus_iteration.delta.add(0, 'c', 1);
            expected_plus_iteration.delta.add(1, 'c', 1);
            expected_plus_iteration.delta.add(1, 'a', 2);
            expected_plus_iteration.delta.add(2, 'a', 2);
            CHECK(are_equivalent(aut, expected_plus_iteration));
        }
    }
} // }}}

TEST_CASE("mata::Parser error")
{ // {{{
    SECTION("Complex regex that fails")
    {
        mata::nfa::Nfa aut;
        mata::parser::create_nfa(&aut, "((aa)*)*(b)*");
        REQUIRE(!aut.delta.empty());
        REQUIRE(!aut.is_lang_empty());
        REQUIRE(aut.is_in_lang(Word{'a','a','b'}));
        REQUIRE(!aut.is_in_lang(Word{'a','b'}));
    }

    SECTION("Regexes from issue #48")
    {
        mata::nfa::Nfa aut1;
        mata::nfa::Nfa aut2;
        mata::parser::create_nfa(&aut1, "[qQrR]*");
        mata::parser::create_nfa(&aut2, "[qr]*");
        REQUIRE(!aut1.delta.empty());
        REQUIRE(!aut1.is_lang_empty());
        REQUIRE(!aut2.delta.empty());
        REQUIRE(!aut2.is_lang_empty());
        REQUIRE(aut1.is_in_lang(Word{'Q','R','q','r'}));
        REQUIRE(aut2.is_in_lang(Word{'q','r','q','r'}));
        REQUIRE(!aut2.is_in_lang(Word{'q','R','q'}));
    }

    SECTION("Regex from issue #139") {
        Nfa x;
        mata::parser::create_nfa(&x, "(cd(abcde)*)|(a(aaa)*)");
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'a', 'a' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'd', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'd' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a', 'a', 'a', 'a' }, {} }));

        x.clear();
        mata::parser::create_nfa(&x, "(cd(abcde)*)|(a(aaa)*)", false, 306, false);
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'a', 'a' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'd', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'd' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a', 'a', 'a', 'a' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a', 'a', 'a' }, {} }));
    }

    SECTION("Regex from issue #456") {
        Nfa x;
        mata::parser::create_nfa(&x, "[\\x00-\\x5a\\x5c-\\x7F]");

        Nfa y;
        State initial_s = 0;
        State final_s = 1;
        y.initial.insert(initial_s);
        y.final.insert(final_s);
        for (Symbol c = 0; c <= 0x7F; c++) {
            if (c == 0x5B) {
                continue;
            }
            y.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(x, y));
    }

    SECTION("Another failing regex") {
        Nfa x;
        mata::parser::create_nfa(&x, "(cd(abcde)+)|(a(aaa)+|ccc+)");
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'a', 'a' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a', 'a', 'a', 'a' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'd', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'd', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'd', 'a', 'b', 'c', 'd', 'e', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'c', 'd', 'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'c', 'c' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'c', 'd' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'c', 'c' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'c', 'c', 'c', 'c', 'c' }, {} }));

        x.clear();
        mata::parser::create_nfa(&x, "(cd(abcde)+)|(a(aaa)+|ccc+)", false, 306, false);
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'a', 'a' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'a', 'a', 'a', 'a', 'a', 'a', 'a' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'd', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'd', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'd', 'a', 'b', 'c', 'd', 'e', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'c', 'd', 'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd', 'e' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'c', 'c' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'c', 'd' }, {} }));
        CHECK(!x.is_in_lang(Run{ Word{ 'c', 'c' }, {} }));
        CHECK(x.is_in_lang(Run{ Word{ 'c', 'c', 'c', 'c', 'c', 'c' }, {} }));
    }
} // }}}

TEST_CASE("mata::Parser bug epsilon")
{ // {{{
    SECTION("failing regex")
    {
        Nfa x;
        mata::parser::create_nfa(&x, "(cd(abcde)*)|(a(aaa)*)");
        CHECK(x.is_in_lang(Run{Word{'a', 'a', 'a', 'a'}, {}}));
    }
} // }}}

TEST_CASE("mata::parser Parsing regexes with ^ and $") {
    Nfa nfa;
    Nfa expected{};

    SECTION("Handling of '\\'") {
        mata::parser::create_nfa(&nfa, "a\\\\b");
        expected = mata::nfa::builder::parse_from_mata(
        std::string{ R"(
            @NFA-explicit
            %Alphabet-auto
            %Initial q0
            %Final q3
            q0 97 q1
            q1 92 q2
            q2 98 q3)"
        });
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("a|b$, a simple OR example with end marker") {
        mata::parser::create_nfa(&nfa, "a|b$");
        expected.initial.insert(0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(0, 'b', 1);
        expected.final.insert(1);
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("^a|b, a simple OR example with begin marker") {
        mata::parser::create_nfa(&nfa, "^a|b");
        expected.initial.insert(0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(0, 'b', 1);
        expected.final.insert(1);
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("^a|b$, a simple OR example with begin and end marker") {
        mata::parser::create_nfa(&nfa, "^a|b$");
        expected.initial.insert(0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(0, 'b', 1);
        expected.final.insert(1);
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("^(a|b)$, a simple OR example with begin and end marker around capture group") {
        mata::parser::create_nfa(&nfa, "^(a|b)$");
        expected.initial.insert(0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(0, 'b', 1);
        expected.final.insert(1);
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("a$|b, a simple OR example with end marker on the left side") {
        mata::parser::create_nfa(&nfa, "a$|b");
        expected.initial.insert(0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(0, 'b', 1);
        expected.final.insert(1);
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("^a$|^b$, a simple OR example with multiple begin and end markers") {
        mata::parser::create_nfa(&nfa, "^a$|^b$");
        expected.initial.insert(0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(0, 'b', 1);
        expected.final.insert(1);
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("aed|(bab)$, a simple OR example with trailing end marker") {
        mata::parser::create_nfa(&nfa, "aed|(bab)$");
        expected.initial.insert(0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(1, 'e', 2);
        expected.delta.add(2, 'd', 3);
        expected.delta.add(0, 'b', 4);
        expected.delta.add(4, 'a', 5);
        expected.delta.add(5, 'b', 3);
        expected.final.insert(3);
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("aed|bab$, a simple OR example with trailing end marker") {
        mata::parser::create_nfa(&nfa, "aed|bab$");
        expected.initial.insert(0);
        expected.delta.add(0, 'a', 1);
        expected.delta.add(1, 'e', 2);
        expected.delta.add(2, 'd', 3);
        expected.delta.add(0, 'b', 4);
        expected.delta.add(4, 'a', 5);
        expected.delta.add(5, 'b', 3);
        expected.final.insert(3);
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }

    SECTION("^systempath\\=https|ftp$ correct parentheses") {
        mata::parser::create_nfa(&nfa, "^[sS][yY][sS][tT][eE][mM][pP][aA][tT][hH]\\\\=(([hH][tT]{2}[pP][sS]?)|([fF][tT][pP]))$");
        expected = mata::nfa::builder::parse_from_mata(std::string{ R"(
            @NFA-explicit
            %Alphabet-auto
            %Initial q0
            %Final q16 q17
            q0 83 q1
            q0 115 q1
            q1 89 q2
            q1 121 q2
            q2 83 q3
            q2 115 q3
            q3 84 q4
            q3 116 q4
            q4 69 q5
            q4 101 q5
            q5 77 q6
            q5 109 q6
            q6 80 q7
            q6 112 q7
            q7 65 q8
            q7 97 q8
            q8 84 q9
            q8 116 q9
            q9 72 q10
            q9 104 q10
            q10 92 q11
            q11 61 q12
            q12 70 q18
            q12 72 q13
            q12 102 q18
            q12 104 q13
            q13 84 q14
            q13 116 q14
            q14 84 q15
            q14 116 q15
            q15 80 q16
            q15 112 q16
            q16 83 q17
            q16 115 q17
            q18 84 q19
            q18 116 q19
            q19 80 q17
            q19 112 q17
            )"
        });
        CHECK(mata::nfa::are_equivalent(nfa, expected));
    }
}

TEST_CASE("Foldcase") {
    SECTION("Regex [a-z]") {
        Nfa nfa;
        mata::parser::create_nfa(&nfa, "[a-z]");

        Nfa result;
        State initial_s = 0;
        State final_s = 1;
        result.initial.insert(initial_s);
        result.final.insert(final_s);
        for (Symbol c = 'a'; c <= 'z'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(nfa, result));
    }

    SECTION("Regex [A-Z]") {
        Nfa nfa;
        mata::parser::create_nfa(&nfa, "[A-Z]");

        Nfa result;
        State initial_s = 0;
        State final_s = 1;
        result.initial.insert(initial_s);
        result.final.insert(final_s);
        for (Symbol c = 'A'; c <= 'Z'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(nfa, result));
    }

    SECTION("Regex [A-Za-z]") {
        Nfa nfa;
        mata::parser::create_nfa(&nfa, "[A-Za-z]");

        Nfa result;
        State initial_s = 0;
        State final_s = 1;
        result.initial.insert(initial_s);
        result.final.insert(final_s);
        for (Symbol c = 'A'; c <= 'Z'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        for (Symbol c = 'a'; c <= 'z'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(nfa, result));
    }

    SECTION("Regex [a-zA-Z]") {
        Nfa nfa;
        mata::parser::create_nfa(&nfa, "[a-zA-Z]");

        Nfa result;
        State initial_s = 0;
        State final_s = 1;
        result.initial.insert(initial_s);
        result.final.insert(final_s);
        for (Symbol c = 'A'; c <= 'Z'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        for (Symbol c = 'a'; c <= 'z'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(nfa, result));
    }

    SECTION("Regex [M-Ya-x]") {
        Nfa nfa;
        mata::parser::create_nfa(&nfa, "[M-Ya-x]");

        Nfa result;
        State initial_s = 0;
        State final_s = 1;
        result.initial.insert(initial_s);
        result.final.insert(final_s);
        for (Symbol c = 'a'; c <= 'x'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        for (Symbol c = 'M'; c <= 'Y'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(nfa, result));
    }

    SECTION("Regex [\\x00-\\x5a\\x5c-\\x7F]") {
        Nfa nfa;
        mata::parser::create_nfa(&nfa, "[\\x00-\\x5a\\x5c-\\x7F]");

        Nfa result;
        State initial_s = 0;
        State final_s = 1;
        result.initial.insert(initial_s);
        result.final.insert(final_s);
        for (Symbol c = 0; c <= 0x7F; c++) {
            if (c == 0x5B) {
                continue;
            }
            result.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(nfa, result));
    }

    SECTION("Regex [A-Ma-m]") {
        Nfa nfa;
        mata::parser::create_nfa(&nfa, "[A-Ma-m]");

        Nfa result;
        State initial_s = 0;
        State final_s = 1;
        result.initial.insert(initial_s);
        result.final.insert(final_s);
        for (Symbol c = 'A'; c <= 'M'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        for (Symbol c = 'a'; c <= 'm'; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(nfa, result));
    }

    SECTION("Regex [\\x00-\\x7F]") {
        Nfa nfa;
        mata::parser::create_nfa(&nfa, "[\\x00-\\x7F]");

        Nfa result;
        State initial_s = 0;
        State final_s = 1;
        result.initial.insert(initial_s);
        result.final.insert(final_s);
        for (Symbol c = 0; c <= 0x7F; c++) {
            result.delta.add(initial_s, c, final_s);
        }
        CHECK(are_equivalent(nfa, result));
    }
}
