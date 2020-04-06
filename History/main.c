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

const int sizex = 128;
const int sizey = 128;
const int world_height = 5;
int cur_level = 0;

int px = 16;
int py = 16;
int camx = 16;
int camy = 16;

//////////////////////////////PERLIN NOISE

static int SEED;

static int hash_noise[] = {208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
                     185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
                     9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
                     70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
                     203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
                     164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
                     228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
                     232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
                     193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
                     101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
                     135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
                     114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219};

int noise2(int x, int y)
{
    int tmp = hash_noise[(y + SEED) % 256];
    return hash_noise[(tmp + x) % 256];
}

float lin_inter(float x, float y, float s)
{
    return x + s * (y-x);
}

float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3-2*s));
}

float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int+1, y_int);
    int u = noise2(x_int, y_int+1);
    int v = noise2(x_int+1, y_int+1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}

float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x*freq;
    float ya = y*freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for(i=0; i<depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin/div;
}

float ridgenoise(float x, float y, float f, int d){
    float e = 2 * (.5 - abs(.5 - perlin2d(x,y,f,d)));
    return pow(e, 1);
}

float turb(float cx, float cy, float f, int d){
    return .5 * perlin2d(cx,cy,f,d) + 
          .25 * perlin2d(2*cx,2*cy,f,d) + 
         .125 * perlin2d(4*cx,4*cy,f,d);
}




////////////////////////////Misc functions

void note(int a){
    move(0,0);
    printw("%d", a);
    refresh();
    getch();
}


// Info in top right corner
void topInfo(int a, int b, int x, int y){
    if (px == x && y == py){

        attron(COLOR_PAIR(0));
        mvwprintw(stdscr, 1, 5, "%d", a);
        mvwprintw(stdscr, 2, 5, "%d", b);
        attroff(COLOR_PAIR(0)); 
    }
}

int rgbTo8bit(int r, int g, int b){

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

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
    bool remember = false;

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

        for (int n = 0; n < 3; n++){
            if (char_rgb[n] > 255) char_rgb[n] = 255;
            if (back_rgb[n] > 255) back_rgb[n] = 255;
        }

        EightBitColorL = rgbTo8bit(r1, g1, b1);
        EightBitColorR = rgbTo8bit(r2, g2, b2);

        if (type == "Snow") {
            change_rgb_range = 10;
        }
        if (type == "Wall"){
        	blocking = true;
        }
        if (type == "Floor"){

        }
        if (type == "Ice"){
            blocking = true;
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

int Determine(std::vector<int> &g, int searchTile, int width, int height){
    int count = 0;
    int n = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){
            if ( g[n] == searchTile && within_bounds(width, height, i, j) )
                count++;
            n++;
        }
    }
    return count;
}

int Locate(std::vector<int> &g, int searchTile, int find, int width, int height){
    int n = 0;
    int count = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){

            if ( g[n] == searchTile && within_bounds(width, height, i, j) )
                count++;
            
            if ( count == find ){
                break;
            }
            if ( n == sizex * sizex ){
                return 0;
            }
            n++;
        }
    }
    return n;
}


int genHalls(std::vector<int> &g){
    for (int i = 0; i < 5; i++){
        for (int j = 10; j < sizey-10; j++){
            int n = i+j*sizex;
            g[n] = 1;
        }
    }
}

int genRectangle(std::vector<int> &g){
    int count = 0;
    int a = rand() % 15;
    int width = a + 10;
    int height = (15-a) + 10;
    int find;
    int n;

    //Determine how many places rectangle can be placed
    count = Determine(g, 0, width*2, height*2);
    find = rand() % count; count = 0;

    //Relocate rectangle location 
    n = Locate(g, 0, find, width*2, height*2);

    //Draw rectangle
    for (int i = -width; i < width; i++){
        for (int j = -height; j < height; j++){
            g[n+i+j*sizex] = 1;

            //Put player in center
            px = n % sizex;
            py = int(n / sizey);
            camx = px; camy = py;
        }
    }

    int remw = width / 2;
    int remh = height / 2;
    count = Determine(g, 1, width*2, height*2);
    find = rand() % count; count = 0;
    n = Locate(g, 1, find, width*2, height*2);
    for (int i = -remw; i < remw; i++){
        for (int j = -remh; j < remh; j++){
            g[n+i+j*sizex] = 0;
            // px = n % sizex;
            // py = int(n / sizey);
            camx = px; camy = py;
        }
    }

    return 1;

}

void addWindow(std::vector<int> &g){
    int count, find, n;
    //Determine how many places window can be placed
    count = Determine(g, 1, 0,0);
    find = rand() % count; count = 0;

    //Relocate window
    n = Locate(g, 1, find, 0,0);
    g[n] = 4;

    px = n % sizex;
    py = int(n / sizey);
}

void genFloorPlan(std::vector<int> &genTiles, std::vector<int> &genTemp){

    int first = 3;
    int alt = 0;
    //Loop through and edit tiles based on surrounding tiles
    int sx = sizex; int sy = sizey;
    for (int n = 0; n < sx*sy; n++) {

        int a = 0;
        if (genTiles[n] != alt) a += 1;
        if (genTiles[n + 1] != alt) a += 1;
        if (genTiles[n - 1] != alt) a += 1;
        if (genTiles[n + sx] != alt) a += 1;
        if (genTiles[n - sx] != alt) a += 1;

        if (genTiles[n + 1 + sx] != alt) a += 1;
        if (genTiles[n - 1 + sx] != alt) a += 1;
        if (genTiles[n + 1 - sx] != alt) a += 1;
        if (genTiles[n - 1 - sx] != alt) a += 1;

        genTemp[n] = alt;
        if (a >= 5) genTemp[n] = first;

    }
    //Modify tiles into new
    for (int n = 0; n < sx*sy; n++) {
        genTiles[n] =  genTemp[n];
    }
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
            if (surroundingTilesAdj(genTiles, n, 0, 1) == true 
                || surroundingTilesAdj(genTiles, n, 3, 1) == true) {
                
                genTiles[n] = 2;
            }
        }
    }
}

void genWalls(std::vector<int> &genTiles, std::vector<int> &genTemp){
    // return;
    //Loop through and edit tiles based on surrounding tiles
    int sx = sizex; int sy = sizey;
    for (int n = 0; n < sx*sy; n++) {
        if (n < sx*sy - sx*3 && genTiles[n] != 0) {
            float a = 0;
            float b = 0;
            float c = 0;
            if (genTiles[n] == 2) a += 0;
            if (genTiles[n + 1 * 1] == 2){ a += 1; c += 1; }
            if (genTiles[n - 1 * 1] == 2){ a += 1; c += 1;}
            if (genTiles[n + sx * 1] == 2){ b += 1; c += 1;}
            if (genTiles[n - sx * 1] == 2){ b += 1; }
            // int o = rand() % 3+1;
            // int t = rand() % 3+1;
            int o = 1;
            int t = 1;
            if (genTiles[n +1*o + sx*o]==2) {
                a -= 0; b -= .5; c -= .5; }
            if (genTiles[n -1*t + sx*t]==2){
                a -= .5; b -= .5; ; c -= .5; }
            if (genTiles[n +1*o - sx*o]==2) {
                a -= .5; b -= .5; ; c -= .5; }
            if (genTiles[n -1*t - sx*t]==2) {
                a -= .5; b -= .5; ; c -= .5; }

            genTiles[n] = genTemp[n];

            if (genTiles[n] == 0 && genTiles[n-1] == 2 && genTiles[n+1+sx] == 2){
                genTiles[n] = 2;
                genTiles[n+sx] = 2;
            }
            if (genTiles[n] == 0 && genTiles[n-1] == 2 && genTiles[n+1-sx] == 2){
                genTiles[n] = 2;
                genTiles[n-sx] = 2;
            }

            if (a >=1 || b >= 1 || c >= 1){
                int r = rand() % 30;
                genTiles[n] = 2;
                if ( r == 0 && b >= 1)
                    genTiles[n+1] = 2;
            }
        }else{
            genTemp[n] = 0;
        }
    }
}


void randomRooms(std::vector<int> &genTiles, std::vector<int> &genTemp){
    int sx = sizex; int sy = sizey;
    for (int n = 0; n < sx*sy; n++) {
        int r = rand()%10;
        int a = 0;
        int x = n % sx;
        int y = int(n/sx);

        for (int i = -3; i < 0; i++){
            for (int j = -3; j < 0; j++){
                // if ( x-i < 0 || y-j < 0 ) continue;
                if (genTiles[n+i+j*sx] == 2){
                    a = 1;
                }
            }
        }
        if (a == 1) continue;

        if (genTiles[n] == 1 && r == 0
            && surroundingTiles(genTiles,n,1,2) == true ) {

            genTiles[n] = 2; //Random walls placed
            int s = rand() % 20;
            if ( s == 0) genTiles[n+1] = 2;
            if ( s >= 1) genTiles[n+sx] = 2;
        }
    }
}


vector<struct Tile*> Objects;
int airColor;

void generateLevel(int loops){

	struct Level* W = &World[cur_level];

	std::vector<int> genTiles;
    std::vector<int> genTemp;
 
	//All air
	for (int j = 0; j < sizey; j++){
		for (int i = 0; i < sizex; i++){
            int r = 0;
	        genTiles.push_back(r);
		}
	}
	genTemp = genTiles;

    //for(int n = 0; n < 20; n++) {
    //   genFloorPlan(genTiles, genTemp);
    //}

    // genHalls(genTiles);
    genRectangle(genTiles);
    genTemp = genTiles;

    randomRooms(genTiles, genTemp);
    for(int n = 0; n < loops+20; n++) {
        genWalls(genTiles, genTemp);
    }

    fillEdges(genTiles, genTemp);

    //Turn genTiles into ascii types
    int n = 0;
    for (int y = 0; y < sizey; y++) {
    	for (int x = 0; x < sizex; x++){

	        if (genTiles[n] == 0){
                int s = rand() % 30;
                W->tiles[y][x] = new Tile("Air",  0,0,0,
                                            L" ", 200,airColor,100, "Small");

	        }  
	        if (genTiles[n] == 1){
	            W->tiles[y][x] = new Tile("Floor", 0,0,0,
	                             			L" ",  0,0,40, "Small");
	        }
	        if (genTiles[n] == 2){
                int r = rand() % 10;
                wchar_t* a = chooseWalls(genTiles, n);

                if (r == 0 && (a == L"║" || a == L"═")){
	                W->tiles[y][x] = new Tile("Window",     105,105,255,
                                            L"[",       0,0,80, "Small");
                }else{
                    W->tiles[y][x] = new Tile("Wall", 0,0,255,
                                            a,        0,0,80, "NoCenter");
                }
            }
	    	n++;
            // W->tiles[y][x]->remember = true;

    	}
    }

}

struct Vector3 {
    int L;
    int R;
    int index;
};


float calcLighting(int x, int y, float brightness){

    float taper = 1.0 / (brightness);

    //If tile too far, auto no light
    float a = px - x;
    float b = py - y;
    float c = 1-sqrt(a*a+b*b) * taper;
    if (c <= 0){ 
        return 0;
    }

    struct Level* W = &World[cur_level];

    //Ray Tracing
    int blocking = 0;
    int xl, yl;

    for ( float d = 0.0; d < brightness; d+= c*.9 ){
        float ry = py-y; float rx = px-x;
        ry *= (d*taper); rx *= (d*taper); 
        xl = px-int(rx);
        yl = py-int(ry);
        if (xl < 0 || yl < 0 || xl >= sizex || yl >= sizey){ continue; }
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



float noise(int x, int y) {
    int n;
    n = x + y * 57;
    n = (n << 13) ^ n;
    return (1.0 - ( (n * ((n * n * 15731) + 789221) +  1376312589) & 0x7fffffff) / 1073741824.0);
}


int main(int argc, char *argv[]){

    srand(time(0));

    airColor = rand() % 2;
    if (airColor == 1) airColor = 200;

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
    int turns = -1;

    //Main Loop
	while(1){

		turns++;
		int n = 1;
        std::vector<Vector3> savedPairs;

		for (int j = 0; j < LINES; j++){
		    for (int i = 0; i < COLS / 2; i++){

				struct Level* W = &World[cur_level];

				int x = i + camx - COLS / 4;// + COLS / 2;
				int y = j + camy - LINES / 2;// + LINES / 2;

                struct Tile* T;
                bool autoTile = false;

                if (x >= 0 && x < sizex && y >= 0 && y < sizey && W->tiles[y][x] != NULL){
                    T = (W->tiles[y][x]);
                }else{
                    autoTile = true;
                    int c = int(noise(x, y) * 3) % 3;
                    int d = int(noise(y, x) * 4) % 4;             

                    int s = 255-d*10;
                    int t = 255-c*10;
                    int a = 100;
                    T = new Tile("Air",     0,0,0,
                                    L" ",   200,airColor,100, "Small");
                    
                    // topInfo(c, b, x, y);
                }

                //Light calc
                float value;

                value = calcLighting(x, y, 10);
                if (value <= 0) value = 0;
                if (value > 0){
                    
                    if (T->type != "Air"){ T->remember = true; value = 1;}
                }
                if (value == 0 && T->remember == true){
                    value = .5;
                }

                //Generate new color values based on shadow
                int newR = rgbTo8bit( T->back_rgb[0]*value, T->back_rgb[1]*value, T->back_rgb[2]*value );
                int newL = rgbTo8bit( T->char_rgb[0]*value, T->char_rgb[1]*value, T->char_rgb[2]*value );

                //See if pair exists
                int index = n;
                bool found = false;
                for (auto elem : savedPairs ){
                    if (elem.L == newL && elem.R == newR ){
                        found = true; 
                        index = elem.index; n--; break;
                    }
                }

                //If pair does not exist yet, create new one
                if (found == false){
                    init_pair(index, newL, newR);
                    Vector3 p;
                    p.L = newL;
                    p.R = newR;
                    p.index = index;
                    savedPairs.push_back( p );
                }

				//Set color
				attron(COLOR_PAIR(index));

                //Footsteps on snow
                wchar_t* sgl = T->ext_ascii;
                wchar_t* dou = L" ";
                if (T->type == "Wall" && T->ext_ascii != L"╣" && T->ext_ascii != L"║" && T->ext_ascii != L"╗" && T->ext_ascii != L"╝")
                    dou = L"═";
                if (T->type == "Window")
                     dou = L"]";
                if (T->type == "Snow" && T->ext_ascii == L"."){
                    int r = (y) % 2;
                    if (r == 1){
                        sgl = L" ";
                        dou = L".";
                    }
                }

                //Draw tile
                mvwprintw(stdscr, j, i*2,   "%ls\n", sgl);
                mvwprintw(stdscr, j, i*2+1, "%ls\n", dou);

                //Turn off color
                attroff(COLOR_PAIR(index));

                //Draw player
                if (px == x && py == y){
                    // int playerBit  = rgbTo8bit(T->char_rgb[0]/2, T->char_rgb[1]/2, T->char_rgb[2]/2);
                    // int playerBack = rgbTo8bit(T->back_rgb[0]+40, T->back_rgb[1]+40, T->back_rgb[2]+40);
                    int playerBit = rgbTo8bit(50,100,50);
                    int playerBack = rgbTo8bit(0,255,0);
                    init_pair(255, playerBack, playerBit);
                    attron(COLOR_PAIR(255));
                    mvwprintw(stdscr, j,i*2,   "%c", 'A');
                    mvwprintw(stdscr, j,i*2+1, "%c", 'l');
                    attroff(COLOR_PAIR(255));

                    if (T->type == "Snow" && T->ext_ascii != L"."){
                        T->ext_ascii = L".";
                        T->back_rgb[0] *= .9;
                        T->back_rgb[1] *= .9;
                    }
                }

                if (autoTile == true){
                    free(T);
                }
            
				n++;
                if (n > 254) n = 1;


			}
	    }

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

        if (px-camx > COLS / 8){
            camx = px + COLS / 8;
        }
        if (px-camx < -COLS / 8){
            camx = px - COLS / 8;
        }
        if (py-camy > LINES / 4){
            camy = py + LINES / 4;
        }
        if (py-camy < -LINES / 4){
            camy = py - LINES / 4;
        }

	}

	endwin();
}

