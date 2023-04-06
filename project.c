#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

char buffer;
char buffer2;
char bufferout;
int count = 0;
int fin, fout;

void statistica(char buffer, int flag){
for(int i= 0;i < flag; i++){
    if(isalnum(&buffer)){
        count++;
    }
}
}

void scriere(int fout){
struct stat var;
int test = 0;
fstat(fout, &var); //var.st_uid
sprintf(&bufferout, "malfanum: %d, idowner: %u", count, var.st_uid);
if((test=write(fout,&bufferout, strlen(&bufferout)))<0){
perror("Couldn't write to file");
}
}

void citire(int fin){
ssize_t flag= 0;
while((flag=read(fin, &buffer, sizeof(&buffer)))!=0){
sprintf(&buffer2, "%p", &buffer);
statistica(buffer, flag);
}
}

int main(int argc, char* argv[]){
if(argc != 3){
    perror("Wrong number of arguments. The accepted number of arguments is 3.");
}
if ((fin=open(argv[1], O_RDONLY)<0)){
    perror("Couldn't read input file.");
}
if ((fout=open(argv[2], O_CREAT|O_TRUNC|O_WRONLY, S_IWUSR|S_IWGRP|S_IRUSR))<0){
    perror("Couldn't create output file.");
}
citire(fin);
scriere(fout);

close(fin);
close(fout);
    return 0;
}