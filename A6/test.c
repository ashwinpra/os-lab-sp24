#include<stdio.h>
#include<stdlib.h>
#include<event.h>

void get_time(char* timestr, int current_time){
    // taking base time as 9:00 am, return current time s
    int hours, minutes;
    if(current_time < 0){
        hours = 9 - (-current_time) / 60;
        minutes = 60 - (-current_time) % 60;
    } else {
        hours = 9 + current_time / 60;
        minutes = current_time % 60;
    }

    if(hours > 12){
        hours -= 12;
        // pad mintues and hours with 0 if less than 10 and print
        sprintf(timestr, "[%02d:%02d pm]", hours, minutes);
    } else {
        sprintf(timestr, "[%02d:%02d am]", hours, minutes);
    }
}


int main(){
    eventQ E = initEQ("arrival.txt");
    while(!emptyQ(E)){
        event e = nextevent(E);
        if(e.type == 'P'){
            char timestr[20];
            get_time(timestr, e.time);
            printf("%s Patient arrives\n", timestr);
        } else if(e.type == 'R'){
            char timestr[20];
            get_time(timestr, e.time);
            printf("%s Report arrives\n", timestr);
        } else if(e.type == 'S'){
            char timestr[20];
            get_time(timestr, e.time);
            printf("%s Sales Rep arrives\n", timestr);
        }
        E = delevent(E);
    }
    return 0;
}