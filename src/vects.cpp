#include <stdlib.h>
#include <hashtable.h>
#include <initializer_list>
#include <exception>
#include <type_traits>
#include <iostream> // cout

// Usefull metaprogramming extencions
namespace TMPExtensions
{
    template<std::size_t N, typename... Args>
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
    
        // Further processing if the size is correct
        std::cout << "Argument size is correct.\n";
    }
}

class Vect{
    /**
     * @brief ABClass for all vectors
     * 
     */
};

// Template class NVect
// WARNING: to ensure type safety, NVect uses extensive metaprogramming.
// This may cause: wierd compile time errors and long compile times.
// Very safe at runtime. Proseed with caution.
template<size_t Dim, typename Num>
class NVect : public Vect
{
    /**
     * @brief Template class for all vectors. Template must support numeric opperations
     * 
     */
    Num components[Dim];
public:
    static_assert(Dim > 0, "Vector must have a positive dimention number");
    static const size_t dim = Dim;
    
    constexpr NVect()
    {
        for(int i = 0; i < Dim; i++)
        {
            components[i] = Num();
        }
    }
    template<typename...Args>
    constexpr NVect(Args... args)
    {
        static_assert(TMPExtensions::CheckSize<Dim, Args...>::value, "Vector must have exactly as many components as its dimention!");
        static_assert(TMPExtensions::MatchTypes<TMPExtensions::AllSame<Num>, Args...>::value, "Vector components must all be the same type as the vector");
        int i = 0; 
        ((components[i++] = args, 0), ...);     
    }
    
    constexpr Num& operator[](size_t n)
    {
        if(n >= Dim)
        {
            throw std::exception(); // out of bounds exception!
        }
        return static_cast<Num&>(components[n]);
    }
    constexpr Num& x()
    {
        return components[0];
    }
    constexpr Num& y()
    {
        static_assert(Dim >= 2, "y only exists for 2+ dimentional vectors");
        return components[1];
    }
    constexpr Num& z()
    {
        static_assert(Dim >= 3, "z only exists for 3+ dimentional vectors");
        return components[2];
    }
    constexpr Num& w()
    {
        static_assert(Dim >= 4, "w only exists for 4+ dimentional vectors");
        return components[3];
    }
    
    NVect operator+(const NVect<Dim, Num>& other)
    {
        NVect<Dim, Num> rval;
        for(int i = 0; i < Dim; i++)
        {
            rval[i] = components[i] + other[i];
        }
        return rval;
    }
    NVect operator-(const NVect<Dim, Num>& other)
    {
        NVect<Dim, Num> rval;
        for(int i = 0; i < Dim; i++)
        {
            rval[i] = components[i] - other[i];
        }
        return rval;
    }
    NVect operator*(Num scalar)
    {
        NVect<Dim, Num> rval;
        for(int i = 0; i < Dim; i++)
        {
            rval[i] = components[i] * scalar;
        }
        return rval;
    }
    NVect operator/(Num scalar)
    {
        NVect<Dim, Num> rval;
        for(int i = 0; i < Dim; i++)
        {
            rval[i] = components[i] / scalar;
        }
        return rval;
    }

    NVect& operator+=(const NVect<Dim, Num>& other)
    {
        NVect<Dim, Num> rval;
        for(int i = 0; i < Dim; i++)
        {
            components[i] = components[i] + other.components[i];
        }
        return *this;
    }
    NVect& operator-=(const NVect<Dim, Num>& other)
    {
        for(int i = 0; i < Dim; i++)
        {
            components[i] = components[i] - other.components[i];
        }
        return *this;
    }
    
    size_t hash() const
    {
        size_t hash = 0;
        static const size_t prime = 31;
        for(int i = 0; i < Dim; i++)
        {
            hash += components[i] * prime;
        }
        return hash;
    }
    bool operator==(const NVect<Dim, Num>& other) const
    {
        for(int i = 0; i < Dim; i++)
        {
            if(components[i] != other[i])
                return false;
        }
        return true;
    }
};
namespace std {
    template <size_t Dim, typename Num>
    struct hash<NVect<Dim, Num>> {
        hash(){};
        std::size_t operator()(const NVect<Dim, Num>& vect) const noexcept
        {
            return vect.hash();
        }
    };
}

template<typename Num>
class Vect2 : public Vect
{
    /**
     * @brief Template class for 2d vectors. Template must support numeric opperations
     * 
     */
public:
    Num x, y;
    inline Vect2()
    {
        x = 0;
        y = 0;
    }
    inline Vect2(const Num& x, const Num& y)
    {
        this->x = x;
        this->y = y;
    }
    inline Vect2 operator+(const Vect2& other) const
    {
        return {x + other.x, y + other.y};
    }
    inline Vect2 operator-(const Vect2& other) const
    {
        return {x - other.x, y - other.y};
    }
    inline Vect2 operator*(const Num& scalar) const
    {
        return {x * scalar, y * scalar};
    }
    inline Vect2 operator/(const Num& scalar) const
    {
        return {x / scalar, y / scalar};
    }
    
    size_t hash() const
    {
        return ~(std::hash<Num>()(x) ^ ~(std::hash<Num>()(y))); 
    }
    inline bool operator==(const Vect2& other) const
    {
        return other.x == x && other.y == y;
    }
};
namespace std {
    template <typename Num>
    struct hash<Vect2<Num>> {
        hash(){};
        std::size_t operator()(const Vect2<Num>& vect) const noexcept
        {
            return vect.hash();
        }
    };
}

typedef Vect2<int> Vect2i;
typedef Vect2<float> Vect2f;

typedef NVect<3, int> Vect3i;
typedef NVect<4, int> Vect4i;

/*
int main()
{
    TMPExtensions::checkArgs<3>(1,2,3);
    Vect4i v, g(1,2,3,4);
    v.x() = 10;
    v.y() = 11;
    v.z() = 0;
    v.w() = 1;
    v+=g;
    for(int i = 0; i < v.dim; i++)
    {
        std::cout<<v[i]<<' ';
    }
    std::cout<<std::endl;
    std::hash<Vect4i>()(Vect4i(1,2,3,4));
    return 0;
}
*/