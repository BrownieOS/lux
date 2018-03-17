
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#pragma once

#include <types.h>
#include <vfs.h>

#define EXT2_ROOT_INODE			2	// the root dir is always inode 2

// File Permissions are stored in the Inode Metadata
#define EXT2_READ_USER			0x100
#define EXT2_WRITE_USER			0x080
#define EXT2_EXECUTE_USER		0x040
#define EXT2_READ_GROUP			0x020
#define EXT2_WRITE_GROUP		0x010
#define EXT2_EXECUTE_GROUP		0x008
#define EXT2_READ_OTHER			0x004
#define EXT2_WRITE_OTHER		0x002
#define EXT2_EXECUTE_OTHER		0x001

// File Types
#define EXT2_FIFO			0x1
#define EXT2_CHR			0x2
#define EXT2_DIR			0x4
#define EXT2_BLK			0x6
#define EXT2_REG			0x8
#define EXT2_LNK			0xA
#define EXT2_SOCK			0xC

typedef struct ext2_superblock_t
{
	/* these are always here */
	uint32_t total_inodes;
	uint32_t total_blocks;
	uint32_t superuser_blocks;
	uint32_t free_blocks;
	uint32_t free_inodes;
	uint32_t superblock_number;
	uint32_t block_size;
	uint32_t fragment_size;
	uint32_t blocks_per_group;
	uint32_t fragments_per_group;
	uint32_t inodes_per_group;
	uint32_t mount_time;
	uint32_t written_time;
	uint16_t mount_count;
	uint16_t mount_count_remaining;
	uint16_t ext2_magic;
	uint16_t fs_state;
	uint16_t fs_error;
	uint16_t version_low;
	uint32_t last_check_time;
	uint32_t check_interval;
	uint32_t os_id;
	uint32_t version_high;
	uint16_t superuser_uid;
	uint16_t superuser_gid;

	/* these are for ext2 version 1.0 and on */
	uint32_t first_inode;
	uint16_t inode_struct_size;
	uint16_t superblock_backup;
	uint32_t optional_features;
	uint32_t required_features;
	uint32_t unsupported_features;

	uint8_t fs_id[16];
	uint8_t volume_name[16];
	uint8_t last_mountpoint[64];

	uint32_t compression;
	uint8_t preallocate_file;
	uint8_t preallocate_directory;
	uint16_t reserved;

	uint8_t journal_id[16];
	uint32_t journal_inode;
	uint32_t journal_device;
	uint32_t orphan_inode;

	uint8_t reserved2[788];
}__attribute__((packed)) ext2_superblock_t;

typedef struct ext2_block_group_t
{
	uint32_t block_usage_bitmap;
	uint32_t inode_usage_bitmap;
	uint32_t inode_table;
	uint16_t free_blocks;
	uint16_t free_inodes;
	uint16_t directory_count;
	uint8_t reserved[14];
}__attribute__((packed)) ext2_block_group_t;

typedef struct ext2_inode_t
{
	uint16_t type;
	uint16_t uid;
	uint32_t size_low;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t dtime;
	uint16_t gid;
	uint16_t hard_links;
	uint32_t sector_size;		/* size in sectors, not blocks */
	uint32_t flags;
	uint32_t os_specific1;
	uint32_t direct_blocks[12];
	uint32_t singly_block;
	uint32_t doubly_block;
	uint32_t triply_block;
	uint32_t generation_number;
	uint32_t extended_block;	/* >= v1.0 only */
	uint32_t size_high;		/* >= v1.0 only */
	uint32_t fragment;
	uint8_t os_specific2[12];
}__attribute__((packed)) ext2_inode_t;

typedef struct ext2_directory_t
{
	uint32_t inode;
	uint16_t entry_size;
	uint8_t name_length;
	uint8_t reserved;
	char file_name[];
}__attribute__((packed)) ext2_directory_t;

int ext2_stat(mountpoint_t *, const char *, struct stat *);




