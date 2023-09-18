#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
class file
{
public:
    file();
    int write(const char * filepath,const char * buf,int len);
    int writeStream(const char * filepath,const char * buf,int len);
    int read(const char * filepath,const char * buf,int len);
    std::vector<std::string> readLine(const char * filepath,int len);
    FILE *outFile,*inFile;
};

#endif // FILE_H
