#include "file.h"

file::file()
{

}

int file::writeStream(const char * filepath,const char * buf,int len){
    outFile = fopen(filepath,"ab+");
    if(outFile == NULL){
        std::cout << "error to open: " << filepath << std::endl;
        return -1;
    }
    int l = fwrite(buf,1,len,outFile);
    if(l != len){
        std::cout << "error to write " << std::endl;
    }
    fflush(outFile);
    fclose(outFile);
    return l;
}

int file::write(const char * filepath,const char * buf,int len){
    outFile = fopen(filepath,"a+");
    if(outFile == NULL){
        std::cout << "error to open: " << filepath << std::endl;
        return -1;
    }
    int l = fwrite(buf,1,len,outFile);
    if(l != len){
        std::cout << "error to write " << std::endl;
    }
    fwrite("\n",1,1,outFile);//每写一次换一次行
    fflush(outFile);
    fclose(outFile);
    return l;
}

int file::read(const char * filepath,const char * buf,int len){
    inFile = fopen(filepath,"r+");
    if(inFile == NULL){
        std::cout << "error to open: " << filepath << std::endl;
        return -1;
    }
    int l = fread(const_cast<char *>(buf),1,len,inFile);
    fclose(inFile);
    return l;
}

std::vector<std::string> file::readLine(const char * filepath,int len){
    inFile = fopen(filepath,"r+");
    std::vector<std::string>v;
    if(inFile == NULL){
        std::cout << "error to open: " << filepath << std::endl;
        return v;
    }
    while (!feof(inFile)){
        char buf[1024] = {0};
        if(!fgets(buf,len-1,inFile)){
            break;
        }
        //删除回车
        char lastBuf[1024] = {0};
        memcpy(lastBuf,buf,strlen(buf)-1);
        v.push_back(lastBuf);
    }
    //v.pop_back();//将最后一个空的项给删除
    //    for(auto it = v.begin();it!=v.end();it++){
    //        std::cout << (*it);
    //    }
    //    std::cout << v.size() << std::endl;
    fclose(inFile);
}
