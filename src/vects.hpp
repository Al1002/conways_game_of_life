#pragma once
#include <hashtable.h>
#include <type_traits>

#include <tmp_extensions.hpp>

class Vect{
    /**
     * @brief ABClass for all vectors
     * 
     */
};

template<typename Num>
class Vect2 : public Vect
{
    /**
     * @brief Template class for 2d vectors. Template must support numeric opperations
     * 
     */
public:
    Num x, y;
    constexpr Vect2()
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

    Vect2& operator+=(const Vect2<Num>& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Vect2& operator-=(const Vect2<Num>& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline size_t hash() const
    {
        size_t h1 = std::hash<Num>()(x);
        size_t h2 = std::hash<Num>()(y);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2)); // Murphys combination
    }
    inline bool operator==(const Vect2& other) const
    {
        return other.x == x && other.y == y;
    }
};
namespace std {
    template <typename Num>
    struct hash<Vect2<Num>>
    {
        hash(){};
        std::size_t operator()(const Vect2<Num>& vect) const noexcept
        {
            return vect.hash();
        }
    };
}

/* Template class NVect
    WARNING: to ensure type safety, NVect uses extensive metaprogramming.
    This may cause: wierd compile time errors and long compile times.
    Very safe at runtime. Proseed with caution.
*/
template<size_t Dim, typename Num>
class NVect : public Vect
{
    /**
     * @brief Template class for n-dimentional vectors. Template must support numeric opperations
     * 
     */
public:
    Num components[Dim];
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
    
    Num& operator[](const size_t &n)
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
    template <size_t U = Dim>
    constexpr Num& y()
    {
        // "y only exists for 2+ dimentional vectors", SFINAE
        (typename std::enable_if<U >= 2>::type) 0;
        return components[1];
    }
    template <size_t U = Dim>
    constexpr Num& z()
    {
        // "z only exists for 3+ dimentional vectors", SFINAE
        (typename std::enable_if<U >= 3>::type) 0;
        return components[2];
    }
    template <size_t U = Dim>
    constexpr Num& w()
    {
        // "w only exists for 4+ dimentional vectors", SFINAE
        (typename std::enable_if<U >= 4>::type) 0;
        return components[3];
    }
    
    NVect operator+(const NVect<Dim, Num>& other) const
    {
        NVect<Dim, Num> rval;
        for(int i = 0; i < Dim; i++)
        {
            rval[i] = components[i] + other.components[i];
        }
        return rval;
    }
    NVect operator-(const NVect<Dim, Num>& other) const
    {
        NVect<Dim, Num> rval;
        for(int i = 0; i < Dim; i++)
        {
            rval[i] = components[i] - other.components[i];
        }
        return rval;
    }
    NVect operator*(const Num &scalar) const
    {
        NVect<Dim, Num> rval;
        for(int i = 0; i < Dim; i++)
        {
            rval[i] = components[i] * scalar;
        }
        return rval;
    }
    NVect operator/(const Num &scalar) const
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
            if(components[i] != other.components[i])
                return false;
        }
        return true;
    }
};
namespace std {
    template <size_t Dim, typename Num>
    struct hash<NVect<Dim, Num>>
    {
        hash(){};
        std::size_t operator()(const NVect<Dim, Num>& vect) const noexcept
        {
            return vect.hash();
        }
    };
}

typedef Vect2<int> Vect2i;
typedef Vect2<float> Vect2f;

typedef NVect<3, int> Vect3i;
typedef NVect<4, int> Vect4i;
