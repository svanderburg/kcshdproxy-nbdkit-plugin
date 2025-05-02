#include "mbrsize.h"

#define NBDKIT_API_VERSION 2
#include <nbdkit-plugin.h>

#include <stdbool.h>
#include <unistd.h>

#include <stdio.h>

#define SECTOR_SIZE 512

#define OFFSET_PARTITION_ENTRY_1 0x1be
#define OFFSET_PARTITION_ENTRY_2 0x1ce
#define OFFSET_PARTITION_ENTRY_3 0x1de
#define OFFSET_PARTITION_ENTRY_4 0x1ee
#define OFFSET_SIGNATURE_1       0x1fe
#define OFFSET_SIGNATURE_2       0x1ff

#define PARTITION_ENTRY_OFFSET_FIRST_SECTOR   0x8
#define PARTITION_ENTRY_OFFSET_NUM_OF_SECTORS 0xc

extern off_t kcsPartitionOffset;

static bool checkMBRSignature(uint8_t *mbr)
{
    return mbr[OFFSET_SIGNATURE_1] == 0xaa && mbr[OFFSET_SIGNATURE_2] == 0x55;
}

static uint32_t parseMingledUInt32(uint8_t *data)
{
    return (data[0] << 8) | data[1] | (data[2] << 24) | (data[3] << 16);
}

static int64_t updateMaxHDSize(int64_t maxSize, uint8_t *partitionEntry)
{
    uint32_t offsetFirstSector = parseMingledUInt32(partitionEntry + PARTITION_ENTRY_OFFSET_FIRST_SECTOR);
    uint32_t numOfSectors = parseMingledUInt32(partitionEntry + PARTITION_ENTRY_OFFSET_NUM_OF_SECTORS);

    int64_t estimatedSize = (offsetFirstSector + numOfSectors) * SECTOR_SIZE;

    if(estimatedSize > maxSize)
        return estimatedSize;
    else
        return maxSize;
}

int64_t determine_size_from_mbr(int fd)
{
    uint8_t readBuffer[SECTOR_SIZE];
    ssize_t actualBufferSize;

    if(lseek(fd, kcsPartitionOffset, SEEK_SET) == -1)
    {
        nbdkit_error("Cannot seek offset: %x\n", kcsPartitionOffset);
        return -1;
    }

    actualBufferSize = read(fd, readBuffer, SECTOR_SIZE);

    if(actualBufferSize != SECTOR_SIZE)
    {
        nbdkit_error("Cannot read MBR block! Read %d bytes, expecting %d bytes\n", actualBufferSize, SECTOR_SIZE);
        return -1;
    }

    if(checkMBRSignature(readBuffer))
    {
        int64_t result = 0;

        result = updateMaxHDSize(result, readBuffer + OFFSET_PARTITION_ENTRY_1);
        result = updateMaxHDSize(result, readBuffer + OFFSET_PARTITION_ENTRY_2);
        result = updateMaxHDSize(result, readBuffer + OFFSET_PARTITION_ENTRY_3);
        result = updateMaxHDSize(result, readBuffer + OFFSET_PARTITION_ENTRY_4);

        return result;
    }
    else
    {
        nbdkit_error("Invalid MBR signature: %x%x", readBuffer[OFFSET_SIGNATURE_1], readBuffer[OFFSET_SIGNATURE_2]);
        return -1;
    }
}
