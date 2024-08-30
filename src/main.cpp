#include <SDL2/SDL.h>
#include <iostream>
#include <unistd.h>
#include "chunks.cpp"

class Offset2D : public Decorator<BoolGrid2D, BoolGrid2D>
{
    Vect2i offset;
public:
    Offset2D(BoolGrid2D *decorated, Vect2i offset) 
    {
        this->decorated = decorated;
        this->offset = offset;
    }
    inline bool get(const Vect2i &pos) const override
    {
        return decorated->get(pos+offset);
    }
    inline void set(const Vect2i &pos, bool val) override
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

void process_chunk_insides_v2(const UnpackedBoolChunk &from, UnpackedBoolChunk &result)
{
    int triect[BoolChunk::side_len_b];
    //setup, its fucked up bcs or the rolling buffer
    for(int x = 0; x < BoolChunk::side_len_b; x++)
    {
        triect[x] = from.get({x, 0}) * 2 + from.get({x, 1});
    }
    for(int y = 1; y < BoolChunk::side_len_b - 1; y++)
    {
        for(int x = 1; x < BoolChunk::side_len_b - 1; x++)
        {
            // rolling buffer
            triect[x] += from.get({x, y + 1}) - from.get({x, y - 1});
            int sum = triect[x - 1] + triect[x] + triect[x + 1];
            if(sum == 0) // if completely empty
            {
                result.set({x, y}, 0);
                continue;
            }
            sum -= from.get({x, y});
            conways_rules(result, from, sum, x, y);
        }
    }
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
    auto to_map = to.getChunkMap();
    // iterate over all chunks
    for(auto iter = map.begin(); iter != map.end(); ++iter)
    {
        const Vect2i &chunk_pos = iter->first;
        const BoolChunk &chunk = *iter->second;
        static const int max = BoolChunk::side_len_b - 1;
        UnpackedBoolChunk result;
        auto process_edge = [&](int xoffset, int yoffset, int dx, int dy, BoolGrid2D& to)
        {
            auto sum_triect = [&](int x, int y) -> int
            {
                return from.get({x - dy, y - dx}) + from.get({x, y}) + from.get({x + dy, y + dx});
            };
            int x = xoffset;
            int y = yoffset;
            int triect[3], triect_iter = 0;
            triect[1] = sum_triect(x - dx, y - dy);
            triect[2] = sum_triect(x, y);
            for (int i = 0; i < BoolChunk::side_len_b; i++)
            {   
                x = xoffset + i * dx;
                y = yoffset + i * dy;
                triect[triect_iter++] = sum_triect(x + dx, y + dy);
                if(triect_iter > 2)
                    triect_iter = 0;
                int sum = triect[0] + triect[1] + triect[2];
                //if(sum == 0 && to.get({x, y}) == 0) // skip
                //    continue;
                sum -= from.get({x,y});
                conways_rules(to, from, sum, x, y);
            }
        };
        // skip
        if(chunk.live_cells != 0)
        {
            // load/wake up neighbours
            process_edge(chunk_pos.x           , chunk_pos.y -1    , 1, 0, to); // up , 0, -1
            process_edge(chunk_pos.x           , chunk_pos.y + max + 1, 1, 0, to); // down , 0, 1
            process_edge(chunk_pos.x - 1       , chunk_pos.y       , 0, 1, to); // left , -1, 0
            process_edge(chunk_pos.x + max + 1 , chunk_pos.y       , 0, 1, to); // right , 1, 0
            // for insides
            process_chunk_insides(from.get_unpacked_chunk(chunk_pos), result);
        }
        // for inside edges
        Offset2D result_decorator(&result, {-chunk_pos.x, -chunk_pos.y}); //dumb
        process_edge(chunk_pos.x       , chunk_pos.y      , 1, 0, result_decorator); // up , 0, -1
        process_edge(chunk_pos.x       , chunk_pos.y + max, 1, 0, result_decorator); // down , 0, 1
        process_edge(chunk_pos.x       , chunk_pos.y      , 0, 1, result_decorator); // left , -1, 0
        process_edge(chunk_pos.x + max , chunk_pos.y      , 0, 1, result_decorator); // right , 1, 0
        to.set_unpacked_chunk(chunk_pos, result);
    }
}

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

void set_glider_b(BoolGrid2D &&c)
{
    // Glider
    c.set(Vect2i(1,2), 1);
    c.set(Vect2i(2,3), 1);
    c.set(Vect2i(3,1), 1);
    c.set(Vect2i(3,2), 1);
    c.set(Vect2i(3,3), 1);
}

// Gosper glider gun
//x = 36, y = 9, rule = B3/S23, but the rule is always Life so...
//24bo$22bobo$12b2o6b2o12b2o$11bo3bo4b2o12b2o$2o8bo5bo3b2o$2o8bo3bob2o4b
//obo$10bo5bo7bo$11bo3bo$12b2o!

void print_board_compact(const BoolGrid2D &c, int viewport_size)
{
    printf("\033c");

    for(int i = 0; i < viewport_size; i++)
    {
        std::cout<<"-";
    }
    std::cout<<"\n";
    for(int y = 0; y<viewport_size; y+=2)
    {
        std::cout<<"|";
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

//FIXME
void cull(BoolChunkLoader *front, BoolChunkLoader *back)
{
    auto map = front->getChunkMap();
    std::vector<Vect2i> to_delete;
    for (auto iter = map.begin(); iter != map.end(); )
    {
        if (iter->second->live_cells == 0)
            to_delete.push_back(iter->first);
        ++iter;
    }
    while(!to_delete.empty())
    {
        front->kill(to_delete.back());
        back->kill(to_delete.back());
        to_delete.pop_back();
    }
}

BoolChunkLoader* run_simulation(
    BoolChunkLoader* start,
    float tick_delay = 0.5,
    int simulation_len = -1,
    bool graphics = true,
    int viewport_size = 64,
    Vect2i viewport_offset = Vect2i(-32, -32),
    bool manual = false
    )
{
    BoolChunkLoader *front = start, *back = new BoolChunkLoader;
    for(int i = 0; i != simulation_len; i++)
    {
        if(graphics)
            print_board_compact(Offset2D(front, viewport_offset), viewport_size);
        else if(i % 10 == 0)
            std::cout<<"Generation, chunks: "<< i << ", " << back->getChunkMap().size() <<'\n';
        if(manual)
            std::cin.ignore(9999, '\n');
        tick_optimized(*front, *back);
        BoolChunkLoader *swap = front;
        front = back;
        back = swap;
        cull(front, back);
        if(tick_delay && !manual)
            usleep(tick_delay * (1<<20));
    }
    delete back;
    return front;
}

// Very easy to verify processing integrity
void set_vertical_pattern(BoolGrid2D&& chunk) {
    // Create a pattern of vertical lines: live line, two empty lines, repeating
    for (int x = -10; x < 100; x += 3) {
        for (int y = -10; y < 100; ++y) {
            // Set a vertical line at every 3rd x position
            chunk.set({x, y}, true);
            chunk.set({x + 1, y}, false);
            chunk.set({x + 2, y}, false);
        }
    }
}

int main(int argc, char **argv)
{
    BoolChunkLoader* start = new BoolChunkLoader;
    //set_glider(Offset2D(start, {0, 0}));
    //set_vertical_pattern(Offset2D(start, {0, 0}));
    set_acorn(Offset2D(start, {0, 0}));
    print_board_compact(Offset2D(start, {0,0}), 64);
    usleep(1 * (1<<20));
    auto result = run_simulation(start, 0, 100000, 0, 64, {0, 0}, 0);
    print_board_compact(Offset2D(result, {0, 0}), 64);
    auto map = result->getChunkMap();
    int live_cnt = 0;
    int i = 0;
    for(auto iter = map.begin(); iter != map.end(); ++iter)
    {
        print_board_compact(*iter->second, 32);
        std::cout<<"N: " << i++ << ", dead?: " << iter->second->live_cells <<'\n';
        usleep(0.2 * (1<<20));
    }
    print_board_compact(Offset2D(result, {0, 0}), 64);
    for(auto iter = map.begin(); iter != map.end(); ++iter)
        live_cnt += iter->second->live_cells;
    std::cout << "Final live count: " << live_cnt << '\n';
    std::cout << "Final num chunks: " << map.size() << '\n';
    live_cnt = 0;
    for(auto iter = map.begin(); iter != map.end(); ++iter)
        live_cnt += iter->second->live_cells ? 0 : 1;
    std::cout << "Final num dead chunks: " << live_cnt << '\n';
    live_cnt = 0;
    for(auto iter = map.begin(); iter != map.end(); ++iter)
    {
        live_cnt += iter->second->live_cells > 10 ? 1 : 0;
    }
    std::cout << "Final num simple chunks: " << live_cnt << '\n';
    return 0;
}

// Unrelated: whs: 29y not write in C?
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