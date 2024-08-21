#include <SDL2/SDL.h>
#include <iostream>
#include <unistd.h>
#include "chunks.cpp"

class Offset2D : public Decorator<BoolGrid2D, BoolChunkLoader>
{
    Vect2i offset;
public:
    Offset2D(BoolChunkLoader *decorated, Vect2i offset) 
    {
        this->decorated = decorated;
        this->offset = offset;
    }
    bool get(const Vect2i &pos) const override
    {
        return decorated->get(pos+offset);
    }
    void set(const Vect2i &pos, bool val) override
    {
        decorated->set(pos+offset, val);
    }
};

void print_board_compact(const BoolGrid2D&, int);

inline int sum_neighbours(const BoolGrid2D &c, const int x, const int y)
{
    int sum = 0;
    for(int i = x-1; i<=x+1; i++)
    {
        for(int j = y-1; j<=y+1; j++)
        {
            sum += c.get({i, j}); 
        }
    }
    sum -= c.get({x, y});
    return sum;
}

inline int sum_triect(const BoolGrid2D &c, const int x, const int y)
{
    return c.get({x, y - 1}) + c.get({x, y}) + c.get({x, y + 1});
}

inline void conways_rules(BoolGrid2D &to, const BoolGrid2D &from, const int sum, const int x, const int y)
{
    if(sum < 2)
        to.set({x, y}, 0);
    else if (sum == 2)
        to.set({x, y}, from.get({x, y}));
    else if(sum == 3)
        to.set({x, y}, 1);
    else //if(sum > 3)
        to.set({x, y}, 0);
}

// Iteration over chunk insides, thus removing cross-chunk reads (cache miss)
void process_chunk_insides(const UnpackedBoolChunk &from, UnpackedBoolChunk &result)
{
    for(int y = 1; y < BoolChunk::side_len_b - 1; y++)
    {
        // setup
        auto sum_triect = [&](int x, int y) -> int
        {
            return from.get({x, y - 1}) + from.get({x, y}) + from.get({x, y + 1});
        };
        int triect[3], triect_iter = 0;
        triect[1] = sum_triect(0, y);
        triect[2] = sum_triect(1, y);
        for(int x = 1; x < BoolChunk::side_len_b - 1; x++)
        {
            // rolling buffer
            triect[triect_iter++] = sum_triect(x + 1, y);
            if(triect_iter > 2)
                triect_iter = 0;
            int sum = triect[0] + triect[1] + triect[2];
            if(sum == 0) // if completely empty
            {
                if(result.get({x, y}) == 1)
                    result.set({x, y}, 0);
                continue;
            }
            sum -= from.get({x, y});
            conways_rules(result, from, sum, x, y);
        }
    }
}

// stores sum of 3 cells in a rolling buffer, re-using read data 2/3rds of the time
// 1010
// 0011 
// 0100
void print_board_compact(const BoolGrid2D &c, int viewport_size);

void tick_optimized(BoolChunkLoader &from, BoolChunkLoader &to)
{
    auto map = from.getChunkMap();
    // iterate over all chunks
    for(auto iter = map.begin(); iter != map.end(); ++iter)
    {
        const Vect2i &chunk_pos = iter->first;
        // for insides
        UnpackedBoolChunk result;
        process_chunk_insides(from.get_unpacked_chunk(chunk_pos), result);
        to.set_unpacked_chunk(chunk_pos, result);
        // for edges + loading
        auto process_edge = [&](int xoffset, int yoffset, int dx, int dy)
        {
            auto sum_triect = [&](int x, int y) -> int
            {
                return from.get({x - dy, y - dx}) + from.get({x, y}) + from.get({x + dy, y + dx});
            };
            int x = chunk_pos.x + xoffset;
            int y = chunk_pos.y + yoffset;
            int triect[3], triect_iter = 0;
            triect[1] = sum_triect(x - dx, y - dy);
            triect[2] = sum_triect(x, y);
            for (int i = 0; i < BoolChunk::side_len_b; i++)
            {
                x = chunk_pos.x + xoffset + i * dx;
                y = chunk_pos.y + yoffset + i * dy;
                triect[triect_iter++] = sum_triect(x + dx, y + dy);
                if(triect_iter > 2)
                    triect_iter = 0;
                int sum = triect[0] + triect[1] + triect[2];
                sum -= from.get({x,y});
                conways_rules(to, from, sum, x, y);
            }
        };
        process_edge(0, 0, 1, 0); // up , 0, -1
        process_edge(0, BoolChunk::side_len_b - 1, 1, 0); // down , 0, 1
        process_edge(0, 0, 0, 1); // left , -1, 0
        process_edge(BoolChunk::side_len_b - 1, 0, 0, 1); // right , 1, 0
        // for unloaded:
        process_edge(0, -1, 1, 0); // up , 0, -1
        process_edge(0, BoolChunk::side_len_b, 1, 0); // down , 0, 1
        process_edge(-1, 0, 0, 1); // left , -1, 0
        process_edge(BoolChunk::side_len_b, 0, 0, 1); // right , 1, 0
    }
}


/**
 * @brief std::vector<Edge> findEdgesBetweenLiveAndUnaliveCells(const std::unordered_map<Vect2Dint, bool>& liveCells) {
    std::vector<Edge> edges;
    
    // Define the 4 possible neighbors (left, right, top, bottom)
    std::vector<Vect2Dint> neighbors = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    // Iterate over all live cells
    for (const auto& cellPair : liveCells) {
        const Vect2Dint& liveCell = cellPair.first;
        
        // Check all 4 neighbors
        for (const Vect2Dint& offset : neighbors) {
            Vect2Dint neighborCell = {liveCell.x + offset.x, liveCell.y + offset.y};
            
            // If neighbor cell is not in the liveCells map, it is an unalive cell
            if (liveCells.find(neighborCell) == liveCells.end()) {
                edges.push_back({liveCell, neighborCell});
            }
        }
    }
    
    return edges;
}
 * 
 */

    // Diehard OLD
    //arr[front][11][13] = 1;
    //arr[front][12][13] = 1;
    //arr[front][12][14] = 1;
    //arr[front][16][14] = 1;
    //arr[front][17][14] = 1;
    //arr[front][18][14] = 1;
    //arr[front][17][12] = 1;
    // TODO easier seed inpt, file or text or mouse
    // Die harder
    //arr[front][16][16] = 1;
    //arr[front][17][16] = 1;
    //arr[front][17][17] = 1;
    //arr[front][21][17] = 1;
    //arr[front][22][17] = 1;
    //arr[front][23][17] = 1;
    //arr[front][22][15] = 1;

void set_acorn(BoolGrid2D &&c)
{
    // Acorn
    c.set(Vect2i(0,2), 1);
    c.set(Vect2i(1,4), 1);
    c.set(Vect2i(2,1), 1);
    c.set(Vect2i(2,2), 1);
    c.set(Vect2i(2,5), 1);
    c.set(Vect2i(2,6), 1);
    c.set(Vect2i(2,7), 1);
}

void set_acorn(BoolGrid2D &c)
{
    // Acorn
    c.set(Vect2i(0,2), 1);
    c.set(Vect2i(1,4), 1);
    c.set(Vect2i(2,1), 1);
    c.set(Vect2i(2,2), 1);
    c.set(Vect2i(2,5), 1);
    c.set(Vect2i(2,6), 1);
    c.set(Vect2i(2,7), 1);
}

void set_glider(BoolGrid2D &&c)
{
    // Glider
    c.set(Vect2i(2,1), 1);
    c.set(Vect2i(3,2), 1);
    c.set(Vect2i(1,3), 1);
    c.set(Vect2i(2,3), 1);
    c.set(Vect2i(3,3), 1);
}

void print_board_compact(const BoolGrid2D &c, int viewport_size)
{
    printf("\033c");
    for(int y = 0; y<viewport_size; y+=2)
    {
        for(int x = 0; x<viewport_size; x++)
        {
            int opcode = c.get({x,y})+c.get({x,y+1})*2;
            switch (opcode)
            {
            case 0:
                std::cout<<' ';
                break;
            case 1:
                std::cout<<'\'';
                break;
            case 2:
                std::cout<<'.';
                break;
            case 3:
                std::cout<<':';
                break;
            default:
                std::cout<<'X';
            }
        }
        std::cout<<"|\n";
    }
    for(int i = 0; i < viewport_size; i++)
    {
        std::cout<<"-";
    }
    std::cout<<"\n";
}


// find way to have infinite growth (ex: view + mem chunk loader)
// a custom chunk loader sounds funny
// tldr on touching a cell, a bit in a big bit array
// must be touched. touching it intercepts and invokes a mem load
// if the bit was 0 (unloaded chunk)
// assume a page is 100% used so only need pages
// 4096B = (64*2)^2*2b, aka 128x128 double bit masks.
// optimization could be: as processing is single thread,
// just use one static page for these things (note, border includes neighbor loading...)

BoolChunkLoader* run_simulation(
    BoolChunkLoader start,
    float tick_delay = 0.5,
    int simulation_len = -1,
    bool graphics = true,
    int viewport_size = 64,
    Vect2i viewport_offset = Vect2i(-32, -32),
    bool manual = false
    )
{
    BoolChunkLoader *front = new BoolChunkLoader(start), *back = new BoolChunkLoader;
    for(int i = 0; i != simulation_len; i++)
    {
        if(graphics)
            print_board_compact(Offset2D(front, viewport_offset), viewport_size);
        if(manual)
            std::cin.ignore(9999, '\n');
        tick_optimized(*front, *back);
        BoolChunkLoader *swap = front;
        front = back;
        back = swap;
        if(tick_delay && !manual)
            usleep(tick_delay * (1<<20));
    }
    delete front;
    return back;
}

void set_glider_b(BoolGrid2D &&c)
{
    // Glider
    c.set(Vect2i(1,2), 1);
    c.set(Vect2i(2,3), 1);
    c.set(Vect2i(3,1), 1);
    c.set(Vect2i(3,2), 1);
    c.set(Vect2i(3,3), 1);
}

#include <cassert> // For assertions

// Assuming ChunkLoader, UnpackedBoolChunk, and related methods exist and are defined properly

void test_chunk_operations(BoolChunkLoader& from, BoolChunkLoader& to, const Vect2i& chunk_pos) {
    // Create an UnpackedBoolChunk to work with
    UnpackedBoolChunk result;
    
    // Set all cells in the `from` chunk
    for (int x = 0; x < 64; ++x) {
        for (int y = 0; y < 64; ++y) {
            bool value = (x + y) % 2 == 0; // Alternate true/false for test
            from.set({x, y}, value);
        }
    }

    // Verify that all cells are set correctly in the `from` chunk
    for (int x = 0; x < 64; ++x) {
        for (int y = 0; y < 64; ++y) {
            bool expected_value = (x + y) % 2 == 0;
            bool actual_value = from.get({x, y});
            assert(actual_value == expected_value && "from.get({x, y}) returned incorrect value");
        }
    }

    std::cout << "All cells set and retrieved correctly in `from` chunk.\n";

    print_board_compact(from, 64);
    print_board_compact(from.get_unpacked_chunk(chunk_pos), 64);

    // Use process_chunk_insides (assuming it's defined correctly)
    process_chunk_insides(from.get_unpacked_chunk(chunk_pos), result);

    print_board_compact(result, 64);
    // Set the unpacked chunk into the `to` chunk
    to.set_unpacked_chunk(chunk_pos, result);

    // Retrieve the unpacked chunk back from the `to` chunk
    UnpackedBoolChunk retrieved_result = to.get_unpacked_chunk(chunk_pos);

    // Verify that the retrieved chunk matches the original
    for (int x = 0; x < 64; ++x) {
        for (int y = 0; y < 64; ++y) {
            bool expected_value = result.get({x, y});
            bool actual_value = retrieved_result.get({x, y});
            assert(actual_value == expected_value && "Mismatch in set and retrieved unpacked chunk values");
        }
    }

    std::cout << "All cells retrieved correctly from `to` chunk.\n";
}

// Very easy to verify processing integrity
void set_vertical_pattern(BoolGrid2D&& chunk) {
    // Create a pattern of vertical lines: live line, two empty lines, repeating
    for (int x = 0; x < 60; x += 3) {
        for (int y = 0; y < 64; ++y) {
            // Set a vertical line at every 3rd x position
            chunk.set({x, y}, true);
            chunk.set({x + 1, y}, false);
            chunk.set({x + 2, y}, false);
        }
    }
}

int main(int argc, char **argv)
{
    BoolChunkLoader start, from, to;
    //set_glider(Offset2D(&start, {-16, -16}));
    //set_vertical_pattern(Offset2D(&start, {-32, -32}));
    //start.set({-10, -10}, 1);
    //start.set({-10, 0}, 1);
    //start.set({0, -10}, 1);
    set_acorn(Offset2D(&start, {0, 0}));
    auto result = run_simulation(start, 0, 200, false, 64, {-32, -32}, false);
    print_board_compact(Offset2D(result, {-32,-32}), 64);
    return 0;
}

// Unrelated: why not write in C?
// it has strict types, strict syntax, just write endpoints here

/* Pipeline:
For each chunk:
    Unpack insides into more processable data
    Process in triacts
    Re-pack insides
    Save

For each chunk:
    Process edges (slow)

For unbound chunk detection:
    Review all unbound chunk edges for processing
    Select edges for processing:
        Load chunks where needed

*/