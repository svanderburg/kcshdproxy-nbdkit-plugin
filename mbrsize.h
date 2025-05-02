#ifndef __MBRSIZE_H
#define __MBRSIZE_H

#include <stdint.h>

/**
 * This function reads the MBR and uses the partition's start offset and sizes
 * to determine what the size of the hard drive is.
 *
 * @param int fd File descriptor to read from
 * @return The size of the harddisk in bytes or -1 in case of a failure
 */
int64_t determine_size_from_mbr(int fd);

#endif
