#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OFFSET_SIGNATURE_1 0x1fe
#define OFFSET_SIGNATURE_2 0x1ff

#define BUFFER_SIZE 512

#define TRUE 1

typedef int BOOL;
typedef unsigned char UBYTE;

static BOOL checkMBRSignature(UBYTE *buffer)
{
    return buffer[OFFSET_SIGNATURE_1] == 0xaa && buffer[OFFSET_SIGNATURE_2] == 0x55;
}

int main(int argc, char *argv[])
{
    int exit_status;
    char *targetFile;
    off_t offset;

    if(argc >= 2)
        targetFile = argv[1];
    else
        targetFile = NULL;

    if(argc >= 3)
        offset = atoi(argv[2]);
    else
        offset = 0;

    if(targetFile == NULL)
    {
        fprintf(stderr, "No targetFile was specified!\n");
        exit_status = 1;
    }
    else
    {
        int fd = open(targetFile, O_RDONLY);

        if(fd == -1)
        {
            fprintf(stderr, "Cannot open target file: %s\n", strerror(errno));
            exit_status = 1;
        }
        else
        {
            if(lseek(fd, offset, SEEK_SET) == -1)
            {
                fprintf(stderr, "Cannot seek offset: %s\n", strerror(errno));
                exit_status = 1;
            }
            else
            {
                while(TRUE)
                {
                    UBYTE readBuffer[BUFFER_SIZE];
                    ssize_t actualBufferSize = read(fd, readBuffer, BUFFER_SIZE);

                    if(actualBufferSize == -1)
                    {
                        fprintf(stderr, "Read error: %s\n", strerror(errno));
                        exit_status = 1;
                        break;
                    }
                    else if(actualBufferSize < BUFFER_SIZE)
                    {
                        fprintf(stderr, "We could not a read a full block of 512 bytes. Instead, we have read: %ld bytes\n", actualBufferSize);
                        exit_status = 1;
                    }
                    else if(checkMBRSignature(readBuffer))
                    {
                        printf("MBR found at offset: %lu\n", offset);
                        exit_status = 0;
                        break;
                    }

                    offset += BUFFER_SIZE;
                }
            }

            close(fd);
        }
    }

    return exit_status;
}
