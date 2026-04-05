#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<sys/stat.h>

int found = 0;

void search(const char *current_path, const char *target_file);

int main(int argc, char *argv[]){
    char *target_file = NULL;
    char *ori_dir = ".";

    if(argc == 2){
        target_file = argv[1];
    }else if(argc == 3){
        ori_dir = argv[1];
        target_file = argv[2];
    }else{
        perror("錯誤格式");
        return 1;
    }

    printf("from path: %s\n", ori_dir);
    printf("search file: %s\n", target_file);

    search(ori_dir, target_file);

    if(found == 0){
        printf("cannot find the corresponding file\n");
    }

    return 0;
}

void search(const char *current_path, const char *target_file){
    DIR *dp = opendir(current_path);
    struct dirent *entry;
    char full_path[1024];

    if(dp == NULL){
        perror("無法打開目錄");
    }

    while((entry = readdir(dp)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        if(strcmp(current_path, "/") == 0){
            snprintf(full_path, sizeof(full_path), "/%s", entry->d_name);
        }else{
            snprintf(full_path, sizeof(full_path), "%s/%s", current_path, entry->d_name);
        }

        if(strcmp(entry->d_name, target_file) == 0){
            printf("%s\n", full_path);
            found = 1;
        }

        if(entry->d_type == DT_DIR){
            search(full_path, target_file);
        }
    }

    closedir(dp);
}






















