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

#define AWK "\
tput cup $2 $1 \n\
awk -v TILE1=$3 -v TILE2=$4 -v F_R=$5 -v F_G=$6 -v F_B=$7 -v B_R=$8 -v B_G=$9 -v B_B=$10  \'BEGIN { \n\
    s = TILE1 TILE2; \n\
    if (s == \"ss\"){ \n\
        F_R = B_R; \n\
        F_G = B_G; \n\
        F_B = B_B; \n\
    } \n\
    printf \"\033[48;2;%d;%d;%dm\", B_R, B_G, B_B; \n\
    printf \"\033[38;2;%d;%d;%dm\", F_R, F_G, F_B; \n\
    printf \"%s\033[0m\", s; \n\
}\' \n\
"


#define AWK2 "\
#/bin/bash \n\
#clear \n\
echo \"$1\" \n\
awk -v COLS=100 \'BEGIN{ \n\
	for (i = 0; i<COLS; i++) { \n\
		s = \"kacz[]|\"; s = s s s s s s s s;\n\
		printf \"\033[48;2;%d;%d;%dm\", i,i*2,i*3; \n\
		printf \"\033[38;2;%d;%d;%dm\", i*3,i*2,i*1; \n\
		printf \"%s\033[0m\", substr(s,i+1,1); \n\
	} \n\
}\' \n\
"

int px = 16;
int py = 16;
int camx = 16;
int camy = 16;

struct Vector3 {
    int L;
    int R;
    int index;
};

int main(int argc, char *argv[]){

    srand(time(0));
    // posix_spawnattr_setflags(&attr, POSIX_SPAWN_USEVFORK);

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

    int ch = 0;
    int turns = 0;

    int tiles[COLS][LINES];
    int fresh[COLS][LINES];
    int f_r[COLS][LINES];
    int f_g[COLS][LINES];
    int f_b[COLS][LINES];
    int b_r[COLS][LINES];
    int b_g[COLS][LINES];
    int b_b[COLS][LINES];

    for (int n = 0; n < LINES; n++){
        for (int m = 0; m < COLS / 2; m++){
            fresh[m][n] = 1;
            tiles[m][n] = rand() % 2;
            f_r[m][n] = 100;
            f_g[m][n] = 255;
            f_b[m][n] = 100;
            b_r[m][n] = 1;
            b_g[m][n] = rand() % 25+75;
            b_b[m][n] = 1;
        }
    }
    int on_line = 0;

    while(1){

        refresh();        

    	for (int j = 0; j < LINES; j++){
		    for (int i = 0; i < COLS / 2; i++){

				int x = i + camx - COLS / 4; // + COLS / 2;
				int y = j + camy - LINES / 2; // + LINES / 2;

                //Refresh
                if (fresh[i][j] == 1){
                    fresh[i][j] = 0;

                    int t = tiles[i][j];
                    char s1 = 's';
                    char s2 = 's';
                    if (t == 1){ s1 = ','; s2 = ','; }//Grass

                    char bar[750];
                    snprintf(bar, 750, "echo %d %d %c %c %d %d %d %d %d %d | (read a b c d e f g h i j; \n\
tput cup $b $a \n\
awk -v TILE1=$c -v TILE2=$d -v F_R=$e -v F_G=$f -v F_B=$g -v B_R=$h -v B_G=$i -v B_B=$j  \'BEGIN { \n\
    printf \"*\"; \n\
}\' \n\
) \n\
            ", 
                        i*2, j, s1, s2, 
                        f_r[j][i], f_g[j][i], f_b[j][i], b_r[j][i], b_g[j][i], b_b[j][i]);
                    

                    if (!vfork()){
                        execl("/bin/sh", "sh", "-c", bar, NULL);
                        // execv(, NULL);
                        exit(1);
                    }else{
                        wait(NULL);
                    }

                }

		    }

		}

        //Player
        char bar[850];
        snprintf(bar, 850, "echo %d %d %c %c %d %d %d %d %d %d | (read a b c d e f g h i j; \n\
tput cup $b $a \n\
awk -v TILE1=$c -v TILE2=$d -v F_R=$e -v F_G=$f -v F_B=$g -v B_R=$h -v B_G=$i -v B_B=$j  \'BEGIN { \n\
    s = TILE1 TILE2; \n\
    if (s == \"ss\"){ \n\
        F_R = B_R; \n\
        F_G = B_G; \n\
        F_B = B_B; \n\
    } \n\
    printf \"\033[48;2;%d;%d;%dm\", B_R, B_G, B_B; \n\
    printf \"\033[38;2;%d;%d;%dm\", F_R, F_G, F_B; \n\
    printf \"%s\033[0m\", s; \n\
}\' \n\
) \n\
            ", 
            px*2, py, 'A', 'l',
            255,255,1, 200,200,1);
        system(bar);

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

