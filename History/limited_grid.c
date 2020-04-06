#define _XOPEN_SOURCE_EXTENDED

#include <ncurses.h>

#include <locale.h>
#include <wchar.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <time.h>

using namespace std;

const int sizex = 64;
const int sizey = 64;
const int world_height = 5;
int cur_level = 0;

int px = 16;
int py = 16;
int camx = 16;
int camy = 16;

void note(int a){
    move(0,0);
    printw("%d", a);
    refresh();
    getch();
}

int rgbTo8bit(int r, int g, int b){

    bool grayPossible = true;
    bool gray = false;
    float sep = 42.5;

    while (grayPossible){
        if (r < sep || g < sep || b < sep){
            gray = r < sep && g < sep && b < sep;
            grayPossible = false;
        }
        sep += 42.5;
    }
    if (gray){
        return round(232.0 + (r+g+b) / 33.0);
    }else{

        return (16 + ((int) (6.0 * ( (float) r / 256.0))) 
                * 36 + ((int) (6.0 * ( (float) g / 256.0))) 
                * 6 + ((int) (6.0 * ( (float) b / 256.0))) );
    }
}



//Every tile in the level is a Struct and holds information
struct Tile {

	std::string type = " ";
	std::string fontsize = "Small";
    char ascii = ' ';
    wchar_t* ext_ascii;
    float char_rgb[3];
    float back_rgb[3];
    float delta_rgb[3] = {0,0,0};
    int change_rgb_range = 0;
    bool blocking = false;
    int EightBitColorL = 0;
    int EightBitColorR = 0;

    Tile( std::string type,
          int r1, int g1, int b1,
          wchar_t* elem, 
          int r2, int g2, int b2,
          std::string fontsize) {

		this->type = type;
		this->fontsize = fontsize;
        this->ext_ascii = elem;

        char_rgb[0] = r1; // .255;
        char_rgb[1] = g1; // .255;
        char_rgb[2] = b1; // .255;

        back_rgb[0] = r2; // .255;
        back_rgb[1] = g2; // .255;
        back_rgb[2] = b2; // .255;

        EightBitColorL = rgbTo8bit(r1, g1, b1);
        EightBitColorR = rgbTo8bit(r2, g2, b2);

        // printf("%d", EightBitColor);

        if (type == "Grass") {
            change_rgb_range = 10;
            //EightBitColorR = 127;
            // EightBitColorL = 155;
            //for (int i = 0; i < 3; i++) delta_rgb[i] = (rand()%100) / .255;
        }
        if (type == "Wall"){
        	blocking = true;
            // EightBitColorR = 125;
        }
        if (type == "Floor"){
            // EightBitColorR = 126;
        }

    }
};

//Struct of pointers to 'tiles' called 'Tile'
struct Level {
	struct Tile* tiles[sizey][sizex];
};

//For multiple levels
struct Level World[world_height];



wchar_t* chooseWalls(std::vector<int> &g, int n){
    wchar_t* a = L"╬"; //4-way intersect

    int sx = sizex; int sy = sizey;
    int c = 2;
    //Check each side 1 by 1
    if (g[n+1] != c && g[n-1] != c && g[n+sx] != c && g[n-sx] != c) a = L"■"; //single pillar
    if (g[n+1] != c && g[n-1] != c && g[n+sx] == c && g[n-sx] == c) a = L"║"; //vertical
    if (g[n+1] == c && g[n-1] == c && g[n+sx] != c && g[n-sx] != c) a = L"═"; //horizontal
    if (g[n+1] == c && g[n-1] == c && g[n+sx] == c && g[n-sx] == c) a = L"╬"; //4-way

    if (g[n+1] != c && g[n-1] == c && g[n+sx] == c && g[n-sx] == c) a = L"╣"; //3-way, no right
    if (g[n+1] == c && g[n-1] != c && g[n+sx] == c && g[n-sx] == c) a = L"╠"; //3-way, no left
    if (g[n+1] == c && g[n-1] == c && g[n+sx] != c && g[n-sx] == c) a = L"╩"; //3-way, no top
    if (g[n+1] == c && g[n-1] == c && g[n+sx] == c && g[n-sx] != c) a = L"╦"; //3-way, no bottom

    if (g[n+1] != c && g[n-1] == c && g[n+sx] != c && g[n-sx] == c) a = L"╝"; //2-way, no right bottom
    if (g[n+1] == c && g[n-1] != c && g[n+sx] != c && g[n-sx] == c) a = L"╚"; //2-way, no left bottom
    if (g[n+1] != c && g[n-1] == c && g[n+sx] == c && g[n-sx] != c) a = L"╗"; //2-way, no right top
    if (g[n+1] == c && g[n-1] != c && g[n+sx] == c && g[n-sx] != c) a = L"╔"; //2-way, no left top

    if (g[n+1] == c && g[n-1] != c && g[n+sx] != c && g[n-sx] != c) a = L"═";
    if (g[n+1] != c && g[n-1] == c && g[n+sx] != c && g[n-sx] != c) a = L"═";
    if (g[n+1] != c && g[n-1] != c && g[n+sx] != c && g[n-sx] == c) a = L"║"; //2

    return a;
}

int within_bounds(int w, int h, int i, int j){
    if (i > w && j > h && i < sizex - w && j < sizey - h)
        return 1;
    return 0;
}

int genRectangle(std::vector<int> &g){
    int count = 0;
    int width = rand() %6 + 6;
    int height = rand() %3 + 3;

    //Determine how many places rectangle can be placed
    int n = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){
            if ( g[n] == 0 && within_bounds(width, height, i, j) )
                count++;
            n++;
        }
    }

    int find = rand() % count; count = 0;
    //Relocate rectangle location 
    n = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){

            if ( g[n] == 0 && within_bounds(width, height, i, j) )
                count++;
            
            if ( count == find ){
                break;
            }
            if ( n == sizex * sizex )
                return 0;
            n++;
        }
    }
    //Draw rectangle
    for (int i = -width; i < width; i++){
        for (int j = -height; j < height; j++){
            g[n+i+j*sizex] = 1;
        }
    }

    return 1;

}

bool surroundingTiles(std::vector<int> &check, int n, int against, int d){
    int sx = sizex; int sy = sizey;
    if (check[n + d] == against && check[n-d] == against && check[n-sx*d]== against && check[n+sx*d]== against){
        return true;
    }
    return false;
}
bool surroundingTilesAdj(std::vector<int> &check, int n, int against, int d){
    int sx = sizex; int sy = sizey;
    if (check[n + d] == against || check[n-d] == against || check[n-sx*d] == against || check[n+sx*d] == against
            || check[n + d +sx*d] == against || check[n-d+sx*d] == against || check[n-sx*d + d] == against || check[n-sx*d - d] == against){
        return true;
    }
    return false;
}

void fillEdges(std::vector<int> &genTiles, std::vector<int> &genTemp){
    int sx = sizex; int sy = sizey;    
    //Loop through and edit tiles based on surrounding tiles
    for (int n = 0; n < sx*sy; n++) {
        if (genTiles[n] == 1) {
            if (surroundingTilesAdj(genTiles, n, 0, 1) == true) {
                genTiles[n] = 2;
            }
        }
    }
}



void generateLevel(int loops){

	struct Level* W = &World[cur_level];

	std::vector<int> genTiles;
    std::vector<int> genTemp;
 
	//Randomize everything into noise
	for (int j = 0; j < sizey; j++){
		for (int i = 0; i < sizex; i++){
			int r = rand()%100;
	        //if (i <= 2 || i >= sizex - 3 || j <= 2 || j >= sizey - 3) r = 0;
	        if (r <= 45) r = 0;
	        else r = 1;
	        r = rand() % 2;
            r = 0;
	        genTiles.push_back(r);
		}
	}
	genTemp = genTiles;

    genRectangle(genTiles);
    genRectangle(genTiles);
    genRectangle(genTiles);
    genRectangle(genTiles);

    fillEdges(genTiles, genTemp);

    //Turn genTiles into ascii types
    int n = 0;
    for (int y = 0; y < sizey; y++) {
    	for (int x = 0; x < sizex; x++){

	        if (genTiles[n] == 0){
                wchar_t* a;
                int r = rand() % 4;
                if (r == 0) a = L",";
                if (r == 1) a = L"\'";
                if (r >= 2) a = L" ";
                int b = rand()%20;
                int c = 175;
	            W->tiles[y][x] = new Tile("Grass", 127,127,127, 
	            							a, 127+b,127+b,127+b, "Small");

	        }  
	        if (genTiles[n] == 1){
	            W->tiles[y][x] = new Tile("Floor", 150,210,225,
	                             			L" ",  100+rand()%20,200+rand()%20,255-rand()%20, "Small");
	        }
	        if (genTiles[n] == 2){
	            wchar_t* a = chooseWalls(genTiles, n);
                // note(a);
	            W->tiles[y][x] = new Tile("Wall", 150/2,210/2,225/2,
                                      		a,    255,255,255, "NoCenter");
        	}
	    	n++;

    	}
    }

}

struct Vector3 {
    int L;
    int R;
    int index;
};


float calcLighting(int x, int y, float brightness){

    float taper = 1.0 / brightness;

    //If tile too far, auto no light
    float a = px - x;
    float b = py - y;
    float c = 1-sqrt(a*a+b*b) * taper;
    if (c <= 0){ 
        return 0;
    }
    return c;

    struct Level* W = &World[cur_level];

    //Ray Tracing
    int blocking = 0;
    for (float d = 0.0; d < brightness-1; d++){
        float ry = py-y; float rx = px-x;
        ry *= (d*taper); rx *= (d*taper); 
        int xl = int(px-rx);
        int yl = int(py-ry);
        if (xl < 0 || yl < 0 || xl >= sizex || yl >= sizey) continue;
        if ( W->tiles[yl][xl]->blocking == true ){
            blocking = 1;
            break;
        }
    }

    //Final decision
    if (blocking == 0){
        c = sqrt(c);
        if (c >= 1) c*=2;
        return c;
    }
    return 0;

}

int main(int argc, char *argv[]){

    srand(time(0));

	WINDOW* win = stdscr;
	setlocale(LC_ALL, "");

	initscr();


	if(has_colors() == FALSE || can_change_color() == FALSE)
	{	
		endwin();
		printf("Your terminal does not support color/changing colors\n");
		exit(1);
	}
	cbreak(); noecho(); keypad(win, TRUE);
	clear();
	curs_set(0);
	start_color();

    //Gen Level
    generateLevel(40);

    int ch = 0;
    int turns = 0;

    //Main Loop
	while(1){

		turns++;
		int n = 1;
        std::vector<Vector3> savedPairs;

		for (int j = 0; j < sizex; j++){
		    for (int i = 0; i < sizey; i++){

				struct Level* W = &World[cur_level];

				int x = i - camx + COLS / 2;
				int y = j - camy + LINES / 2;

                struct Tile* T;


                if (y >= LINES || x >= COLS || y < 0 || x < 0){
                    // continue;
                    T = new Tile("Grass", 150,180,100,
                                    L",",    30,160,80, "NoCenter");
                }else{
                    T = (W->tiles[j][i]);
                }
				

                //Light calc
                float value = calcLighting(i, j, 25);
                // if (value <= 0) continue;

                //Generate new color values based on shadow
                int newR = rgbTo8bit( T->back_rgb[0]*value, T->back_rgb[1]*value, T->back_rgb[2]*value );
                int newL = rgbTo8bit( T->char_rgb[0]*value, T->char_rgb[1]*value, T->char_rgb[2]*value );

                //See if pair exists
                int index = n;
                bool found = false;
                for (auto elem : savedPairs ){
                    // if (elem.L == T->EightBitColorL && elem.R == T->EightBitColorR ){
                    if (elem.L == newL && elem.R == newR ){
                        found = true; 
                        index = elem.index; n--; break;
                    }
                }

                //If pair does not exist yet, create new one
                if (found == false){
                    // init_pair(index, T->EightBitColorL, T->EightBitColorR);
                    init_pair(index, newL, newR);
                    Vector3 p;
                    // p.L = T->EightBitColorL;
                    // p.R = T->EightBitColorR;
                    p.L = newL;
                    p.R = newR;
                    p.index = index;
                    savedPairs.push_back( p );
                }

				//Set color
				attron(COLOR_PAIR(index));

                mvwprintw(stdscr, y, x, "%ls\n", T->ext_ascii);

                attroff(COLOR_PAIR(index));

                //Draw player
                if (px == i && py == j){
                    int playerBit  = rgbTo8bit(T->char_rgb[0]/2, T->char_rgb[1]/2, T->char_rgb[2]/2);
                    // int playerBack = rgbTo8bit(T->back_rgb[0]/2, T->back_rgb[1]/2, T->back_rgb[2]/2);

                    init_pair(255, playerBit, T->EightBitColorR);
                    attron(COLOR_PAIR(0));
                    mvwprintw(stdscr, y, x, "%c", '@');
                    attroff(COLOR_PAIR(0));
                }


                //Info in top right corner
                if (COLS/2 == x && y == LINES/2){
                    attron(COLOR_PAIR(0));
                    mvwprintw(stdscr, 1, 5, "%d", T->EightBitColorL);
                    mvwprintw(stdscr, 2, 5, "%d", T->EightBitColorR);
                    attroff(COLOR_PAIR(0));
                }

				attroff(COLOR_PAIR(index));

				n++;
                if (n > 254) n = 1;

			}
	    }

    	//mvwprintw(stdscr, 1,1, "%d", turns);
        // note(n);
                /*int kk = 0;
                int jj = 0;
                for (auto elem : savedPairs ){
                    // note(elem.index);
                    // if (elem.L == T->EightBitColorL && elem.R == T->EightBitColorL ){
                        move(jj, 1);
                        printw("%d, %d -%d", elem.L, elem.R, elem.index);
                        jj++;
                    // }
                } */

    	refresh();
    	ch = getch();

    	if (ch == 68){ //Left
    		px--;
    	}
    	if (ch == 67){ //Right
    		px++;
    	}
    	if (ch == 65){ //Up
    		py--;
    	}
    	if (ch == 66){ //Down
    		py++;
    	}
    	if (ch == KEY_F(1))
    		break;

        //Update camera position if moved too far 
        int oldx = camx;
        int oldy = camy;
        if ( (abs(px-camx) >= COLS / 4) || abs(py-camy) >= LINES / 4){
            camx = px;
            camy = py;
        }

        //Clear areas that are out of bounds
        for (int j = 0; j < sizex; j++){
            for (int i = 0; i < sizey; i++){
                
                int x = i - oldx + COLS / 2;
                int y = j - oldy + LINES / 2;

                int newx = i - camx + COLS / 2;
                int newy = j - camy + LINES / 2;

                if (x > newx || y > newy || x < newx || y < newy){
                    move(y, x);
                    printw(" ");
                }
            }
        }

	}

	endwin();
}

