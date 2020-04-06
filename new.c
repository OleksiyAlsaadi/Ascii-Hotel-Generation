#define _XOPEN_SOURCE_EXTENDED
#define _GNU_SOURCE

#include <ncurses.h>
#include <spawn.h>
#include <math.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <time.h>
#include <cstdio>
using namespace std;

#define gotoxy(x,y) printf("\033[%d;%dH",(y),(x))

void printT(int x, int y, string s, int F_R, int F_G, int F_B,   int B_R, int B_G, int B_B){
    gotoxy(x+1,y+1);
    // printf("%s", s*);
    // printf("test");
    printf("\033[48;2;%d;%d;%dm", B_R, B_G, B_B);
    printf("\033[38;2;%d;%d;%dm", F_R, F_G, F_B);
    printf("%s\033[0m", s.c_str());
}

int px = 64;
int py = 64;
int camx = px;
int camy = py;

int ch;
int turns = 0;
int fresh_all = 2;
const int sizex = 128;
const int sizey = 128;

int tiles[sizey][sizex];
string word[sizey][sizex];
int fresh[sizey][sizex];
int f_r[sizey][sizex];
int f_g[sizey][sizex];
int f_b[sizey][sizex];
int b_r[sizey][sizex];
int b_g[sizey][sizex];
int b_b[sizey][sizex];
float light_arr[sizey][sizex];
int blocks[sizey][sizex];
int extra[sizey][sizex];

float surroundingTilesFour(int y, int x, int against){
    float a = 0;
    if (tiles[y][x+1] == against) a+=1;
    if (tiles[y][x-1] == against) a+=1;
    if (tiles[y+1][x] == against) a+=1;
    if (tiles[y-1][x] == against) a+=1;
    return a;
}

float surroundingTilesAdj(int y, int x, int against){
    float a = 0;
    if (tiles[y][x+1] == against) a+=1;
    if (tiles[y][x-1] == against) a+=1;
    if (tiles[y+1][x] == against) a+=1;
    if (tiles[y-1][x] == against) a+=1;
    if (tiles[y+1][x+1] == against) a+=.25;
    if (tiles[y-1][x+1] == against) a+=.25;
    if (tiles[y+1][x-1] == against) a+=.25;
    if (tiles[y-1][x-1] == against) a+=.25;
    return a;
}

void rgb(int y, int x, int a, int b, int c, int d, int e, int f){
    f_r[y][x] = a;
    f_g[y][x] = b;
    f_b[y][x] = c;
    b_r[y][x] = d;
    b_g[y][x] = e;
    b_b[y][x] = f;
}


float calcLighting(int posx, int posy, int x, int y, float brightness){

    float taper = 1.0 / brightness;

    //If tile too far, auto no light
    float a = posx - x;
    float b = posy - y;
    float c = 1-sqrt(a*a+b*b) * taper;
    if (c <= 0){ 
        return 0;
    }

    //Ray Tracing
    int blocking = 0;
    for (float d = 0.0; d < brightness; d+= c*.9 ){
        float ry = posy-y; float rx = posx-x;
        ry *= (d*taper); rx *= (d*taper); 
        int xl = posx-int(rx);
        int yl = posy-int(ry);
        if (xl < 0 || yl < 0 || xl >= sizex || yl >= sizey){ blocking = 1; break; }
        if ( blocks[yl][xl] < 0 ){
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

void lightAll(){

    for (int y = 0; y < sizey; y++) {
        for (int x = 0; x < sizex; x++) {
            light_arr[y][x] = 0;
        }
    }


    //Calc Lighting
    for (int yy = 0; yy < sizey; yy++) {
    for (int xx = 0; xx < sizex; xx++) {

        if ( blocks[yy][xx] == 2 ){ //Window or Lamp
            for (int y = 0; y < sizey; y++) {
                for (int x = 0; x < sizex; x++) {
                    float value;
                    if ( blocks[y][x] == 0 || blocks[y][x] == -1 ) //Always lit
                        value = 1;
                    else{
                        value = calcLighting(xx,yy, x, y, 10);                    
                        if (value <= 0) value = 0;
                    }
                    light_arr[y][x] += value;
                    if (light_arr[y][x] > 1) light_arr[y][x] = 1;
                }
            }
        }

    }}
}

void genRandom(){
    int w = rand()%5+16;
    int h = rand()%(22-w)+11;
    for (int y = 0; y < sizey; y++){
        for (int x = 0; x < sizex; x++){
            fresh[y][x] = 0;
            tiles[y][x] = 0;
            blocks[y][x] = 0;
            word[y][x] = "  ";
            extra[y][x] = 0;


            if (x > int(sizex/2)-w && x < int(sizex/2)+w && y > int(sizey/2-h) && y < int(sizey/2)+h){
                tiles[y][x] = rand() % 3;
                if (tiles[y][x] == 2) tiles[y][x] = 1;
            }

            //tiles[y][x] = rand() % 2;
            /*if (x < 3 || y < 3 || x >= sizex-3 || y >= sizey-3 || y % 30 == 0 || x % 30 == 0){
                tiles[y][x] = 0;
            } */

        }
    }
}

void genFloorPlan(){
    int org = 0; //Grass
    int alt = 1; //Floor
    //Loop through and edit tiles based on surrounding tiles
    for (int y = 0; y < sizey; y++) {
        for (int x = 0; x < sizex; x++) {

            int a = 0;
            if (tiles[y][x] != alt) a += 1;
            if (tiles[y][x + 3] != alt) a += 1;
            if (tiles[y][x - 2] != alt) a += 1;
            if (tiles[y + 3][x] != alt) a += 1;
            if (tiles[y - 2][x] != alt) a += 1;

            // if (tiles[y + 1][x + 1] != alt) a += 1;
            // if (tiles[y + 1][x - 1] != alt) a += 1;
            // if (tiles[y - 1][x + 1] != alt) a += 1;
            // if (tiles[y - 1][x - 1] != alt) a += 1;

            tiles[y][x] = alt;
            if (a >= 3) tiles[y][x] = org;
        }


    }

}

string chooseWall(int y, int x, int rando){
    string a = "╬ "; //4-way intersect

    int c = 2;
    if (rando == 1){
        int e = rand()%16;
        if (e == 0) a = "╬ ";
        if (e == 1) a = "║ ";
        if (e == 2) a = "══";
        if (e == 3) a = "╬═";
        if (e == 4) a = "╣ ";
        if (e == 5) a = "╠═";
        if (e == 6) a = "╩═";
        if (e == 7) a = "╦═";
        if (e == 8) a = "╝ ";
        if (e == 9) a = "╚═";
        if (e == 10) a = "╗ ";
        if (e == 11) a = "╔═";
        return a;
    }
    //Check each side 1 by 1
    if (tiles[y][x+1] != c && tiles[y][x-1] != c && tiles[y+1][x] != c && tiles[y-1][x] != c) a = "╬ "; //single pillar █
    if (tiles[y][x+1] != c && tiles[y][x-1] != c && tiles[y+1][x] == c && tiles[y-1][x] == c) a = "║ "; //vertical
    if (tiles[y][x+1] == c && tiles[y][x-1] == c && tiles[y+1][x] != c && tiles[y-1][x] != c) a = "══"; //horizontal
    if (tiles[y][x+1] == c && tiles[y][x-1] == c && tiles[y+1][x] == c && tiles[y-1][x] == c) a = "╬═"; //4-way

    if (tiles[y][x+1] != c && tiles[y][x-1] == c && tiles[y+1][x] == c && tiles[y-1][x] == c) a = "╣ "; //3-way, no right
    if (tiles[y][x+1] == c && tiles[y][x-1] != c && tiles[y+1][x] == c && tiles[y-1][x] == c) a = "╠═"; //3-way, no left
    if (tiles[y][x+1] == c && tiles[y][x-1] == c && tiles[y+1][x] != c && tiles[y-1][x] == c) a = "╩═"; //3-way, no top
    if (tiles[y][x+1] == c && tiles[y][x-1] == c && tiles[y+1][x] == c && tiles[y-1][x] != c) a = "╦═"; //3-way, no bottom

    if (tiles[y][x+1] != c && tiles[y][x-1] == c && tiles[y+1][x] != c && tiles[y-1][x] == c) a = "╝ "; //2-way, no right bottom
    if (tiles[y][x+1] == c && tiles[y][x-1] != c && tiles[y+1][x] != c && tiles[y-1][x] == c) a = "╚═"; //2-way, no left bottom
    if (tiles[y][x+1] != c && tiles[y][x-1] == c && tiles[y+1][x] == c && tiles[y-1][x] != c) a = "╗ "; //2-way, no right top
    if (tiles[y][x+1] == c && tiles[y][x-1] != c && tiles[y+1][x] == c && tiles[y-1][x] != c) a = "╔═"; //2-way, no left top

    if (tiles[y][x+1] == c && tiles[y][x-1] != c && tiles[y+1][x] != c && tiles[y-1][x] != c) a = "══";
    if (tiles[y][x+1] != c && tiles[y][x-1] == c && tiles[y+1][x] != c && tiles[y-1][x] != c) a = "══";
    if (tiles[y][x+1] != c && tiles[y][x-1] != c && tiles[y+1][x] != c && tiles[y-1][x] == c) a = "║ "; //2

    return a;
}


void genRoad(){
    int shift = rand()%15-7;
    if (shift == 0) shift = 1;
    for (int y = 0; y < sizey; y++) {
        for (int x = 0; x < sizex; x++) {
            int ys = (y/shift)-(sizey/2/shift)-3;
            if (x <= sizex/2-26+ys && x >= sizex/2-32+ys){
                tiles[y][x] = 4; //Road
                if (x == sizex/2-29+ys && y%2==0) word[y][x] = " .";
                int s = rand()%16; int d = 40;
                if (x == sizex/2-26+ys || x == sizex/2-32+ys) d = 0;
                rgb(y,x, 200,200,200,  120-s-d,120-s-d,120-s-d);
            }
        }
    }
}

void genColors(){

    for (int y = 0; y < sizey; y++) {
        for (int x = 0; x < sizex; x++) {

            int f = fresh[y][x]*15; 
            if (tiles[y][x] == 0){ //Grass
                int r = rand() % 25;
                if (r == 0) word[y][x] = " '";
                if (r == 1) word[y][x] = " ,";
                if (r == 2) word[y][x] = "' ";
                if (r == 3) word[y][x] = ", "; 
                rgb(y,x, 100-f, 255-rand() % 100-f, 100-f,   1, rand() % 25+75-f, 1);
            }
            else if (tiles[y][x] == 1){ //Floor
                word[y][x] = "  "; 
                blocks[y][x] = 3; //No light naturally
                int s = rand()%15; //35+100;
                rgb(y,x, 180,150,80,  180-s,150-s,80-s);
                if (extra[y][x] == 1)
                    rgb(y,x, 180,150,80,  125-s,75-s,40-s);
            }
            else if (tiles[y][x] == 2){ //Wall
                word[y][x] = chooseWall(y,x,0);
                blocks[y][x] = -1; //Blocks and is always lit
                if (extra[y][x] == 1) 
                    blocks[y][x] = -2; //Blocks, unlit
                if (surroundingTilesAdj(y,x,0) >= 2){ //2 for fancy effect
                    rgb(y,x, 200,200,200,  1, rand() % 25+75-f, 1);
                }else{
                    int s = rand()%15;//35+100-35;
                    rgb(y,x, 200,200,200,  160-s,160-s,160-s);
                }
                fresh[y+1][x+1] = 1;
            }
            else if (tiles[y][x] == 3){ //Window
                blocks[y][x] = 2; //Generate light
                word[y][x] = chooseWall(y,x,0);
                rgb(y,x, 1,45*3,65*3,  1,65,85);
            }
            else if (tiles[y][x] == 4){ //Road //Pavement
                if (f_r[y][x] != 200){ //Prevent Previously placed road from changing
                    word[y][x] = "  ";
                    int s = rand()%15;
                    rgb(y,x, 200,200,200,  120-s,120-s,120-s);
                }
            }
            else if (tiles[y][x] == 5){ //Door
                blocks[y][x] = -1; //Blocks light and is always lit
                word[y][x] = "[]";
                int s = rand()%15;
                rgb(y,x, 100,200,200,  60-s,110-s,110-s);
            }
            else if (tiles[y][x] == 6){ //Lamp
                blocks[y][x] = 2; //Generate light
                word[y][x] = "|^";
                int s = rand()%15;
                rgb(y,x, 255,255,1,  125-s,75-s,40-s);
            }

            fresh[y][x] = 0;
            extra[y][x] = 0;
        }
    }
}

void genWalls(){
    for (int y = 0; y < sizey; y++) {
        for (int x = 0; x < sizex; x++) {
            if (tiles[y][x] == 1){ //Floor
                if (surroundingTilesAdj(y,x,0)){
                    tiles[y][x] = 2; // Wall;
                }
            }
        }
    }

}


int Determine(int against, int adj){
    int count = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){

            int f = surroundingTilesAdj(j,i,adj);

            if ( tiles[j][i] == against && surroundingTilesFour(j,i,adj) && floor(f) == f){
                int a = 0;
                if ( tiles[j+1][i] == against || tiles[j-1][i] == against 
                    || tiles[j][i+1] == against || tiles[j][i-1] == against ) a++;
                if ( a == 1 ) count++;
            }
                
        }
    }
    return count;
}

int addx = 0;
int addy = 0;

void Add(int against, int replace, int find, int adj){
    int count = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){

            int f = surroundingTilesAdj(j,i,adj);

            if ( tiles[j][i] == against && surroundingTilesFour(j,i,adj) && floor(f) == f){
                int a = 0;
                if ( tiles[j+1][i] == against || tiles[j-1][i] == against 
                    || tiles[j][i+1] == against || tiles[j][i-1] == against ) a++;
                if ( a == 1 ) count++;
            }
            
            if ( count == find ){
                tiles[j][i] = replace;
                addx = i; addy = j;
                return;
            }
        }
    }
}

void addTile(int search, int replace, int adj){
    int count, find, n;
    //Determine how many places window can be placed
    count = Determine(search, adj);
    // if (search == 0){ cout << count << endl; getch(); }
    if (count == 0) return;
    find = rand() % count+1; count = 0;

    //Relocate window
    Add(search, replace, find, adj);
}

struct Tree {
    int L;
    int R;
    int index;
    int parent;
};

void Dij(int y, int x, int find, int change){
    extra[y][x] = 1;
    std::vector<Tree> leaves;

    Tree l;
    l.L = y;
    l.R = x;
    l.parent = -1;
    leaves.push_back(l);

    for(int n = 0; n < 100; n++){
        int a = 0;
        int mm = -1;
        for (auto elem : leaves){
            mm++;

            auto j = elem.L;
            auto i = elem.R;

            if ( tiles[j][i] == find ) {
                int point = mm;
                while(1){
                    // cout << point << endl; getch();
                    int yy = leaves[point].L;
                    int xx = leaves[point].R;
                    tiles[yy][xx] = 4;
                    px = xx; py = yy;
                    point = leaves[point].parent;
                    if (point == -1) break;
                }
                a = 1;
                break;
            }

            if ( j < 1 || i < 1 || j > sizey-1 || i > sizex-1) continue;
            if ( blocks[j][i+1] >= 0 && extra[j][i+1] == 0 ){
                Tree p;
                p.L = j;
                p.R = i+1;
                p.parent = mm;
                leaves.push_back(p);
                extra[p.L][p.R] = 1;
                // word[p.L][p.R] = mm;
            }
            if ( blocks[j][i-1] >= 0 && extra[j][i-1] == 0 ){
                Tree p;
                p.L = j;
                p.R = i-1;
                p.parent = mm;
                leaves.push_back(p);
                extra[p.L][p.R] = 1;
            }
            if ( blocks[j+1][i] >= 0 && extra[j+1][i] == 0 ){
                Tree p;
                p.L = j+1;
                p.R = i;
                p.parent = mm;
                leaves.push_back(p);
                extra[p.L][p.R] = 1;
            }
            if ( blocks[j-1][i] >= 0  && extra[j-1][i] == 0){
                Tree p;
                p.L = j-1;
                p.R = i;
                p.parent = mm;
                leaves.push_back(p);
                extra[p.L][p.R] = 1;
            }

        }
        if (a) break;
    }       
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){
            extra[j][i] = 0;
        }
    } 
}

void FloodFill(int y, int x, int find, int change, int dist){
    extra[y][x] = 1;
    std::vector<Tree> leaves;

    Tree l;
    l.L = y;
    l.R = x;
    l.parent = -1;
    leaves.push_back(l);

    for(int n = 0; n < dist; n++){
        int a = 0;
        int mm = -1;
        for (auto elem : leaves){
            mm++;

            auto j = elem.L;
            auto i = elem.R;

            if ( j < 1 || i < 1 || j > sizey-1 || i > sizex-1) continue;
            if ( tiles[j][i+1] == find && extra[j][i+1] == 0 ){
                Tree p; p.L = j; p.R = i+1; p.parent = elem.parent+1; leaves.push_back(p); extra[p.L][p.R] = 1;
            }
            if ( tiles[j][i-1] == find && extra[j][i-1] == 0 ){
                Tree p; p.L = j; p.R = i-1; p.parent = elem.parent+1; leaves.push_back(p); extra[p.L][p.R] = 1;
            }
            if ( tiles[j+1][i] == find && extra[j+1][i] == 0 ){
                Tree p; p.L = j+1; p.R = i; p.parent = elem.parent+1; leaves.push_back(p); extra[p.L][p.R] = 1;
            }
            if ( tiles[j-1][i] == find && extra[j-1][i] == 0){
                Tree p; p.L = j-1; p.R = i; p.parent = elem.parent+1; leaves.push_back(p); extra[p.L][p.R] = 1;
            }

            if ( tiles[j+1][i+1] == find && extra[j+1][i+1] == 0 ){
                Tree p; p.L = j+1; p.R = i+1; p.parent = elem.parent+1; leaves.push_back(p); extra[p.L][p.R] = 1;
            }
            if ( tiles[j-1][i-1] == find && extra[j-1][i-1] == 0 ){
                Tree p; p.L = j-1; p.R = i-1; p.parent = elem.parent+1; leaves.push_back(p); extra[p.L][p.R] = 1;
            }
            if ( tiles[j+1][i-1] == find && extra[j+1][i-1] == 0 ){
                Tree p; p.L = j+1; p.R = i-1; p.parent = elem.parent+1; leaves.push_back(p); extra[p.L][p.R] = 1;
            }
            if ( tiles[j-1][i+1] == find && extra[j-1][i+1] == 0){
                Tree p; p.L = j-1; p.R = i+1; p.parent = elem.parent+1; leaves.push_back(p); extra[p.L][p.R] = 1;
            }
        }
        if (a) break;
    }  
    for (int k = 0; k < 2; k++){  //Two times to get the weird conditions   
        int l = 0; 
        for (auto elem : leaves){
            if (elem.parent > dist-2
                && surroundingTilesAdj(elem.L,elem.R,2) <= 2.5 
                && surroundingTilesAdj(elem.L,elem.R,2) != 1.5 
                && surroundingTilesAdj(elem.L,elem.R,3) == 0 
                && surroundingTilesAdj(elem.L,elem.R,5) == 0 ){
                tiles[elem.L][elem.R] = 2;
                extra[elem.L][elem.R] = 1;
            }else if (l == 0 && rand()%40 == 0
                && surroundingTilesAdj(elem.L,elem.R, 2) ){
                tiles[elem.L][elem.R] = 6; // lamp
                l = 1;
            }
        }
    }

}


void genRooms(){
    int p = rand()%10+10;
    for (int y = 0; y < sizey; y++){
        for (int x = 0; x < sizex; x++){
            if (x%p <= 2 && tiles[y][x] == 1){
                tiles[y][x] = 1; //becomes Floor
                extra[y][x] = 1; //Floor is different color
            }
        }
    }
}

bool checkMap(){
    int a = 0;
    for (int y = 0; y < sizey; y++){
        for (int x = 0; x < sizex; x++){
            if (tiles[y][x] == 5 && surroundingTilesAdj(y,x,4)){ //Check door near pavement
                a = 1;
            }
            if (tiles[y][x] == 2 && surroundingTilesAdj(y,x,2) == 3.5){
                return false;
            }
        }
    }
    if (a == 1)
        return true;
    
    return false;
}


int main(int argc, char *argv[]){

    srand(time(0));

	WINDOW* win = stdscr;
	setlocale(LC_ALL, "");

	initscr();
	cbreak(); noecho(); keypad(win, TRUE);
	clear();
	curs_set(0);
	start_color();
    // timeout(2000);

    while(1){

        genRandom();
        for(int n=0;n<50;n++)
            genFloorPlan();
        genWalls();
        genRoad();

        addTile(2,5, 1); // Wall to Door, adj to floor

        genColors(); //First color added in case colors needed for further tile generation
        for(int n = 0; n < 5; n++)
            addTile(2,3,  1); // Wall to window, Adj to Floor

        addTile(0,0, 5); //Grass to Grass, adj to door
        Dij(addy, addx, 4, 4); //y,x start, find Road, Change to Road along path

        for(int n = 0; n < 4; n++){
            addTile(1,1,1); //Floor to floor, adj to floor, size of fill
            FloodFill(addy, addx, 1,2,  rand()%5+3);
        }

        genColors();
        lightAll();

        if (checkMap()) break;
    }


    while(1){

        // refresh();      
        turns++;

    	for (int j = 0; j < LINES; j++){
		    for (int i = 0; i < COLS / 2; i++){

				int x = i + camx - COLS / 4; // + COLS / 2;
				int y = j + camy - LINES / 2; // + LINES / 2;

                if (x < 0 || y < 0 || x >= sizex || y >= sizey) continue;

                //Refresh Display Tile
                fresh_all = 1;   
                if (fresh[y][x] == 1 || fresh_all == 1){
                    fresh[y][x] = 0;

                    float v = light_arr[y][x];

                    if (x == px && y == py){ //Display Player
                        printT(i*2,j, "Al" ,255,255,1,  200,200,1);
                    }else{ //Display Tile
                        printT(i*2,j, word[y][x] ,f_r[y][x]*v, f_g[y][x]*v, f_b[y][x]*v, 
                            b_r[y][x]*v, b_g[y][x]*v, b_b[y][x]*v);
                    }

                    //Animate Tile
                    /*if (tiles[j][i] == 1){
                        if (word[j][i] == " ,") word[j][i] = " .";
                        else word[j][i] = " ,";
                    }*/

                }

		    }

		}
        
    	ch = getch();

        int oldx = px; 
        int oldy = py;
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

        if (oldx != px || oldy != py){
            fresh[oldy][oldx] = 1; 
            fresh[py][px] = 1; 
        }

        if (px < 0 || px >= sizex || py < 0 || py >= sizey){
            px = oldx; py = oldy;
        }

        //Update camera position if moved too far 
        oldx = camx;
        oldy = camy;

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
        if (camx != oldx || camy != oldy){
            camx = px; camy = py;
        }
        // camx = px;
        // camy = py;
        if (camx < COLS/4) camx = COLS/4;
        if (camy < LINES/2) camy = LINES/2;
        if (camx > sizex-COLS/4) camx = sizex-COLS/4;
        if (camy > sizey-LINES/2) camy = sizey-LINES/2;

        if (fresh_all > 0) fresh_all--;
        if (oldx != camx || oldy != camy) fresh_all = 1;

    }

	endwin();
}

