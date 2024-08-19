#include "vects.cpp"
#include <stdlib.h>
#include <unordered_map>

inline void set_bit(unsigned char &byte, int offset, bool val) 
{
    // Helper
    if(val)
        byte = byte | (1 << offset);
    else
        byte = byte & ~(1 << offset);
}

inline bool get_bit(const unsigned char &byte, int offset)
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
    virtual bool get(const Vect2i &pos) const = 0;
    virtual void set(const Vect2i &pos, bool val) = 0;
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

    bool get(const Vect2i &pos) const override
    {
        // unsafe
        // math is y rows * bytes per row (side_len) + x/8 bytes + x%8 bits
        const unsigned char *byte = &bytes[(pos.x >> 3) + pos.y * side_len];
        return get_bit(*byte, pos.x & (8 - 1));
    }

    void set(const Vect2i &pos, bool val) override
    {
        // unsafe
        // math is y rows * bytes per row (side_len) + x/8 bytes + x%8 bits
        unsigned char *byte = &bytes[(pos.x >> 3) + pos.y * side_len];
        return set_bit(*byte, pos.x & (8 - 1), val);
    }
};

class BoolChunkLoader : public BoolGrid2D
{
private:
    mutable std::unordered_map<Vect2i, BoolChunk*> chunks;
    mutable Vect2i hot_pos;
    mutable BoolChunk* hot_pointer;
public:
    BoolChunkLoader()
    {
        hot_pointer = new BoolChunk();
        chunks.insert({{0,0},hot_pointer});
    }

    bool get(const Vect2i &pos) const override
    {
        Vect2i local_pos = {pos.x % BoolChunk::side_len_b, pos.y % BoolChunk::side_len_b};
        if(local_pos.x < 0)
            local_pos.x += BoolChunk::side_len_b;
        if(local_pos.y < 0)
            local_pos.y += BoolChunk::side_len_b;
        Vect2i chunk_pos = pos - local_pos;
        if(chunk_pos == hot_pos)
            return hot_pointer->get(local_pos);
        BoolChunk* chunk;
        auto querry = chunks.find(chunk_pos);
        if (querry == chunks.end())
            return 0;
        else
        {
            hot_pos = querry->first;
            hot_pointer = querry->second;
            return querry->second->get(local_pos);
        }
    }

    void set(const Vect2i &pos, bool val) override
    {
        Vect2i local_pos = {pos.x % BoolChunk::side_len_b, pos.y % BoolChunk::side_len_b};
        if(local_pos.x < 0)
            local_pos.x += BoolChunk::side_len_b;
        if(local_pos.y < 0)
            local_pos.y += BoolChunk::side_len_b;
        Vect2i chunk_pos = pos - local_pos;
        if(chunk_pos == hot_pos)
        {
            hot_pointer->set(local_pos, val);
            return;
        }
        BoolChunk* chunk;
        auto querry = chunks.find(chunk_pos);
        if (querry == chunks.end())
        {
            chunk = new BoolChunk();
            chunks.insert({chunk_pos, chunk});
        }
        else
            chunk = querry->second;
        chunk->set(local_pos, val);
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
