#define NBDKIT_API_VERSION 2
#include <nbdkit-plugin.h>

#define THREAD_MODEL NBDKIT_THREAD_MODEL_SERIALIZE_ALL_REQUESTS

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "version.h"
#include "mbrsize.h"

char *targetFile = NULL;
off_t kcsPartitionOffset = 0;

typedef struct
{
    int fd;
}
Handle;

static int kcshdproxy_config(const char *key, const char *value)
{
    if(strcmp(key, "offset") == 0)
        kcsPartitionOffset = atoi(value);
    else if(strcmp(key, "targetFile") == 0)
        targetFile = (char*)value;

    return 0;
}

static int kcshdproxy_config_complete(void)
{
    if(targetFile == NULL)
    {
        nbdkit_error("No targetFile was specified!\n");
        return -1;
    }
    else
        return 0;
}

static void *kcshdproxy_open(int readonly)
{
    int flags;
    int fd;

    if(readonly)
        flags = O_RDONLY;
    else
        flags = O_RDWR;

    fd = open(targetFile, flags);

    if(fd == -1)
    {
        nbdkit_error("Cannot open target file: %s\n", targetFile);
        return NULL;
    }
    else
    {
        Handle *handle = (Handle*)malloc(sizeof(Handle));
        handle->fd = fd;
        return handle;
    }
}

static void kcshdproxy_close(void *handle)
{
    Handle *handleObj = (Handle*)handle;
    close(handleObj->fd);
    free(handleObj);
}

static int64_t kcshdproxy_get_size(void *handle)
{
    Handle *handleObj = (Handle*)handle;
    return determine_size_from_mbr(handleObj->fd);
}

static void reverseWordOrder(uint8_t *data, uint32_t dataLength)
{
    uint32_t i;

    for(i = 1; i < dataLength; i += 2)
    {
        uint8_t remember = data[i - 1];
        data[i - 1] = data[i];
        data[i] = remember;
    }
}

static int kcshdproxy_pread(void *handle, void *buf, uint32_t count, uint64_t offset, uint32_t flags)
{
    Handle *handleObj = (Handle*)handle;

    if(lseek(handleObj->fd, offset + kcsPartitionOffset, SEEK_SET) == -1)
    {
        nbdkit_error("Cannot reposition file: %s\n", strerror(errno));
        return -1;
    }

    if(read(handleObj->fd, buf, count) != count)
    {
        nbdkit_error("Cannot read the right amount of bytes: %s\n", strerror(errno));
        return -1;
    }

    reverseWordOrder((uint8_t*)buf, count);

    return 0;
}

static int kcshdproxy_pwrite(void *handle, const void *buf, uint32_t count, uint64_t offset, uint32_t flags)
{
    Handle *handleObj = (Handle*)handle;

    if(lseek(handleObj->fd, offset + kcsPartitionOffset, SEEK_SET) == -1)
    {
        nbdkit_error("Cannot reposition file: %s\n", strerror(errno));
        return -1;
    }

    reverseWordOrder((uint8_t*)buf, count);

    if(write(handleObj->fd, buf, count) != count)
    {
        nbdkit_error("Cannot write the right amount of bytes: %s\n", strerror(errno));
        return -1;
    }

    reverseWordOrder((uint8_t*)buf, count);

    return 0;
}

static struct nbdkit_plugin plugin = {
    .name = "kcshdproxy-plugin",
    .version = VERSION,
    .longname = "KCS HD Proxy plugin",
    .description = "NBD plugin for accessing KCS PowerPC board emulated hard drives",
    .config = kcshdproxy_config,
    .config_complete = kcshdproxy_config_complete,
    .open = kcshdproxy_open,
    .get_size = kcshdproxy_get_size,
    .pread = kcshdproxy_pread,
    .pwrite = kcshdproxy_pwrite,
    .close = kcshdproxy_close
};

NBDKIT_REGISTER_PLUGIN(plugin);
