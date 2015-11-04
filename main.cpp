#include "BlockingBuffer.h"
#include <pthread.h>

#define PARALLELNUM 8
//#define CHUNKSIZE   7*1034*125
#define CHUNKSIZE   64*1024*1024
//#define CHUNKSIZE   1233434

void* DownloadThreadfunc(void* data) {

    BlockingBuffer* buffer = (BlockingBuffer*)data;
    size_t filled_size = 0;
    void* pp = (void*)pthread_self();
    // assert offset > 0
    do {
        filled_size = buffer->Fill();
        std::cout<<"Fillsize is "<<filled_size<<" of "<<pp<<std::endl;
        if(buffer->EndOfFile())
            break;
        if (filled_size == -1) { // Error
            // retry?
            if(buffer->Error()) {
                break;
            } else
                continue;
        }
    } while(1);
    std::cout<<"quit\n";
    return NULL;
}

#ifdef ASMAIN

int main(int argc, char const *argv[])
{
    // filepath and file length
    if(argc < 3) {
        printf("not enough parameters\n");
        return 1;
    }
    SIZE_T filelen = atoll(argv[2]);

    /* code */
    //InitRB();
    //InitOffset(601882624, CHUNKSIZE);
    pthread_t threads[PARALLELNUM];
    BlockingBuffer* buffers[PARALLELNUM];
    OffsetMgr* o = new OffsetMgr(filelen,CHUNKSIZE);
    for(int i = 0; i < PARALLELNUM; i++) {
        // Create
        buffers[i] = BlockingBuffer::CreateBuffer(argv[1], o);
        if(!buffers[i]->Init())
            std::cerr<<"init fail"<<std::endl;
        pthread_create(&threads[i], NULL, DownloadThreadfunc, buffers[i]);
    }

    int i = 0;
    BlockingBuffer *buf = NULL;
    char* data = (char*) malloc(4096);
    size_t len;
    size_t totallen = 0;

    int fd = open("data.bin", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd == -1) {
        perror("create file error");
        return 1;
    }
    while(true) {
        buf = buffers[i%PARALLELNUM];
        len = buf->Read(data, 4096);

        write(fd, data, len);
        totallen += len;
        if(len < 4096) {
            i++;
            if(buf->Error())
                break;
        }
        if(totallen ==  filelen) {
            if(buf->EndOfFile())
                break;
        }
    }
    std::cout<<"exiting"<<std::endl;
    free(data);
    for(i = 0; i < PARALLELNUM; i++) {
        pthread_join(threads[i],NULL);
    }
    for(i = 0; i < PARALLELNUM; i++) {
        delete buffers[i];
    }
    close(fd);
    delete o;
    return 0;
}

#endif // ASMAIN
