tput cup $2 $1
awk -v TILE1=$3 -v TILE2=$4 -v F_R=$5 -v F_G=$6 -v F_B=$7 -v B_R=$8 -v B_G=$9 -v B_B=$10  'BEGIN {

	s = TILE1 TILE2;

    if (s == "ss"){
    	F_R = B_R;
    	F_G = B_G;
    	F_B = B_B;
    }

	printf "\033[48;2;%d;%d;%dm", B_R, B_G, B_B;
	printf "\033[38;2;%d;%d;%dm", F_R, F_G, F_B;
	printf "%s\033[0m", s;

}'
