#include <SDL2/SDL.h>
#include <iostream>
#include <unistd.h>
#include "chunks.cpp"

int sum_neighbours(const BoolGrid2D &c, int x, int y)
{
    int sum = 0;
    for(int i = x-1; i<=x+1;i++)
    {
        for(int j = y-1; j<=y+1; j++)
        {
            sum += c.get(Vect2i(i, j)); 
        }
    }
    sum -= c.get(Vect2i(x, y));
    return sum;
}

inline bool evaluate_cell(BoolChunkLoader &from, int i, int j);

void tick_optimized(BoolChunkLoader &from, BoolChunkLoader &to)
{
    auto map = from.getChunkMap();
    for(auto iter = map.begin(); iter != map.end(); ++iter)
    {
    for(int i = iter->first.x; i <= iter->second->side_len_b; i++)
    {
        bool skip = 0;
        for(int j = iter->first.y; j <= iter->second->side_len_b; j++)
        {
            int sum = sum_neighbours(from, i, j);
            if(sum == 0)
            {
                j++;
                skip = 1;
                continue;
            }
            if(sum == 2)
                to.set({i, j}, 0);
            else if (sum == 2)
                to.set({i, j}, from.get({i, j}));
            else if(sum == 3)
                to.set({i, j}, 1);
            else //if(sum > 3)
                to.set({i, j}, 0);
            if(!skip)
                continue;
            j--;
            sum = sum_neighbours(from, i, j);
            if(sum == 2)
                to.set({i, j}, 0);
            else if (sum == 2)
                to.set({i, j}, from.get({i, j}));
            else if(sum == 3)
                to.set({i, j}, 1);
            else //if(sum > 3)
                to.set({i, j}, 0);
            j++;
            }
        }
    }
}


void tick(BoolChunkLoader &from, BoolChunkLoader &to)
{
    auto map = from.getChunkMap();
    for(auto iter = map.begin(); iter != map.end(); ++iter)
    for(int i = iter->first.x; i < iter->second->side_len_b; i++)
    {
        for(int j = iter->first.y; j < iter->second->side_len_b; j++)
        {
            int sum = sum_neighbours(from, i, j);
            if (sum == 0)
            {
                to.set({i, j}, 0);
                continue;
            }
            if (sum == 2)
                to.set({i, j}, from.get({i, j}));
            else if (sum == 3)
                to.set({i, j}, 1);
            else 
                to.set({i, j}, 0);
        }
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
    c.set(Vect2i(10,12), 1);
    c.set(Vect2i(11,14), 1);
    c.set(Vect2i(12,11), 1);
    c.set(Vect2i(12,12), 1);
    c.set(Vect2i(12,15), 1);
    c.set(Vect2i(12,16), 1);
    c.set(Vect2i(12,17), 1);
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
    for(int y = -(viewport_size/2); y<viewport_size/2; y+=2)
    {
        for(int x = -(viewport_size/2); x<viewport_size/2; x++)
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

void print_board(const BoolChunk &c)
{
    printf("\033c");
    for(int y = 0; y<c.side_len_b; y++)
    {
        for(int x = 0; x<c.side_len_b; x++)
        {
            std::cout<<(int)c.get({x,y});
        }
        std::cout<<"\n";
    }
    std::cout<<"\n";
}

void run_simulation(BoolChunkLoader start, int viewport_size, int simulation_len,  float tick_rate)
{
    BoolChunkLoader *front = new BoolChunkLoader(start), *back = new BoolChunkLoader;
    for(int i = 0; i != simulation_len; i++)
    {
        print_board_compact(*front, viewport_size);
        tick(*front, *back);
        BoolChunkLoader *swap = front;
        front = back;
        back = swap;
        usleep(tick_rate * (1<<20));
    }
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
class Offset2D : public Decorator<BoolGrid2D, BoolChunkLoader>
{
    Vect2i offset;
public:
    Offset2D(BoolChunkLoader *decorated, Vect2i offset) 
    {
        this->decorated = decorated;
        this->offset = offset;
    }
    bool get(Vect2i pos) const
    {
        return decorated->get(pos+offset);
    }
    void set(Vect2i pos, bool val)
    {
        decorated->set(pos+offset, val);
    }
};

int main(int argc, char **argv)
{
    BoolChunkLoader start;
    set_acorn(Offset2D(&start, {0, 0}));
    print_board_compact(start, 50);
    run_simulation(start, 50, 100, 0);
    return 0;
}

// Unrelated: why not write in C?
// it has strict types, strict syntax, just write endpoints here
