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
    printf("\033[48;2;%d;%d;%dm", B_R, B_G, B_B);
    printf("\033[38;2;%d;%d;%dm", F_R, F_G, F_B);
    printf("%s\033[0m", s.c_str());
}

int ch;
int turns = 0;
int fresh_all = 2;
const int sizex = 128;
const int sizey = 128;
const int sizez = 16;

int px = 64;
int py = 64;
int pz = 8;
int camx = px;
int camy = py;
int camz = pz;

int tiles[sizez][sizey][sizex];
string word[sizez][sizey][sizex];
int fresh[sizez][sizey][sizex];
int f_r[sizez][sizey][sizex];
int f_g[sizez][sizey][sizex];
int f_b[sizez][sizey][sizex];
int b_r[sizez][sizey][sizex];
int b_g[sizez][sizey][sizex];
int b_b[sizez][sizey][sizex];
float light_arr[sizez][sizey][sizex];
int blocks[sizez][sizey][sizex];
int extra[sizez][sizey][sizex];

float surroundingTilesFour(int y, int x, int against){
    float a = 0;
    if (tiles[camz][y][x+1] == against) a+=1;
    if (tiles[camz][y][x-1] == against) a+=1;
    if (tiles[camz][y+1][x] == against) a+=1;
    if (tiles[camz][y-1][x] == against) a+=1;
    return a;
}

float surroundingTilesAdj(int z, int y, int x, int against){
    float a = 0;
    if (tiles[z][y][x+1] == against) a+=1;
    if (tiles[z][y][x-1] == against) a+=1;
    if (tiles[z][y+1][x] == against) a+=1;
    if (tiles[z][y-1][x] == against) a+=1;
    if (tiles[z][y+1][x+1] == against) a+=.25;
    if (tiles[z][y-1][x+1] == against) a+=.25;
    if (tiles[z][y+1][x-1] == against) a+=.25;
    if (tiles[z][y-1][x-1] == against) a+=.25;
    return a;
}

void rgb(int z, int y, int x, int a, int b, int c, int d, int e, int f){
    f_r[z][y][x] = a;
    f_g[z][y][x] = b;
    f_b[z][y][x] = c;
    b_r[z][y][x] = d;
    b_g[z][y][x] = e;
    b_b[z][y][x] = f;
}


float calcLighting(int posx, int posy, int posz, int x, int y, int z, float brightness){

    float taper = 1.0 / brightness;

    //If tile too far, auto no light
    float a = posx - x;
    float b = posy - y;
    // float e = posz - z; e*e
    float c = 1-sqrt(a*a+b*b) * taper;
    if (c <= 0){ 
        return 0;
    }

    //Ray Tracing
    int blocking = 0;
    for (float d = 0.0; d < brightness; d+= c*.9 ){
        float ry = posy-y; float rx = posx-x; //float rz = posz-z;
        ry *= (d*taper); rx *= (d*taper);  //rz *= (d*taper)*2;
        int xl = posx-int(rx);
        int yl = posy-int(ry);
        int zl = posz;
        // int zl = posz-int(rz);
        if ( xl < 0 || yl < 0 || xl >= sizex || yl >= sizey ){ blocking = 1; break; }
        if ( blocks[zl][yl][xl] < 0 ){
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

    for (int z = 0; z < sizez; z++) {
        for (int y = 0; y < sizey; y++) {
            for (int x = 0; x < sizex; x++) {
                light_arr[z][y][x] = 0;
            }
        }
    }


    //Calc Lighting
    for (int zz = 0; zz < sizez; zz++) { 
    // int zz = camz;
    for (int yy = 0; yy < sizey; yy++) {
    for (int xx = 0; xx < sizex; xx++) {

        if ( blocks[zz][yy][xx] == 2 ){ // Window or Lamp
            // for (int z = 0; z < sizez; z++){
            int z = zz;
                for (int y = 0; y < sizey; y++) {
                    for (int x = 0; x < sizex; x++) {
                        float value;
                        if (blocks[z][y][x] == 4 && light_arr[z][y][x] == 0) 
                            light_arr[z][y][x] += 0.4;

                        if ( blocks[z][y][x] == 0 || blocks[z][y][x] == -1 ) //Always lit
                            value = 1;
                        else{
                            value = calcLighting(xx,yy,zz, x, y, z, 10);                    
                            if (value <= 0) value = 0;
                        }
                        light_arr[z][y][x] += value;
                        if (light_arr[z][y][x] > 1) light_arr[z][y][x] = 1;
                    }
                }
            // }
        }else{
            if ( blocks[zz][yy][xx] == 0 || blocks[zz][yy][xx] == -1 ){ //Always lit
                light_arr[zz][yy][xx] = 1;
            }
        }

    }}}
}

float sky_r = 1; float sky_g = 150; float sky_b = 250;

void genRandom(int z){
    // int sky = rand()%3;
    // if (sky == 1){ sky_r = 250;  sky_g = 150;  sky_b = 250; }
    // if (sky == 2){ sky_r = 250;  sky_g = 250;  sky_b = 1; }

    int w = rand()%5+16;
    int h = rand()%(22-w)+11;
    for (int y = 0; y < sizey; y++){
        for (int x = 0; x < sizex; x++){

            //All to 0
            if (z > 8) tiles[z][y][x] = -1; // Air
            if (z < 8) tiles[z][y][x] = 4; // pavement
            blocks[z][y][x] = 0;
            fresh[z][y][x] = 0;
            extra[z][y][x] = 0;
            word[z][y][x] = "  ";

            if (z == 8){ // First Level
                tiles[z][y][x] = 0;
                blocks[z][y][x] = 0;
                word[z][y][x] = "  ";

                if (x > int(sizex/2)-w && x < int(sizex/2)+w && y > int(sizey/2-h) && y < int(sizey/2)+h){
                    tiles[z][y][x] = rand() % 3;
                    if (tiles[z][y][x] == 2) tiles[z][y][x] = 1;
                }
            }
            if (z > 8 + 2){ // Clouds form above
                if (rand()%2){
                    tiles[z][y][x] = -2;
                }
            }

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
            if (tiles[camz][y][x] != alt) a += 1;
            if (tiles[camz][y][x + 3] != alt) a += 1;
            if (tiles[camz][y][x - 2] != alt) a += 1;
            if (tiles[camz][y + 3][x] != alt) a += 1;
            if (tiles[camz][y - 2][x] != alt) a += 1;

            // if (tiles[y + 1][x + 1] != alt) a += 1;
            // if (tiles[y + 1][x - 1] != alt) a += 1;
            // if (tiles[y - 1][x + 1] != alt) a += 1;
            // if (tiles[y - 1][x - 1] != alt) a += 1;

            tiles[camz][y][x] = alt;
            if (a >= 3) tiles[camz][y][x] = org;
        }


    }

}

string chooseWall(int c, int z, int y, int x, int rando){
    string a = "╬ "; //4-way intersect

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
    if (tiles[z][y][x+1] != c && tiles[z][y][x-1] != c && tiles[z][y+1][x] != c && tiles[z][y-1][x] != c) a = "╬ "; //single pillar █
    if (tiles[z][y][x+1] != c && tiles[z][y][x-1] != c && tiles[z][y+1][x] == c && tiles[z][y-1][x] == c) a = "║ "; //vertical
    if (tiles[z][y][x+1] == c && tiles[z][y][x-1] == c && tiles[z][y+1][x] != c && tiles[z][y-1][x] != c) a = "══"; //horizontal
    if (tiles[z][y][x+1] == c && tiles[z][y][x-1] == c && tiles[z][y+1][x] == c && tiles[z][y-1][x] == c) a = "╬═"; //4-way

    if (tiles[z][y][x+1] != c && tiles[z][y][x-1] == c && tiles[z][y+1][x] == c && tiles[z][y-1][x] == c) a = "╣ "; //3-way, no right
    if (tiles[z][y][x+1] == c && tiles[z][y][x-1] != c && tiles[z][y+1][x] == c && tiles[z][y-1][x] == c) a = "╠═"; //3-way, no left
    if (tiles[z][y][x+1] == c && tiles[z][y][x-1] == c && tiles[z][y+1][x] != c && tiles[z][y-1][x] == c) a = "╩═"; //3-way, no top
    if (tiles[z][y][x+1] == c && tiles[z][y][x-1] == c && tiles[z][y+1][x] == c && tiles[z][y-1][x] != c) a = "╦═"; //3-way, no bottom

    if (tiles[z][y][x+1] != c && tiles[z][y][x-1] == c && tiles[z][y+1][x] != c && tiles[z][y-1][x] == c) a = "╝ "; //2-way, no right bottom
    if (tiles[z][y][x+1] == c && tiles[z][y][x-1] != c && tiles[z][y+1][x] != c && tiles[z][y-1][x] == c) a = "╚═"; //2-way, no left bottom
    if (tiles[z][y][x+1] != c && tiles[z][y][x-1] == c && tiles[z][y+1][x] == c && tiles[z][y-1][x] != c) a = "╗ "; //2-way, no right top
    if (tiles[z][y][x+1] == c && tiles[z][y][x-1] != c && tiles[z][y+1][x] == c && tiles[z][y-1][x] != c) a = "╔═"; //2-way, no left top

    if (tiles[z][y][x+1] == c && tiles[z][y][x-1] != c && tiles[z][y+1][x] != c && tiles[z][y-1][x] != c) a = "══";
    if (tiles[z][y][x+1] != c && tiles[z][y][x-1] == c && tiles[z][y+1][x] != c && tiles[z][y-1][x] != c) a = "══";
    if (tiles[z][y][x+1] != c && tiles[z][y][x-1] != c && tiles[z][y+1][x] != c && tiles[z][y-1][x] == c) a = "║ "; //2

    return a;
}


void genRoad(){
    int shift = rand()%15-7;
    if (shift == 0) shift = 1;
    for (int y = 0; y < sizey; y++) {
        for (int x = 0; x < sizex; x++) {
            int ys = (y/shift)-(sizey/2/shift)-3;
            if (x <= sizex/2-26+ys && x >= sizex/2-32+ys){
                tiles[camz][y][x] = 4; //Road
                if (x == sizex/2-29+ys && y%2==0) word[camz][y][x] = " .";
                int s = rand()%16; int d = 40;
                if (x == sizex/2-26+ys || x == sizex/2-32+ys) d = 0;
                rgb(camz, y,x, 200,200,200,  120-s-d,120-s-d,120-s-d);
            }
        }
    }
}

float searchBelow(int z, int y, int x, int six[], string& w){
    for (int dist = 1; dist < z; dist++){
        if (tiles[z-dist][y][x] > -1){ //Find a tile  that is not air
            six[0] = f_r[z-dist][y][x];
            six[1] = f_g[z-dist][y][x];
            six[2] = f_b[z-dist][y][x];
            six[3] = b_r[z-dist][y][x];
            six[4] = b_g[z-dist][y][x];
            six[5] = b_b[z-dist][y][x];
            w = word[z-dist][y][x];
            return dist;
        }
    }
    return 0;
}

void genColors(){

    for (int z = 0; z < sizez; z++) {
        for (int y = 0; y < sizey; y++) {
            for (int x = 0; x < sizex; x++) {
                // int z = camz;

                if (tiles[z][y][x] == -1){ //Air
                    blocks[z][y][x] = 4; //Half light naturally
                    int r = rand() % 15;
                    int six[6];
                    float d = searchBelow(z,y,x,six, word[z][y][x])+1;
                    if (d > 2) d = 2;

                    for(int n = 0; n < 3; n++)
                        six[n] = (six[0] + six[1] + six[2])/3;
                    for(int n = 3; n < 6; n++)
                        six[n] = (six[4] + six[5] + six[3])/3;

                    six[0]*=1.0/d; six[1]*=1.0/d; six[2]*=1.0/d;
                    six[3]*=1.0/d; six[4]*=1.0/d; six[5]*=1.0/d;

                    six[0] = six[0] + (sky_r+r)*(d/6); six[1] = six[1] + (sky_g+r)*(d/6); six[2] = six[2] + (sky_b+r)*(d/6);
                    six[3] = six[3] + (sky_r+r)*(d/6); six[4] = six[4] + (sky_g+r)*(d/6); six[5] = six[5] + (sky_b+r)*(d/6);
                    if (six[0] > sky_r) six[0] = sky_r+r;
                    if (six[1] > sky_g) six[1] = sky_g+r;
                    if (six[2] > sky_b) six[2] = sky_b+r;
                    if (six[3] > sky_r) six[3] = sky_r+r;
                    if (six[4] > sky_g) six[4] = sky_g+r;
                    if (six[5] > sky_b) six[5] = sky_b+r;
                    rgb(z,y,x, six[0], six[1], six[2],   six[3], six[4], six[5]);
                }
                else if (tiles[z][y][x] == -2){ //Cloud
                    blocks[z][y][x] = 4; //No light naturally
                    word[z][y][x] = "()";
                    int s = rand()%15;
                    rgb(z,y,x, 254,254,254,  254-s,254-s,254-s);
                }

                int f = fresh[z][y][x]*15; 
                if (tiles[z][y][x] == 0){ //Grass
                    int r = rand() % 25;
                    if (r == 0) word[z][y][x] = " '";
                    if (r == 1) word[z][y][x] = " ,";
                    if (r == 2) word[z][y][x] = "' ";
                    if (r == 3) word[z][y][x] = ", "; 
                    rgb(z, y,x, 100-f, 255-rand() % 100-f, 100-f,   1, rand() % 25+75-f, 1);
                }
                else if (tiles[z][y][x] == 1){ //Floor
                    word[z][y][x] = "  "; 
                    blocks[z][y][x] = 3; //No light naturally
                    int s = rand()%15; //35+100;
                    rgb(z,y,x, 180,150,80,  180-s,150-s,80-s);
                    if (extra[z][y][x] == 1)
                        rgb(z,y,x, 180,150,80,  125-s,75-s,40-s);
                }
                else if (tiles[z][y][x] == 2){ //Wall
                    word[z][y][x] = chooseWall(2, z,y,x,0);
                    blocks[z][y][x] = -1; //Blocks and is always lit
                    if (extra[z][y][x] == 1) 
                        blocks[z][y][x] = -2; //Blocks, unlit
                    if (surroundingTilesAdj(z,y,x,0) >= 2 && z == 8){ //2 for fancy effect
                        rgb(z,y,x, 200,200,200,  1, rand() % 25+75-f, 1);
                    }else{
                        int s = rand()%15;//35+100-35;
                        rgb(z,y,x, 200,200,200,  160-s,160-s,160-s);
                    }
                    fresh[z][y+1][x+1] = 1;
                }
                else if (tiles[z][y][x] == 3){ //Window
                    blocks[z][y][x] = 2; //Generate light
                    word[z][y][x] = chooseWall(2, z,y,x,0);
                    rgb(z,y,x, 1,45*3,65*3,  1,65,85);
                }
                else if (tiles[z][y][x] == 4){ //Road //Pavement
                    blocks[z][y][x] = 0;
                    if (f_r[z][y][x] != 200){ //Prevent Previously placed road from changing
                        // word[z][y][x] = "  ";
                        int s = rand()%15;
                        rgb(z,y,x, 250,250,250,  120-s,120-s,120-s);
                    }
                }
                else if (tiles[z][y][x] == 5){ //Door
                    blocks[z][y][x] = -1; //Blocks light and is always lit
                    word[z][y][x] = "[]";
                    int s = rand()%15;
                    rgb(z,y,x, 100,200,200,  60-s,110-s,110-s);
                }
                else if (tiles[z][y][x] == 6){ //Lamp
                    blocks[z][y][x] = 2; //Generate light
                    word[z][y][x] = "|^";
                    int s = rand()%15;
                    rgb(z,y,x, 255,255,1,  180-s,150-s,80-s);
                    if (extra[z][y][x] == 1)
                        rgb(z,y,x, 255,255,1,  125-s,75-s,40-s);
                }

                fresh[z][y][x] = 0;
                extra[z][y][x] = 0;
            }
        }
    }
}

void genWalls(){
    for (int y = 0; y < sizey; y++) {
        for (int x = 0; x < sizex; x++) {
            int z = camz;
            if (tiles[z][y][x] == 1){ //Floor
                if (surroundingTilesAdj(z,y,x,0)){
                    tiles[z][y][x] = 2; // Wall;
                }
            }
        }
    }

}


int Determine(int against, int adj){
    int count = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){
            int z = camz;

            int f = surroundingTilesAdj(z,j,i,adj);

            if ( tiles[z][j][i] == against && surroundingTilesFour(j,i,adj) && floor(f) == f){
                int a = 0;
                if ( tiles[z][j+1][i] == against || tiles[z][j-1][i] == against 
                    || tiles[z][j][i+1] == against || tiles[z][j][i-1] == against ) a++;
                if ( a == 1 ) count++;
            }
                
        }
    }
    return count;
}

int addx = 0;
int addy = 0;
int addz = 0;

void Add(int against, int replace, int find, int adj){
    int count = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){
            int z = camz;

            int f = surroundingTilesAdj(z,j,i,adj);

            if ( tiles[z][j][i] == against && surroundingTilesFour(j,i,adj) && floor(f) == f){
                int a = 0;
                if ( tiles[z][j+1][i] == against || tiles[z][j-1][i] == against 
                    || tiles[z][j][i+1] == against || tiles[z][j][i-1] == against ) a++;
                if ( a == 1 ) count++;
            }
            
            if ( count == find ){
                tiles[z][j][i] = replace;
                addz = z; addx = i; addy = j;
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




void addTileAbove(int z, int search, int replace){
    int count, find, n;
    count = 0;
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){
            if ( tiles[z][j][i] == search ){
                count++;
            }
        }
    }
    if (count == 0) return;
    find = rand() % count+1; count = 0;

    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){
            if ( tiles[z][j][i] == search ){
                count++;
            }
            if (count == find){
                tiles[z+1][j][i] = replace;
                addz = z+1; addx = i; addy = j;
                return;
            }
        }
    }
}


struct Tree {
    int L;
    int R;
    int index;
    int parent;
};

void Dij(int y, int x, int find, int change){
    int z = camz;
    extra[z][y][x] = 1;
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

            if ( tiles[z][j][i] == find ) {
                int point = mm;
                while(1){
                    // cout << point << endl; getch();
                    int yy = leaves[point].L;
                    int xx = leaves[point].R;
                    tiles[z][yy][xx] = 4;
                    px = xx; py = yy;
                    point = leaves[point].parent;
                    if (point == -1) break;
                }
                a = 1;
                break;
            }

            if ( j < 1 || i < 1 || j > sizey-1 || i > sizex-1) continue;
            if ( blocks[z][j][i+1] >= 0 && extra[z][j][i+1] == 0 ){
                Tree p;
                p.L = j;
                p.R = i+1;
                p.parent = mm;
                leaves.push_back(p);
                extra[z][p.L][p.R] = 1;
                // word[p.L][p.R] = mm;
            }
            if ( blocks[z][j][i-1] >= 0 && extra[z][j][i-1] == 0 ){
                Tree p;
                p.L = j;
                p.R = i-1;
                p.parent = mm;
                leaves.push_back(p);
                extra[z][p.L][p.R] = 1;
            }
            if ( blocks[z][j+1][i] >= 0 && extra[z][j+1][i] == 0 ){
                Tree p;
                p.L = j+1;
                p.R = i;
                p.parent = mm;
                leaves.push_back(p);
                extra[z][p.L][p.R] = 1;
            }
            if ( blocks[z][j-1][i] >= 0  && extra[z][j-1][i] == 0){
                Tree p;
                p.L = j-1;
                p.R = i;
                p.parent = mm;
                leaves.push_back(p);
                extra[z][p.L][p.R] = 1;
            }

        }
        if (a) break;
    }       
    for (int j = 0; j < sizey; j++){
        for (int i = 0; i < sizex; i++){
            extra[z][j][i] = 0;
        }
    } 
}

int flood_floor = 0;

void FloodFill(int z, int y, int x, int find, int change, int dist){
    extra[z][y][x] = 1;
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

            //Creating new floors above, when to stop expanding
            if (flood_floor && tiles[z-1][j][i] == 0) continue;


            if ( tiles[z][j][i+1] == find && extra[z][j][i+1] == 0){
                Tree p; p.L = j; p.R = i+1; p.parent = elem.parent+1; leaves.push_back(p); extra[z][p.L][p.R] = 1;
            }
            if ( tiles[z][j][i-1] == find && extra[z][j][i-1] == 0 ){
                Tree p; p.L = j; p.R = i-1; p.parent = elem.parent+1; leaves.push_back(p); extra[z][p.L][p.R] = 1;
            }
            if ( tiles[z][j+1][i] == find && extra[z][j+1][i] == 0 ){
                Tree p; p.L = j+1; p.R = i; p.parent = elem.parent+1; leaves.push_back(p); extra[z][p.L][p.R] = 1;
            }
            if ( tiles[z][j-1][i] == find && extra[z][j-1][i] == 0){
                Tree p; p.L = j-1; p.R = i; p.parent = elem.parent+1; leaves.push_back(p); extra[z][p.L][p.R] = 1;
            }

            if ( tiles[z][j+1][i+1] == find && extra[z][j+1][i+1] == 0 ){
                Tree p; p.L = j+1; p.R = i+1; p.parent = elem.parent+1; leaves.push_back(p); extra[z][p.L][p.R] = 1;
            }
            if ( tiles[z][j-1][i-1] == find && extra[z][j-1][i-1] == 0 ){
                Tree p; p.L = j-1; p.R = i-1; p.parent = elem.parent+1; leaves.push_back(p); extra[z][p.L][p.R] = 1;
            }
            if ( tiles[z][j+1][i-1] == find && extra[z][j+1][i-1] == 0 ){
                Tree p; p.L = j+1; p.R = i-1; p.parent = elem.parent+1; leaves.push_back(p); extra[z][p.L][p.R] = 1;
            }
            if ( tiles[z][j-1][i+1] == find && extra[z][j-1][i+1] == 0){
                Tree p; p.L = j-1; p.R = i+1; p.parent = elem.parent+1; leaves.push_back(p); extra[z][p.L][p.R] = 1;
            }
        }
        if (a) break;
    }  
    if (flood_floor == 0){
        for (int k = 0; k < 2; k++){  //Two times to get the weird conditions   
            int l = 0; 
            for (auto elem : leaves){
                if (elem.parent > dist-2
                    && surroundingTilesAdj(z,elem.L,elem.R,2) <= 2.5 
                    && surroundingTilesAdj(z,elem.L,elem.R,2) != 1.5 
                    && surroundingTilesAdj(z,elem.L,elem.R,3) == 0 
                    && surroundingTilesAdj(z,elem.L,elem.R,5) == 0 ){
                    tiles[z][elem.L][elem.R] = 2; //Wall on edges
                    extra[z][elem.L][elem.R] = 1;
                }else if (l == 0 && rand()%40 == 0
                    && surroundingTilesAdj(z, elem.L,elem.R, 2) ){
                    tiles[z][elem.L][elem.R] = 6; // lamp
                    l = 1;
                }

            }
        }
    }else{
        //Handles case of creating a new floor above
        int l = 0; 
        for (auto elem : leaves){
            if (elem.parent > dist-2){
                tiles[z][elem.L][elem.R] = 2; // wall on border
                extra[z][elem.L][elem.R] = 0; // makes it lit
                if (rand()%20 == 0){
                    tiles[z][elem.L][elem.R] = 3;  // occasional window
                }
            }else{
                tiles[z][elem.L][elem.R] = 1; // floor
                extra[z][elem.L][elem.R] = 0;
                if (l == 0 && rand()%40 == 0){
                    l = 1; tiles[z][elem.L][elem.R] = 6;  // single lamp
                }
            }

        }
        //Floors next to air beccome walls
        for (int y = 0; y < sizey; y++){
            for (int x = 0; x < sizex; x++){
                if (tiles[z][y][x] == 1 && surroundingTilesAdj(z,y,x,-1) ){
                    tiles[z][y][x] = 2;
                }
            }
        }


    }

}


void genRooms(){
    int p = rand()%10+10;
    for (int y = 0; y < sizey; y++){
        for (int x = 0; x < sizex; x++){
            int z = camz;
            if (x%p <= 2 && tiles[z][y][x] == 1){
                tiles[z][y][x] = 1; //becomes Floor
                extra[z][y][x] = 1; // Floor is different color
            }
        }
    }
}

bool checkMap(){
    int a = 0;
    for (int y = 0; y < sizey; y++){
        for (int x = 0; x < sizex; x++){
            int z = camz;
            if (tiles[z][y][x] == 5 && surroundingTilesAdj(z,y,x,4)){ //Check door near pavement
                a = 1;
            }
            if (tiles[z][y][x] == 2 && surroundingTilesAdj(z,y,x,2) == 3.5){
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

    int twice_draw = 0;

    while(1){

        for (int z = 0; z < sizez; z++){
            genRandom(z);
        }
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
            FloodFill(addz, addy, addx, 1,2,  rand()%5+3);
        }

        //Add floor tiles and walls above
        for(int n = 0; n < 1; n++){
            addTileAbove(camz, 1,1); //Floor on cur level, Floor on level above
            flood_floor = 1;
            FloodFill(addz, addy, addx, -1,2,  rand()%5+10);
            flood_floor = 0;
        }
        int z = camz;
        //Second floor, set roof of first floor
        for (int y = 0; y < sizey; y++){
            for (int x = 0; x < sizex; x++){
                if (tiles[z+1][y][x] == -1){ // Air above
                    // floor or wall
                    if (tiles[z][y][x] == 1 || surroundingTilesAdj(z,y,x, 1) ){ 
                        tiles[z+1][y][x] = 4;
                        word[z+1][y][x] = word[z][y][x];
                    }
                }
            }
        }

        //Replicate second floor several times
        for(int mult_floors = 0; mult_floors < 1; mult_floors++){
            int z = camz+1+mult_floors;
            for (int y = 0; y < sizey; y++){
                for (int x = 0; x < sizex; x++){
                    if (tiles[z][y][x] != 4 && tiles[z][y][x] != -2){
                        tiles[z+1][y][x] = tiles[z][y][x];
                    }
                }
            }
        }

        genColors();
        lightAll();

        if (checkMap()) break;
    }


    while(1){

        // refresh();      
        turns++;
        for (int times = 0; times < 1+twice_draw; times++){

    	for (int j = 0; j < LINES; j++){
		    for (int i = 0; i < COLS / 2; i++){
                int z = camz;

				int x = i + camx - COLS / 4; // + COLS / 2;
				int y = j + camy - LINES / 2; // + LINES / 2;

                if (x < 0 || y < 0 || x >= sizex || y >= sizey) continue;

                //Refresh Display Tile
                fresh_all = 1;   
                if (fresh[z][y][x] == 1 || fresh_all == 1){
                    fresh[z][y][x] = 0;

                    float v = light_arr[z][y][x];

                    if (x == px && y == py){ //Display Player
                        printT(i*2,j, "Al" ,255,255,1,  200,200,1);
                    }else{ //Display Tile
                        printT(i*2,j, word[z][y][x] ,f_r[z][y][x]*v, f_g[z][y][x]*v, f_b[z][y][x]*v, 
                            b_r[z][y][x]*v, b_g[z][y][x]*v, b_b[z][y][x]*v);
                    }

                    //Animate Tile
                    /*if (tiles[j][i] == 1){
                        if (word[j][i] == " ,") word[j][i] = " .";
                        else word[j][i] = " ,";
                    }*/

                }

		    }

		}
        }
        
    	ch = getch();
        twice_draw = 0;

        int oldx = px; 
        int oldy = py;
        int oldz = pz;
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
        if (ch == 92){ //Top
            twice_draw = 2;
            pz++;
        }
        if (ch == 47){ //Bottom
            twice_draw = 2;
            pz--;
        }
    	if (ch == KEY_F(1))
    		break;

        if (oldx != px || oldy != py || oldz != pz){
            fresh[oldz][oldy][oldx] = 1; 
            fresh[pz][py][px] = 1; 
        }

        if (px < 0 || px >= sizex || py < 0 || py >= sizey || pz < 0 || pz >= sizez){
            px = oldx; py = oldy; pz = oldz;
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

        if (camx < COLS/4) camx = COLS/4;
        if (camy < LINES/2) camy = LINES/2;
        if (camx > sizex-COLS/4) camx = sizex-COLS/4;
        if (camy > sizey-LINES/2) camy = sizey-LINES/2;
        camz = pz;

        if (fresh_all > 0) fresh_all--;
        if (oldx != camx || oldy != camy) fresh_all = 1;

    }

	endwin();
}

