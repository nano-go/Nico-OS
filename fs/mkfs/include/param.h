#define NDIRECT_DATA_BLOCKS 11
#define NFILES_PER_PART (4096 * 4) // Number of files per partition.

#define MAX_OPEN_BLOCKS 10
#define LOG_SIZE (MAX_OPEN_BLOCKS * 3)

#define BLOCK_SIZE 512
#define SECTOR_SIZE 512
#define LBA_TO_BLOCK_NO(lba) ((lba) / (BLOCK_SIZE / SECTOR_SIZE))

// Number of block addresses per block.
#define NINDIRECT_DATA_BLOCKS (BLOCK_SIZE / sizeof(uint32_t))

// Maximum number of data blocks of a inode.
#define MAX_DATA_BLOCKS (NDIRECT_DATA_BLOCKS + NINDIRECT_DATA_BLOCKS)

// Number of inodes per block
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct inode))

// Gets the block number containing inode i.
#define GET_INODE_BLOCK_NO(i, sb) ((sb).inode_start + ((i) / INODES_PER_BLOCK))

#define BITS_PER_BLOCK (BLOCK_SIZE * 8)
#define ROOT_INUM 1