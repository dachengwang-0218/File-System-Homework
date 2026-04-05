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

void print_permissions(mode_t mode);

int main(int argc, char *argv[]){
    char *target_path = ".";
    DIR *dp = opendir(target_path);
    struct dirent *entry;
    struct stat buf;
    int is_l = 0;
    int count = 1;

    for(int i = 1 ; i < argc ; i++){
        if(strcmp(argv[i], "-l") == 0){
            is_l = 1;
        }else{
            target_path = argv[i];
        }
    }

    if(dp == NULL){
        perror("無法打開目錄");
        return 1;
    }

    while((entry = readdir(dp)) != NULL){
        if(entry->d_name[0] == '.') continue;

        if(is_l){
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", target_path, entry->d_name);
            if(lstat(full_path, &buf) == -1){
                perror("無法取得該檔案的metadata");
                continue;
            }

            print_permissions(buf.st_mode);
    
            struct passwd *user = getpwuid(buf.st_uid);
            char *user_name = (user != NULL) ? user->pw_name : "unknown";

            struct group *grp = getgrgid(buf.st_gid);
            char *group_name = (grp != NULL) ? grp->gr_name : "unknown";

            struct tm *ptr_time = localtime(&buf.st_mtime);
            char time_buf[80];
            strftime(time_buf, sizeof(time_buf), "%m/%d/%y %H:%M", ptr_time);

            printf("%-8s %-8s %8ld %s %s", user_name, group_name, buf.st_size, time_buf, entry->d_name);

            if(S_ISLNK(buf.st_mode)){
                char link_buf[256];
                ssize_t len = readlink(full_path, link_buf, sizeof(link_buf) - 1);
                if(len != -1){
                    link_buf[len] = '\0';
                    printf(" -> %s", link_buf);
                }
            }
        
            printf("\n"); 
        }else{
            printf("%-20s", entry->d_name);
            if(count++ == 3){
                printf("\n");
                count = 1;
            }
        }

    }

    closedir(dp);

    return 0;
}

void print_permissions(mode_t mode){
    char permission[11] = "----------";

    if(S_ISDIR(mode)) permission[0] = 'd';
    else if(S_ISLNK(mode)) permission[0] = 'l';
    else if(S_ISCHR(mode)) permission[0] = 'c';
    else if(S_ISBLK(mode)) permission[0] = 'b'; 
    else if(S_ISFIFO(mode)) permission[0] = 'p';
    else if(S_ISSOCK(mode)) permission[0] = 's';

    if(mode & S_IRUSR) permission[1] = 'r';
    if(mode & S_IWUSR) permission[2] = 'w';
    if(mode & S_IXUSR) permission[3] = 'x';

    if(mode & S_IRGRP) permission[4] = 'r';
    if(mode & S_IWGRP) permission[5] = 'w';
    if(mode & S_IXGRP) permission[6] = 'x';

    if(mode & S_IROTH) permission[7] = 'r';
    if(mode & S_IWOTH) permission[8] = 'w';
    if(mode & S_IXOTH) permission[9] = 'x';

    printf("%s ", permission);
}



















