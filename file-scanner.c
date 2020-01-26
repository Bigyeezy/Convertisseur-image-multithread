/*
 * Programme qui scan un dossier et liste les fichier présents dans ce dossier
 */

#include <dirent.h> 
#include <stdio.h> 
#include <string.h>

int main(void) {
  DIR *d;
  struct dirent *dir;
  d = opendir("./input");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
        char* str = ".bmp";
        char * pos = strstr(dir->d_name, str);
        if (pos) {
            printf("%s\n", dir->d_name);
        }
    }
    closedir(d);
  }
  return(0);
}