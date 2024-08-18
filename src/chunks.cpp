#include "vects.cpp"
#include <stdlib.h>
#include <unordered_map>

inline void set_bit(unsigned char *byte, int offset, bool val) 
{
    // Helper
    if(val)
        *byte = *byte | (1 << offset);
    else
        *byte = *byte & ~(1 << offset);
}

inline bool get_bit(unsigned char byte, int offset)
{
    //Helper
    return (byte >> offset) & 1;
}

class BoolGrid2D
{
    /**
     * @brief Interface for 2d boolean grid
     * 
     */
public:
    virtual bool get(Vect2i pos) const = 0;
    virtual void set(Vect2i pos, bool val) = 0;
};

class BoolChunk : public BoolGrid2D
{
public:
    static const int chunk_size_b = 1 << 12; //4096b, one eight page 
    static const int side_len_b = 1 << 6; // 64 rows or 8 octets
    static const int chunk_size = chunk_size_b / 8;
    static const int side_len = side_len_b / 8;
private:
    unsigned char bytes[chunk_size];

    // convoluted and slow, replace with bulk operations
    inline bool read(int x, int y) const
    {
        // unsafe
        int offset = x % 8;
        x -= offset;
        x = x / 8;
        unsigned char byte = bytes[x + y * side_len];
        return get_bit(byte, offset);
    }

    inline void write(int x, int y, bool val)
    {
        // unsafe
        int offset = x % 8;
        x = x / 8;
        unsigned char *byte = &bytes[x + y * side_len];
        set_bit(byte, offset, val);
    }

public:
    BoolChunk()
    {
        // aprox 8x faster than chars
        for (int i = 0; i < chunk_size/sizeof(long long); i++)
        {
            ((long long*)bytes)[i] = 0;
        }
        int hits = 0;
    }

    bool get(Vect2i pos) const override
    {
        // unsafe
        return read(pos.x, pos.y);
    }

    void set(Vect2i pos, bool val) override
    {
        // unsafe
        write(pos.x, pos.y, val);
    }
};

class BoolChunkLoader : public BoolGrid2D
{
private:
    mutable std::unordered_map<Vect2i, BoolChunk*> chunks;
    //std::unordered_map<int, int> chunki;
public:
    bool get(Vect2i pos) const override
    {
        Vect2i local_pos = {pos.x % BoolChunk::side_len_b, pos.y % BoolChunk::side_len_b};
        if(local_pos.x < 0)
            local_pos.x += BoolChunk::side_len_b;
        if(local_pos.y < 0)
            local_pos.y += BoolChunk::side_len_b;
        Vect2i chunk_pos = pos - local_pos;
        if(chunks.find(chunk_pos) == chunks.end())
            return 0;
        return chunks.at(chunk_pos)->get(local_pos);
    }

    void set(Vect2i pos, bool val) override
    {
        Vect2i local_pos = {pos.x % BoolChunk::side_len_b, pos.y % BoolChunk::side_len_b};
        if(local_pos.x < 0)
            local_pos.x += BoolChunk::side_len_b;
        if(local_pos.y < 0)
            local_pos.y += BoolChunk::side_len_b;
        Vect2i chunk_pos = pos - local_pos;
        auto chunk = chunks.find(chunk_pos);
        if(chunk == chunks.end())
            chunks.insert({chunk_pos, new BoolChunk()});
        return chunks.at(chunk_pos)->set(local_pos, val);
    }
    std::unordered_map<Vect2i, BoolChunk*> getChunkMap()
    {
        return chunks;
    }
};

template<class Interface, class ConcreteClass>
class Decorator : public Interface
{
    static_assert(std::is_base_of<Interface, ConcreteClass>::value, "Can not decorate a non-derivied class");
protected:
    ConcreteClass *decorated;
};

template<class Interface, class Key, class Chunk>
class ChunkLoader : public Interface
{
    static_assert(std::is_base_of<Interface, Chunk>::value, "Chunk does not conform to interface");
    static_assert(std::is_base_of<Vect, Key>::value, "Key must be defined over a vector space");
    mutable std::unordered_map<Key, Chunk> chunk_map;
};
