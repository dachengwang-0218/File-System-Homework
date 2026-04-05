#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
#include<unistd.h>
#include<string.h>

int main(int argc, char *argv[]){
    char *target_path = (argc > 1) ? argv[1] : ".";
    DIR *dp = opendir(target_path);
    struct dirent *entry;
    struct stat buf;
    int count = 1;

    if(dp == NULL){
        perror("無法打開目錄");
        return 1;
    }

    while((entry = readdir(dp)) != NULL){
        if(entry->d_name[0] == '.') continue;
        
        printf("%-20s ", entry->d_name);
        if(count++ != 3) printf("\t\t\t");
        else{
            printf("\n");
            count = 1;
        }

    }
    printf("\n");

    closedir(dp);

    return 0;
}



















