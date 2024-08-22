#include <stdlib.h>
#include <type_traits>

// Usefull metaprogramming extencions
namespace TMPExtensions
{
    template<size_t N, typename... Args>
    struct CheckSize {
        // Validates arglist has a lenght of N
        static constexpr bool value = (sizeof...(Args) == N);
    };
    
    template<typename TypeSequence, typename Head, typename... Args>
    struct MatchTypes{
        // Checks if a type list matches a type sequence
        static constexpr bool value = std::is_same<Head, typename TypeSequence::type>::value && MatchTypes<typename TypeSequence::next, Args...>::value;
    };

    template<typename TypeSequence, typename Head>
    struct MatchTypes<TypeSequence, Head>{
        // Trivial type
        static constexpr bool value = std::is_same<Head, typename TypeSequence::type>::value;
    };

    template<typename T>
    struct AllSame
    {
        // Type generator for the same type
        typedef T type;
        typedef AllSame<T> next;
    };

    template<size_t expected_size, typename... Args>
    constexpr void checkArgs(Args&&... args) {
        static_assert(CheckSize<expected_size, Args...>::value, "Argument list does not have the expected size!");
    }
}