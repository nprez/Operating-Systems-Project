#include <stdio.h>
#include<stdlib.h>
#include<fuse.h>

struct dirent{
  int abc;
};

struct stat{
  int abc;
};

int create(char* path){
  return 0;
}

int delete(char* path){
  return 0;
}

int stat(char* path, struct stat* buf){
  return 0;
}

int open(char* path){
  return 0;
}

int close(int fileID){
  return 0;
}

int read(int fileID, void* buffer, int bytes){
  return 0;
}

int write(int fileID, void* buffer, int bytes){
  return 0;
}

struct dirent* readdir(int directoryID){
  return NULL;
}
