#include "raylib.h"
#include "resource_dir.h"	
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <cjson/cJSON.h>

#define WIDTH 30 // width of map
#define HEIGHT 16 // height of map
#define MINES 99
#define TILE_SIZE 16.0 // tile size in pixels

typedef struct {
    int SCREEN_WIDTH;
    int SCREEN_HEIGHT;
    float scale;
    int menu;
    int flags;
    double start_time;
    double final_time;
    double pause_time_buffer;
    int dmap[HEIGHT][WIDTH];
    int vmap[HEIGHT][WIDTH];
    int BBBV;
} Gamestate;


int in_bounds(int row, int col) {return (row >= 0 && row < HEIGHT && col >= 0 && col < WIDTH);}

/* map key
 * 0   = empty tile
 * 1-8 = safe tiles with neighboring mines
 * 9   = flagged tile
 * 10  = covered tile // does not exist in dmap
 * 11  = uncovered mine! // no distinction between covered and uncovered mines in dmap
*/

void initializeMap(Gamestate *gamestate);
void displayMap(Gamestate *gamestate, Texture tiles[13]);
void click(Gamestate *gamestate, int row, int col ,int state); 
void openMap(Gamestate *gamestate, int row, int col);
void displayMenu(Gamestate *gamestate, int pause);
int checkWin(Gamestate *gamestate);
int calculateBBBV(Gamestate *gamestate);

int main ()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(0, 0, "Minesweeper");
    SearchAndSetResourceDir("resources");
    SetTargetFPS(60);

    FILE *times;
    FILE *highscores;
    times = fopen("times.json", "a");
    highscores = fopen("highscores.json", "a+");
    if(times == NULL || highscores == NULL) {
        printf("times or highschores file didn't open\n");
        CloseWindow();
    }

    Gamestate gamestate;
    gamestate.flags             = 0;
    gamestate.menu              = 0;
    gamestate.start_time        = 0;
    gamestate.final_time        = 0;
    gamestate.BBBV              = 0;
    gamestate.pause_time_buffer = 0;
    gamestate.SCREEN_WIDTH  = GetRenderWidth(); 
    gamestate.SCREEN_HEIGHT = GetRenderHeight();
    gamestate.scale = WIDTH > HEIGHT ? gamestate.SCREEN_WIDTH / (WIDTH * TILE_SIZE) : gamestate.SCREEN_HEIGHT / (HEIGHT * TILE_SIZE);

    Texture tiles[13];
    tiles[0]   = LoadTexture("uncovered_tile.png");
    tiles[1]   = LoadTexture("one.png");
    tiles[2]   = LoadTexture("two.png");
    tiles[3]   = LoadTexture("three.png");
    tiles[4]   = LoadTexture("four.png");
    tiles[5]   = LoadTexture("five.png");
    tiles[6]   = LoadTexture("six.png");
    tiles[7]   = LoadTexture("seven.png");
    tiles[8]   = LoadTexture("eight.png");
    tiles[9]   = LoadTexture("flagged_tile.png");
    tiles[10]  = LoadTexture("covered_tile.png");
    tiles[11]  = LoadTexture("uncovered_mine.png");
    tiles[12]  = LoadTexture("incorrect_flagged_tile.png");

    Font default_font = GetFontDefault();
    srand(time(NULL));

    // dmap = data map
    // vmap = visual map
    // menu 0 = main menu
    // menu 1 = game
    // menu 2 = victory screen
    // menu 3 = paused game
    // honestly probably not the best way to make the victory screen but i do not care

	while (!WindowShouldClose())		
	{
        switch(gamestate.menu) {
            case 0:
                BeginDrawing();
                ClearBackground(BLACK);
                DrawText("Left Click to Start!", gamestate.SCREEN_WIDTH / 2, gamestate.SCREEN_HEIGHT / 2, 20 * gamestate.scale, WHITE);

                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    initializeMap(&gamestate);
                    gamestate.start_time = GetTime();
                    gamestate.menu = 1;
                }
                
                EndDrawing();
                break;
            case 1:
                BeginDrawing();
		        ClearBackground(BLACK);
                displayMap(&gamestate, tiles);     
                displayMenu(&gamestate, 0);
		
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    int row = (int)(GetMouseY() / (16.0 * gamestate.scale));
                    int col = (int)(GetMouseX() / (16.0 * gamestate.scale));
                    click(&gamestate, row, col, 0);
                    if(checkWin(&gamestate)) {
                        gamestate.menu = 2;
                        gamestate.final_time = GetTime() - gamestate.start_time;

                        time_t current_time;
                        time(&current_time);
                        cJSON *json_time = cJSON_CreateObject();
                        cJSON_AddNumberToObject(json_time, "Width", WIDTH);
                        cJSON_AddNumberToObject(json_time, "Height", HEIGHT);
                        cJSON_AddNumberToObject(json_time, "Mines", MINES);
                        cJSON_AddNumberToObject(json_time, "Time", gamestate.final_time);
                        cJSON_AddNumberToObject(json_time, "3BV", gamestate.BBBV);
                        cJSON_AddNumberToObject(json_time, "3BV/s", gamestate.BBBV / gamestate.final_time);
                        cJSON_AddStringToObject(json_time, "Date", ctime(&current_time));
                        char *json_time_str = cJSON_Print(json_time);
                        fputs(json_time_str, times);
                        cJSON_free(json_time_str);
                        cJSON_Delete(json_time);

                        cJSON *json_highscore = cJSON_CreateObject();

                        /*
                        char highscore[256];
                        char buffer[256];
                        while(fgets(buffer, sizeof(buffer), highscores)) {
                            strcpy(highscore, buffer);
                        }

                        printf("%s\n", highscore);
                        for(int i = 0; i < sizeof(highscore)/sizeof(highscore[0]); i++) {
                            printf("%c\n", highscore[i]);
                        }*/

                    }
                } else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    int row = (int)(GetMouseY() / (16.0 * gamestate.scale));
                    int col = (int)(GetMouseX() / (16.0 * gamestate.scale));
                    click(&gamestate, row, col, 1);
                }

                if(IsKeyPressed(KEY_R)) {
                    initializeMap(&gamestate);
                    gamestate.menu = 0;
                }

                if(IsKeyPressed(KEY_P)) {
                    gamestate.menu = 3;
                    gamestate.pause_time_buffer = GetTime() - gamestate.start_time;
                }

		        EndDrawing();
                break;
            case 2:
                BeginDrawing();
                ClearBackground(BLACK);
                displayMap(&gamestate, tiles);
                displayMenu(&gamestate, 1);   

                // x, y, width, hieght, color
                DrawRectangle(gamestate.SCREEN_WIDTH / 2 - 50 * gamestate.scale, gamestate.SCREEN_HEIGHT / 2 - 10 * gamestate.scale, 100 * gamestate.scale, 20 * gamestate.scale, BLACK);
                // text, x, y, font size, color
                DrawText("VICTORY!", gamestate.SCREEN_WIDTH / 2 - 50 * gamestate.scale, gamestate.SCREEN_HEIGHT / 2 - 10 * gamestate.scale, 20 * gamestate.scale, WHITE); 

                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))  {
                    gamestate.menu = 1;
                    initializeMap(&gamestate);
                    gamestate.start_time = GetTime();
                }

                EndDrawing();
                break;
            case 3:
                BeginDrawing();
                ClearBackground(BLACK);
                displayMap(&gamestate, tiles);
                displayMenu(&gamestate, 0);

                gamestate.start_time = GetTime() - gamestate.pause_time_buffer;
                if(IsKeyPressed(KEY_P) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    gamestate.menu = 1;
                 
                    int row = (int)(GetMouseY() / (16.0 * gamestate.scale));
                    int col = (int)(GetMouseX() / (16.0 * gamestate.scale));
                    click(&gamestate, row, col, 0);
                }
                
                EndDrawing();
                break;
            case 4:
                // i know this definitely isnt a good way to do this, but i really dont care.
                // just gonna make it work now and refactor later
                BeginDrawing();
                ClearBackground(BLACK);
                displayMap(&gamestate, tiles);
                displayMenu(&gamestate, 0);

                gamestate.start_time = GetTime() - gamestate.pause_time_buffer;
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    gamestate.flags = 0;
                    initializeMap(&gamestate);
                    gamestate.start_time = GetTime();
                    gamestate.pause_time_buffer = 0;
                    gamestate.menu = 3;
                }
                EndDrawing();
                break;
        }
		
	}

    for(int i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
        UnloadTexture(tiles[i]);
    }
    UnloadFont(default_font);
    fclose(times);
    fclose(highscores);


    CloseWindow();
	return 0;
}

void displayMenu(Gamestate *gamestate, int pause) {   
    DrawRectangle(0, HEIGHT * TILE_SIZE * gamestate->scale, gamestate->SCREEN_WIDTH, gamestate->SCREEN_HEIGHT - HEIGHT * TILE_SIZE * gamestate->scale, BLACK);
    char str_flags[32];
    char str_time[32];
    char str_BBBV[32];
    sprintf(str_flags, "Mines Flagged: %d", MINES - gamestate->flags);
    sprintf(str_BBBV, "3BV: %d", gamestate->BBBV);
    if(pause) {
        sprintf(str_time, "Time: %d", (int)(gamestate->final_time));
    } else {
        sprintf(str_time, "Time: %d", (int)(GetTime() - gamestate->start_time));
    }
    DrawText(str_flags, 0, HEIGHT * TILE_SIZE * gamestate->scale, 10 * gamestate->scale, WHITE);
    DrawText(str_time, 100 * gamestate->scale, HEIGHT * TILE_SIZE * gamestate->scale, 10 * gamestate->scale, WHITE);
    DrawText(str_BBBV, 150 * gamestate->scale, HEIGHT * TILE_SIZE * gamestate->scale, 10 * gamestate->scale, WHITE);
}


// left click = state 0
// right click = state 1
void click(Gamestate *gamestate, int row, int col, int state) {
    if(state == 1) {        
        if(gamestate->vmap[row][col] == 10) {
            gamestate->vmap[row][col] = 9;
            gamestate->flags++;
        } else if(gamestate->vmap[row][col] == 9) {
            gamestate->vmap[row][col] = 10;
            gamestate->flags--;
        }
        return;
    }

    if(gamestate->vmap[row][col] == 10) {
        if(gamestate->dmap[row][col] == 0) {
            openMap(gamestate, row, col);
        } else if(gamestate->dmap[row][col] < 9) {
            gamestate->vmap[row][col] = gamestate->dmap[row][col];
        } else if(gamestate->dmap[row][col] == 11 && gamestate->vmap[row][col] == 10) {
            // reveals all unfound mines and ends game
            for(int i = 0; i < HEIGHT; i++) {
                for(int j = 0; j < WIDTH; j++) {
                    if(gamestate->dmap[i][j] == 11 && gamestate->vmap[i][j] != 9) gamestate->vmap[i][j] = 11;
                    if(gamestate->vmap[i][j] == 9 && gamestate->dmap[i][j] != 11) gamestate->vmap[i][j] = 12;
                }
            }
            gamestate->final_time = GetTime() - gamestate->start_time;
            gamestate->menu = 4;
            gamestate->pause_time_buffer = GetTime() - gamestate->start_time;

        } 
    } else if (gamestate->vmap[row][col] < 9 && gamestate->vmap[row][col] != 0) {
        int num_flags = 0;
        for(int i = -1; i <= 1; i++) {
            for(int j = -1; j <= 1; j++) {
                if(!in_bounds(row + i, col + j)) continue;
                if(gamestate->vmap[row + i][col + j] == 9) {
                    num_flags++;
                }
            }
        }

        if(num_flags == gamestate->dmap[row][col]) {
            for(int i = -1; i <= 1; i++) {
                for(int j = -1; j <= 1; j++) {
                    if(!in_bounds(row + i, col + j)) continue;
                    if(gamestate->vmap[row + i][col + j] == 10) {
                        click(gamestate, row + i, col + j, 0);
                    }
                }
            }
        }
    }
}

void displayMap(Gamestate *gamestate, Texture tiles[13]) {
    Vector2 origin = {0, 0};
    Rectangle src = {0, 0, TILE_SIZE, TILE_SIZE};

    for(int row = 0; row < HEIGHT; row++) {
        for(int col = 0; col < WIDTH; col++) {
            Rectangle dst = {col * TILE_SIZE * gamestate->scale, row * TILE_SIZE * gamestate->scale, TILE_SIZE * gamestate->scale, TILE_SIZE * gamestate->scale};
            DrawTexturePro(tiles[gamestate->vmap[row][col]], src, dst, origin, 0, WHITE);
        }
    }
}

void initializeMap(Gamestate *gamestate) {

    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < WIDTH; j++) {
            gamestate->dmap[i][j] = 0;
        }
    }

    int mines_left = MINES;
    while(mines_left > 0) {
        int row = rand() % HEIGHT;
        int col = rand() % WIDTH;
        
        if(gamestate->dmap[row][col] != 11) {
            gamestate->dmap[row][col] = 11;
            mines_left--;
        }
    }

    for(int row = 0; row < HEIGHT; row++) {
        for(int col = 0; col < WIDTH; col++) {
            if(gamestate->dmap[row][col] == 11) {
                for(int i = -1; i <= 1; i++) {
                    for(int j = -1; j <= 1; j++) {
                        if(in_bounds(row + i, col + j) && gamestate->dmap[row + i][col + j] < 9) {
                            gamestate->dmap[row + i][col + j]++;
                        }  
                    }
                }
            }
        }
    }

    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < WIDTH; j++) {
            gamestate->vmap[i][j] = 10;
        }
    }

    // deciding if i want to make the map regenerate if BBBV is too low.
    gamestate->BBBV = calculateBBBV(gamestate);
    
}

int calculateBBBV(Gamestate *gamestate) {
    int clicks = 0;

    for(int row = 0; row < HEIGHT; row++) {
        for(int col = 0; col < WIDTH; col++) {
            if(gamestate->vmap[row][col] == 10 && gamestate->dmap[row][col] == 0) {
                clicks++;
                click(gamestate, row, col, 0);
            }
        }
    }

    for(int row = 0; row < HEIGHT; row++) {
        for(int col = 0; col < WIDTH; col++) {
            if(gamestate->vmap[row][col] == 10 && gamestate->dmap[row][col] < 9 && gamestate->dmap[row][col] != 0) {
                clicks++;
                click(gamestate, row, col, 0);
            }
        }
    }
    
    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < WIDTH; j++) {
            gamestate->vmap[i][j] = 10;
        }
    }

    return clicks;
}


void openMap(Gamestate *gamestate, int row, int col) {
    gamestate->vmap[row][col] = gamestate->dmap[row][col];
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            // check bounds
            if(!in_bounds(row + i, col + j)) continue; 
            // checks if tile is safe and unvisted
            if(gamestate->dmap[row + i][col + j] >= 9 || gamestate->vmap[row + i][col + j] != 10) continue;
            // checks if text tile is empty or not. if not empty, reveal and end, else recurse. 
            if(gamestate->dmap[row + i][col + j] != 0) {
                gamestate->vmap[row + i][col + j] = gamestate->dmap[row + i][col + j];
            } else {
                openMap(gamestate, row + i, col + j);
            }
        }
    }
}

int checkWin(Gamestate *gamestate) {
    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < WIDTH; j++) {
            if(gamestate->dmap[i][j] < 9 && gamestate->vmap[i][j] >= 9) return 0;
        }
    }
    return 1;
}
