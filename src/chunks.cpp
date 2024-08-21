#include "vects.cpp"
#include <stdlib.h>
#include <unordered_map>
#include <cstring>

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

public:
    unsigned char bytes[chunk_size];

    BoolChunk()
    {
        // wipe with 0s
        memset(bytes, 0, sizeof(bytes));
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

class UnpackedBoolChunk : public BoolGrid2D
{
public:
    static const int side_len = BoolChunk::side_len_b;

private:
    bool cells[side_len][side_len];

public:
    UnpackedBoolChunk()
    {
        // wipe with 0s
        memset(cells, 0, sizeof(cells));
    }

    bool get(const Vect2i &pos) const override
    {
        return cells[pos.x][pos.y];
    }

    void set(const Vect2i &pos, bool val) override
    {
        cells[pos.x][pos.y] = val;
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

    // Very slow, use bulk instead
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

    // Very slow, use bulk instead
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
            if(val == 0) // lazy loading not broken by set(0)
                return;
            chunk = new BoolChunk();
            chunks.insert({chunk_pos, chunk});
        }
        else
            chunk = querry->second;
        chunk->set(local_pos, val);
    }

    UnpackedBoolChunk get_unpacked_chunk(const Vect2i &chunk_pos) const
    {
        auto querry = chunks.find(chunk_pos);
        if (querry == chunks.end())
            throw std::exception(); // Cant unpack non-existant chunk
        UnpackedBoolChunk rval;
        const BoolChunk& chunk = *querry->second;
        for(int y = 0; y < BoolChunk::side_len_b; y++)
        {
            for(int x = 0; x < BoolChunk::side_len; x++)
            {
                unsigned char byte = chunk.bytes[x + y * BoolChunk::side_len];
                for(int i = 0; i < 8; i++)
                {
                    rval.set({x*8+i,y},byte & 1);
                    byte = byte >> 1;
                }
            }
        }
        return rval;
    }

    void set_unpacked_chunk(const Vect2i &chunk_pos, const UnpackedBoolChunk &uchunk)
    {
        auto querry = chunks.find(chunk_pos);
        BoolChunk* chunk_p;
        if (querry == chunks.end())
        {
            chunk_p = new BoolChunk();
            chunks.insert({chunk_pos, chunk_p});
        }
        else
            chunk_p = querry->second;
        BoolChunk& chunk = *chunk_p;
        for(int y = 0; y < BoolChunk::side_len_b; y++)
        {
            for(int x = 0; x < BoolChunk::side_len; x++)
            {
                unsigned char byte = 0;
                for(int i = 0; i < 8; i++)
                {
                    byte |= uchunk.get({x*8+i, y}) << i; // endiannes matters
                }
                chunk.bytes[x + y * BoolChunk::side_len] = byte;
            }
        }
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
