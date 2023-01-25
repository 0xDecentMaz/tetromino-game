#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BORDER 10
#define HEIGHT 600
#define WIDTH 420
#define BLOCK_SIZE 30

#define GAME_GRID_Y HEIGHT / BLOCK_SIZE
#define GAME_GRID_X WIDTH / BLOCK_SIZE

#define FORM_GRID_Y 3
#define FORM_GRID_X 3
#define FORM_GRID_SIZE FORM_GRID_Y * FORM_GRID_X

SDL_Window *win = NULL;
SDL_Renderer *rend = NULL;
SDL_Texture *texture = NULL;

void rotate_right();
void rotate_left();
void mirror();
void cement_grid_to_form();
void clear_grid_of_form();
void add_form_to_grid();
char new_form_grid();
char is_colliding(int y, int x);
char game_ready();
void is_row_completed();
void render();
void handle_collision();
char is_out_of_bounds(int x);
void destroy_row(int r);
void game_over();

typedef struct
{
    uint32_t color;
    char state;             // 0 empty, 1 filled, 2 active, 3 destroyed 
}
block;

uint32_t form_colors[3] = {((uint8_t)255 << 24) + ((uint8_t)255 << 16)   + ((uint8_t)0   << 8)   + (uint8_t)0,
                           ((uint8_t)255 << 24) + ((uint8_t)0   << 16)   + ((uint8_t)255 << 8)   + (uint8_t)0,
                           ((uint8_t)255 << 24) + ((uint8_t)0   << 16)   + ((uint8_t)0   << 8)   + (uint8_t)255};

uint32_t black = ((uint8_t)255 << 24) + ((uint8_t)0 << 16) + ((uint8_t)0 << 8) + (uint8_t)0;
uint32_t white = ((uint8_t)255 << 24) + ((uint8_t)255 << 16) + ((uint8_t)255 << 8) + (uint8_t)255;

block game_grid[GAME_GRID_Y][GAME_GRID_X];
char  form_grid[FORM_GRID_Y][FORM_GRID_X];        

int  form_pos_y = 5;
int  form_pos_x = 5;
char form_color = 0;

int  refresh_rate = 10;
int  game_speed   = 500;
int  clear_row_delay = 50;
uint32_t  last_tick = 0;
char running = 0;
int score = 0;



char forms[5][9] = {{0,0,0,
                     0,1,0,
                     1,1,1},

                    {1,0,0,
                     1,1,0,
                     0,1,0},

                    {0,1,0,
                     1,1,0,
                     1,0,0},

                    {0,0,0,
                     1,1,0,
                     1,1,0},

                    {0,1,0,
                     0,1,0,
                     0,1,0}};


int main(void)
{
    srand(time(NULL));

    // intitialize game grid
    for (int i = 0; i < GAME_GRID_Y; i++)
    {
        for (int j = 0; j < GAME_GRID_X; j++)
        {
            game_grid[i][j].color = black;
            game_grid[i][j].state = 0;
        }
    }

    // initialize form_grid and add to game_grid
    new_form_grid();
    add_form_to_grid();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) 
    {
        fprintf(stderr, "SDL failed to initialise: %s\n", SDL_GetError());
        return 1;
    }

    win = SDL_CreateWindow("Tetris", 
                SDL_WINDOWPOS_CENTERED, 
                SDL_WINDOWPOS_CENTERED, 
                WIDTH, HEIGHT, 
                SDL_WINDOW_SHOWN);

    rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    texture = SDL_CreateTexture(rend, 
                    SDL_PIXELFORMAT_ARGB8888, 
                    SDL_TEXTUREACCESS_STREAMING, 
                    WIDTH, 
                    HEIGHT);

    SDL_Event ev;

    while(running == 0)
    {
        // while event quee is not empty
        while (SDL_PollEvent(&ev) != 0)
        {
            if (ev.type == SDL_QUIT)
            {
                running = 1;
            }
            else if (ev.type == SDL_KEYUP)
            {
                if (ev.key.keysym.scancode == SDL_SCANCODE_E)
                {
                    rotate_right();
                    clear_grid_of_form();
                    add_form_to_grid();
                    render();
                }
                else if (ev.key.keysym.scancode == SDL_SCANCODE_Q)
                {
                    rotate_left();
                    clear_grid_of_form();
                    add_form_to_grid();
                    render();
                }
                else if (ev.key.keysym.scancode == SDL_SCANCODE_LEFT)
                {
                    if (is_out_of_bounds(form_pos_x - 1) != 0)
                    {
                        if (is_colliding(form_pos_y, form_pos_x - 1) != 0)
                        {
                            clear_grid_of_form();
                            form_pos_x -= 1;
                            add_form_to_grid();
                            render();
                        }
                    }
                }
                else if (ev.key.keysym.scancode == SDL_SCANCODE_RIGHT)
                {
                    if (is_out_of_bounds(form_pos_x + 1) != 0)
                    {
                        if (is_colliding(form_pos_y, form_pos_x + 1) != 0)
                        {
                            clear_grid_of_form();
                            form_pos_x += 1;
                            add_form_to_grid();
                            render();
                        }
                    }
                }
                else if (ev.key.keysym.scancode == SDL_SCANCODE_DOWN)
                {
                    if (is_colliding(form_pos_y + 1, form_pos_x) == 0)
                    {
                        handle_collision();
                    }
                    else
                    {
                        clear_grid_of_form();
                        form_pos_y += 1;
                        add_form_to_grid();
                        render();
                    }
                }
            }
        }


        // compare new form_grid position to game_grid for collisions
        // if collided, replace form_grid with new one
        // if not collided, clear game_grid of active tiles and add new position of form to grid
        if (game_ready() == 0)
        {
            if (is_colliding(form_pos_y + 1, form_pos_x) == 0)
            {
                // form has landed
                handle_collision();
            }
            else
            {
                // next move down is clear
                clear_grid_of_form();
                form_pos_y += 1;
                add_form_to_grid();
                render();
            }
        }

        // delay
        SDL_Delay(refresh_rate);
    }

    // ----------------------------------- release resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);

    win = NULL;
    rend = NULL;
    texture = NULL;

    SDL_Quit;
}

void is_row_completed()
{
    for (int i = GAME_GRID_Y - 1; i >= 0; i--)
    {
        // check if entire row has state > 0
        char result = 0;

        for (int j = 0; j < GAME_GRID_X; j++)
        {
            if (game_grid[i][j].state == 0)
                result = 1;
        }

        if (result == 0)
        {
            destroy_row(i);
            is_row_completed();
            break;
        }
    }
}

void destroy_row(int r)
{
    // color row white and set state 3
    for (int i = 0; i < GAME_GRID_X; i++)
    {
        game_grid[r][i].state = 3;
        game_grid[r][i].color = white;
    }

    // render
    render();

    // wait x millisec
    SDL_Delay(clear_row_delay);

    // set row values = 0 and color = black
    for (int i = 0; i < GAME_GRID_X; i++)
    {
        game_grid[r][i].state = 0;
        game_grid[r][i].color = black;
    }
    // render
    render();

    // wait 50 millisec 
    SDL_Delay(clear_row_delay);

    // move above rows down 
    for (int i = r; i > 0; i--)
    {
        int above_row = i - 1;
        if (above_row > 0)
        {
            for (int j = 0; j < GAME_GRID_X; j++)
            {
                if (game_grid[above_row][j].state == 1)
                {
                    game_grid[i][j] = game_grid[above_row][j];
                    game_grid[above_row][j].color = black;
                    game_grid[above_row][j].state = 0;
                }
            }
        }
    }

    // add points to score
    score += (1000 / game_speed);
    
    //increase game speed by decreasing the delay 
    if (game_speed > 50)
    {
        game_speed -= 10;
    }

    printf("Score: %i\nGame delay decreased, now: %i\n", score, game_speed);

    // render
    render();

}

void render()
{
    // ----------------------------------- display current grid 
        // pixels
        uint32_t *pixels;
        int pitch = WIDTH * 4;
        SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch);

        for (int i = 0; i < HEIGHT * WIDTH; i++)
        {
            int row = (i / WIDTH) / BLOCK_SIZE;
            int col = (i % WIDTH) / BLOCK_SIZE;

            // bit shift uint8_t argb values into uint32_t pixel
            *(pixels + i) = game_grid[row][col].color;
        }

        SDL_UnlockTexture(texture);
        
        SDL_RenderClear(rend);
        SDL_RenderCopy(rend, texture, NULL, NULL);
        SDL_RenderPresent(rend);
}


char game_ready()
{
    uint32_t diff = SDL_GetTicks() - last_tick;
    if (diff >= game_speed)
    {
        last_tick = SDL_GetTicks();
        return 0;
    }
    
    return 1;
}


char is_colliding(int y, int x)
{
    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            int r = y + i;
            int c = x + j;

            if (r >= GAME_GRID_Y && form_grid[i][j] == 1)
            {
                return 0;
            }
            else if (form_grid[i][j] == 1 && game_grid[r][c].state == 1)
            {
                return 0;
            }
        }
    }

    return 1;
}

char is_out_of_bounds(int x)
{
    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            if (form_grid[i][j] == 1 && (x + j) < 0)
                return 0;
            else if (form_grid[i][j] == 1 && (x + j) >= GAME_GRID_X)
                return 0;
        }
    }

    return 1;
}

void handle_collision()
{
    // form has collided and becomes a permanent part of game grid (value 1 = filled)
    cement_grid_to_form();

    // check if form has completed a row
    is_row_completed();
    
    // create a new form
    if (new_form_grid() != 0)
    {
        //could not create new form, game over
        game_over();
    }

    render();
}


char new_form_grid()
{
    int available_colors = 3;
    int available_forms = 5;

    int form_type = rand() % available_forms;
    form_color = rand() % available_colors;

    // Initialize form_grid
    int index = 0;

    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            form_grid[i][j] = forms[form_type][index];
            index++;
        }
    }

    //reset position with randomized x position
    int max = GAME_GRID_X - FORM_GRID_X;
    int min = FORM_GRID_X;

    form_pos_x = rand() % (max - min +1) + min;   
    form_pos_y = 1;

    //check that form_grid is not created on occupied location
    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            if (game_grid[form_pos_y + i][form_pos_x + j].state != 0)
            {
                return 1;
            }
        }
    }

    return 0;
}


void game_over()
{
    running = 1;
    printf("Game Over\nScore: %i\n", score);
}


void add_form_to_grid()
{
    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            int r = form_pos_y + i;
            int c = form_pos_x + j;

            if (r >= 0 && r < GAME_GRID_Y && c >= 0 && c < GAME_GRID_X && form_grid[i][j] == 1)
            {
                game_grid[r][c].color = form_colors[form_color];
                game_grid[r][c].state = 2;
            }
        }
    }
}

void clear_grid_of_form()
{
    for (int i = 0; i < GAME_GRID_Y; i++)
    {
        for (int j = 0; j < GAME_GRID_X; j++)
        {
            if (game_grid[i][j].state == 2)
            {
                game_grid[i][j].color = black;
                game_grid[i][j].state = 0;
            }
        }
    }
}

void cement_grid_to_form()
{
    for (int i = 0; i < GAME_GRID_Y; i++)
    {
        for (int j = 0; j < GAME_GRID_X; j++)
        {
            if (game_grid[i][j].state == 2)
            {
                game_grid[i][j].state = 1;
            }
        }
    }
}


void rotate_right()
{
    char temp[FORM_GRID_Y][FORM_GRID_X];

    //mirror
    mirror();

    //rotate on axis
    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            temp[i][j] = form_grid[j][i];
        }
    }

    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            form_grid[i][j] = temp[i][j];
        }
    }
}

void rotate_left()
{
    mirror();

    char temp[FORM_GRID_Y][FORM_GRID_X];

    for (int i = FORM_GRID_Y - 1, r = 0; i >= 0; i--, r++)
    {
        for (int j = FORM_GRID_X- 1, c = 0; j >= 0; j--, c++)
        {
            temp[r][c] = form_grid[j][i];
        }
    }

    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            form_grid[i][j] = temp[i][j];
        }
    }
}

void mirror()
{
    char temp[FORM_GRID_Y][FORM_GRID_X];

    for (int i = FORM_GRID_Y - 1, r = 0; i >= 0; i--, r++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            temp[r][j] = form_grid[i][j];
        }
    }

    for (int i = 0; i < FORM_GRID_Y; i++)
    {
        for (int j = 0; j < FORM_GRID_X; j++)
        {
            form_grid[i][j] = temp[i][j];
        }
    }
}