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

int px = 16;
int py = 16;
int camx = 16;
int camy = 16;

struct Vector3 {
    int L;
    int R;
    int index;
};

void genChunk(int height, int width){

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

    int ch;
    int turns = 0;

    int cols2 = COLS/2;
    int tiles[LINES][cols2];
    string word[LINES][cols2];
    int fresh[LINES][cols2];
    int f_r[LINES][cols2];
    int f_g[LINES][cols2];
    int f_b[LINES][cols2];
    int b_r[LINES][cols2];
    int b_g[LINES][cols2];
    int b_b[LINES][cols2];

    

    for (int j = 0; j < LINES; j++){
        for (int i = 0; i < COLS / 2; i++){
            fresh[j][i] = 1;
            tiles[j][i] = rand() % 5;
            word[j][i] = "  ";
            if (tiles[j][i] == 1){
                int r = rand() % 4;
                if (r == 0) word[j][i] = " '";
                if (r == 1) word[j][i] = " ,";
                if (r == 2) word[j][i] = "' ";
                if (r == 3) word[j][i] = ", ";
            }
            f_r[j][i] = 100;
            f_g[j][i] = 255-rand() % 100;
            f_b[j][i] = 100;
            b_r[j][i] = 1;
            b_g[j][i] = rand() % 25+75;
            b_b[j][i] = 1;
        }
    }
    // ch = getch();

    while(1){

        // refresh();      
        turns++;

    	for (int j = 0; j < LINES; j++){
		    for (int i = 0; i < COLS / 2; i++){

				int x = i + camx - COLS / 4; // + COLS / 2;
				int y = j + camy - LINES / 2; // + LINES / 2;

                //Player
                if (x == px && y == py){
                    printT(i*2,j, "Al" ,255,255,1,  200,200,1);
                    continue;
                }

                //Refresh
                if (fresh[j][i] == 1){
                    fresh[j][i] = 1;
                    printT(i*2,j, word[j][i] ,f_r[j][i], f_g[j][i], f_b[j][i], b_r[j][i], b_g[j][i], b_b[j][i]);

                    /*if (tiles[j][i] == 1){
                        if (word[j][i] == " ,") word[j][i] = " .";
                        else word[j][i] = " ,";
                    }*/
                }

		    }

		}
        
    	ch = getch();

    	if (ch == 68){ //Left
    		fresh[px][py] = 1; px--;
    	}
    	if (ch == 67){ //Right
    		fresh[px][py] = 1; px++;
    	}
    	if (ch == 65){ //Up
    		fresh[px][py] = 1; py--;
    	}
    	if (ch == 66){ //Down
    		fresh[px][py] = 1; py++;
    	}
    	if (ch == KEY_F(1))
    		break;

        //Update camera position if moved too far 
        int oldx = camx;
        int oldy = camy;

        if (px-camx > COLS / 6){
            camx = px + COLS / 6;
        }
        if (px-camx < -COLS / 6){
            camx = px - COLS / 6;
        }
        if (py-camy > LINES / 3){
            camy = py + LINES / 3;
        }
        if (py-camy < -LINES / 3){
            camy = py - LINES / 3;
        }

    }

	endwin();
}

