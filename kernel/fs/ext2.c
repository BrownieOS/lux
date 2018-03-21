
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

/* Ext2 Filesystem Implementation */

#include <vfs.h>
#include <ext2.h>
#include <kprintf.h>
#include <mm.h>
#include <string.h>

int ext2_read_superblock(mountpoint_t *, ext2_superblock_t *);
int ext2_get_inode(mountpoint_t *, const char *, uint32_t *);
int ext2_read_block(mountpoint_t *, ext2_superblock_t *, uint32_t, uint32_t, void *);
int ext2_read_metadata(mountpoint_t *, ext2_superblock_t *, uint32_t, ext2_inode_t *);
int ext2_read_inode(mountpoint_t *, ext2_superblock_t *, ext2_inode_t *, void *);
int ext2_read_singly(mountpoint_t *, ext2_superblock_t *, uint32_t, void *, size_t *);
int ext2_read_doubly(mountpoint_t *, ext2_superblock_t *, uint32_t, void *, size_t *);

// ext2_stat(): Returns stat() information for a file on an ext2 volume
// Param:	mountpoint_t *mountpoint - pointer to mountpoint structure
// Param:	const char *path - path of file/directory
// Param:	struct stat *destination - destination to store stat information
// Return:	int - status code

int ext2_stat(mountpoint_t *mountpoint, const char *path, struct stat *destination)
{
	// get the inode of the file/directory
	uint32_t inode_index;
	int status;
	status = ext2_get_inode(mountpoint, path, &inode_index);
	if(status != 0)
		return status;

	// read the superblock to determine inode size
	// then we can read the inode metadata, which has what we need for stat()
	ext2_superblock_t *superblock = kmalloc(1024);
	status = ext2_read_superblock(mountpoint, superblock);
	if(status != 0)
	{
		kfree(superblock);
		return status;
	}

	uint32_t inode_size = 128;
	if(superblock->version_high >= 1)
		inode_size = (uint32_t)superblock->inode_struct_size;

	// allocate space for the inode metadata
	ext2_inode_t *metadata = kmalloc(inode_size);

	// and read the metadata
	status = ext2_read_metadata(mountpoint, superblock, inode_index, metadata);
	if(status != 0)
	{
		kfree(superblock);
		kfree(metadata);
		return status;
	}

	// put all stat's stuff there!
	destination->st_ino = inode_index;
	destination->st_nlink = metadata->hard_links;
	destination->st_uid = metadata->uid;
	destination->st_gid = metadata->gid;
	destination->st_size = metadata->size_low;
	destination->st_mtime = metadata->mtime;
	destination->st_ctime = metadata->ctime;
	destination->st_atime = metadata->atime;
	destination->st_blksize = 1024 << superblock->block_size;
	destination->st_blocks = (destination->st_size + destination->st_blksize - 1) / destination->st_blksize;

	destination->st_mode = 0;

	// file type
	switch((metadata->type >> 12) & 0x0F)
	{
	case EXT2_FIFO:
		destination->st_mode |= S_IFIFO;
		break;
	case EXT2_CHR:
		destination->st_mode |= S_IFCHR;
		break;
	case EXT2_BLK:
		destination->st_mode |= S_IFBLK;
		break;
	case EXT2_DIR:
		destination->st_mode |= S_IFDIR;
		break;
	case EXT2_REG:
		destination->st_mode |= S_IFREG;
		break;
	case EXT2_LNK:
		destination->st_mode |= S_IFLNK;
		break;
	default:
		kprintf("ext2: %s undefined file mode 0x%xw, assuming regular...\n", path, metadata->type);
		destination->st_mode |= S_IFREG;
		break;
	}

	// file permissions
	if(metadata->type & EXT2_READ_USER)
		destination->st_mode |= S_IRUSR;

	if(metadata->type & EXT2_WRITE_USER)
		destination->st_mode |= S_IWUSR;

	if(metadata->type & EXT2_EXECUTE_USER)
		destination->st_mode |= S_IXUSR;

	if(metadata->type & EXT2_READ_GROUP)
		destination->st_mode |= S_IRGRP;

	if(metadata->type & EXT2_WRITE_GROUP)
		destination->st_mode |= S_IWGRP;

	if(metadata->type & EXT2_EXECUTE_GROUP)
		destination->st_mode |= S_IXGRP;

	if(metadata->type & EXT2_READ_OTHER)
		destination->st_mode |= S_IROTH;

	if(metadata->type & EXT2_WRITE_OTHER)
		destination->st_mode |= S_IWOTH;

	if(metadata->type & EXT2_EXECUTE_OTHER)
		destination->st_mode |= S_IXOTH;

	kfree(superblock);
	kfree(metadata);
	return 0;
}

// ext2_read(): Reads a file from an ext2 volume
// Param:	mountpoint_t *mountpoint - pointer to mountpoint structure
// Param:	file_handle_t *file - file handle structure
// Param:	void *buffer - buffer to read into
// Param:	size_t count - bytes to read
// Return:	ssize_t - total count of bytes read, or error code

ssize_t ext2_read(mountpoint_t *mountpoint, file_handle_t *file, void *buffer, size_t count)
{
	if(file->present != 1 || !file->flags & O_RDONLY)
		return EBADF;

	ext2_superblock_t *superblock = kmalloc(1024);
	int status = ext2_read_superblock(mountpoint, superblock);
	if(status != 0)
		return status;

	// get the file's inode number
	uint32_t inode_index;
	status = ext2_get_inode(mountpoint, file->path, &inode_index);
	if(status != 0)
	{
		kfree(superblock);
		return status;
	}

	// read the metadata
	uint32_t inode_size = 128;
	if(superblock->version_high >= 1)
		inode_size = (uint32_t)superblock->inode_struct_size;

	// allocate space for the inode metadata
	ext2_inode_t *metadata = kmalloc(inode_size);
	status = ext2_read_metadata(mountpoint, superblock, inode_index, metadata);
	if(status != 0)
	{
		kfree(superblock);
		kfree(metadata);
		return status;
	}

	// determine how much is readable
	if(file->position >= metadata->size_low)
	{
		kfree(superblock);
		kfree(metadata);
		return EIO;
	}

	if(file->position + count >= metadata->size_low)
		count = metadata->size_low - file->position;

	// the following lines are dirty af and should be replaced with proper
	// code ASAP.

	// read the file
	void *tmp_buffer = kmalloc(metadata->size_low + (1024 << superblock->block_size));
	status = ext2_read_inode(mountpoint, superblock, metadata, tmp_buffer);
	if(status != 0)
	{
		kfree(superblock);
		kfree(metadata);
		kfree(tmp_buffer);
		return EIO;
	}

	// copy the data needed to be read
	memcpy(buffer, tmp_buffer + file->position, count);
	file->position += count;

	kfree(superblock);
	kfree(metadata);
	kfree(tmp_buffer);
	return count;
}

/* Internal Functions */

// ext2_read_superblock(): Returns the superblock & block group descriptor table
// Param:	mountpoint_t *mountpoint - pointer to mountpoint structure
// Param:	ext2_superblock_t *destination - where to store superblock
// Return:	int - status code

int ext2_read_superblock(mountpoint_t *mountpoint, ext2_superblock_t *destination)
{
	int handle;
	handle = open(mountpoint->device, O_RDONLY);
	if(handle < 0)
	{
		kprintf("ext2: unable to read superblock on volume %s\n", mountpoint->device);
		return handle;
	}

	// superblock starts at byte offset 1024
	if(lseek(handle, 1024, SEEK_SET) != 1024)
	{
		kprintf("ext2: unable to read superblock on volume %s\n", mountpoint->device);
		close(handle);
		return EIO;
	}

	if(read(handle, (char*)destination, 1024) != 1024)
	{
		kprintf("ext2: unable to read superblock on volume %s\n", mountpoint->device);
		close(handle);
		return EIO;
	}

	close(handle);
	return 0;
}

// ext2_get_inode(): Returns the inode number of a file/directory
// Param:	mountpoint_t *mountpoint - pointer to mountpoint structure
// Param:	const char *path - path of file/directory
// Param:	uint32_t *destination - where to store inode number
// Return:	int - status code

int ext2_get_inode(mountpoint_t *mountpoint, const char *path, uint32_t *destination)
{
	path += strlen(mountpoint->path);

	// read the superblock
	ext2_superblock_t *superblock = kmalloc(sizeof(ext2_superblock_t));
	int status = ext2_read_superblock(mountpoint, superblock);
	if(status != 0)
		return status;

	// start at the root directory
	uint32_t inode = EXT2_ROOT_INODE;
	ext2_inode_t *inode_metadata;

	if(superblock->version_high >= 1)
		inode_metadata = kmalloc(superblock->inode_struct_size);
	else
		inode_metadata = kmalloc(128);

	ext2_directory_t *dir = kcalloc(sizeof(ext2_directory_t) + 512, 512);
	status = ext2_read_metadata(mountpoint, superblock, inode, inode_metadata);
	if(status != 0)
	{
		kfree(superblock);
		kfree(dir);
		kfree(inode_metadata);
		return status;
	}

	status = ext2_read_inode(mountpoint, superblock, inode_metadata, dir);
	if(status != 0)
	{
		kfree(superblock);
		kfree(dir);
		kfree(inode_metadata);
		return status;
	}

	char path_ent[64];
	size_t i = 0;
	char last_ent = 0;		// set to 1 when this is the last entry
	ext2_directory_t *dirent;

	dirent = dir;

find_ent:

	i = 0;

	if(path[0] == 0)
	{
		kfree(dir);
		kfree(superblock);
		return 1;
	}

	while(path[0] != '/' && path[0] != 0)
	{
		path_ent[i] = path[0];
		path++;
		i++;
	}

	if(path[0] == 0)
		last_ent = 1;
	else
		path++;

	path_ent[i] = 0;

	//kprintf("ext2: search directory for %s\n", path_ent);

	while(dirent->entry_size != 0)
	{
		if(strcmp(dirent->file_name, path_ent) == 0)
			goto found_ent;

		dirent = (ext2_directory_t*)((size_t)dirent + dirent->entry_size);
	}

	//kprintf("ext2: entry %s not found\n", path_ent);
	kfree(dir);
	kfree(superblock);
	kfree(inode_metadata);
	return 1;

found_ent:
	if(last_ent == 1)
	{
		destination[0] = dirent->inode;
		kfree(dir);
		kfree(superblock);
		kfree(inode_metadata);
		return 0;
	} else
	{
		// read the next inode metadata
		inode = dirent->inode;
		status = ext2_read_metadata(mountpoint, superblock, inode, inode_metadata);
		if(status != 0)
		{
			kfree(superblock);
			kfree(dir);
			kfree(inode_metadata);
			return status;
		}

		status = ext2_read_inode(mountpoint, superblock, inode_metadata, dir);
		if(status != 0)
		{
			kfree(superblock);
			kfree(dir);
			kfree(inode_metadata);
			return status;
		}

		// make sure it is a directory
		if((inode_metadata->type >> 12) != EXT2_DIR)
		{
			kfree(superblock);
			kfree(dir);
			kfree(inode_metadata);
			return ENOTDIR;
		}

		dirent = dir;
		goto find_ent;
	}
}

// ext2_read_block(): Reads a block
// Param:	mountpoint_t *mountpoint - mountpoint
// Param:	ext2_superblock_t *superblock - superblock
// Param:	uint32_t block - block address
// Param:	uint32_t count - block count
// Param:	void *destination - destination to read
// Return:	int - return status

int ext2_read_block(mountpoint_t *mountpoint, ext2_superblock_t *superblock, uint32_t block, uint32_t count, void *destination)
{
	uint32_t block_size = 1024 << superblock->block_size;
	off_t byte_offset = block * block_size;
	size_t byte_count = count * block_size;

	int handle;
	handle = open(mountpoint->device, O_RDONLY);
	if(handle < 0)
	{
		kprintf("ext2: failed to read block %d on device %s\n", block, mountpoint->device);
		return EIO;
	}

	if(lseek(handle, byte_offset, SEEK_SET) != byte_offset)
	{
		kprintf("ext2: failed to read block %d on device %s\n", block, mountpoint->device);
		close(handle);
		return EIO;
	}

	if(read(handle, (char*)destination, byte_count) != byte_count)
	{
		kprintf("ext2: failed to read block %d on device %s\n", block, mountpoint->device);
		close(handle);
		return EIO;
	}

	close(handle);
	return 0;
}

// ext2_read_metadata(): Reads an inode's metadata
// Param:	mountpoint_t *mountpoint - mountpoint
// Param:	ext2_superblock_t *superblock - superblock
// Param:	uint32_t inode - inode number
// Param:	ext2_inode_t *destination - destination buffer
// Return:	int - status

int ext2_read_metadata(mountpoint_t *mountpoint, ext2_superblock_t *superblock, uint32_t inode, ext2_inode_t *destination)
{
	inode--;		// because inode numbering starts at one
	uint32_t block_group = inode / superblock->inodes_per_group;

	// read the block group descriptor table
	uint32_t block_table_address, block_size;
	block_size = 1024 << superblock->block_size;
	if(block_size == 1024)
		block_table_address = 2;
	else
		block_table_address = 1;

	// determine how many block groups there are
	uint32_t block_group_count;

	block_group_count = superblock->total_blocks / superblock->blocks_per_group;
	block_group_count++;

	ext2_block_group_t *block_groups = kcalloc(sizeof(ext2_block_group_t), block_group_count);

	int status;
	status = ext2_read_block(mountpoint, superblock, block_table_address, ((block_group_count * sizeof(ext2_block_group_t)) + block_size - 1) / block_size, block_groups);
	if(status != 0)
	{
		kprintf("ext2: unable to read inode %d\n", inode+1);
		kfree(block_groups);
		return status;
	}

	// get inode index block address
	uint32_t inode_index = block_groups[block_group].inode_table;
	kfree(block_groups);

	uint32_t inode_size = 128;		// ext2 < v1.0
	if(superblock->version_high >= 1)
		inode_size = (uint32_t)superblock->inode_struct_size;

	ext2_inode_t *inodes = kcalloc(inode_size, superblock->inodes_per_group);
	uint32_t index = inode % superblock->inodes_per_group;

	// read the inode table
	status = ext2_read_block(mountpoint, superblock, inode_index, ((inode_size * superblock->inodes_per_group) + block_size - 1) / block_size, inodes);
	if(status != 0)
	{
		kfree(inodes);
		return status;
	}

	// copy the requested inode
	memcpy(destination, &inodes[index], inode_size);
	kfree(inodes);
	return 0;
}

// ext2_read_inode(): Reads contents of an inode
// Param:	mountpoint_t *mountpoint - mountpoint
// Param:	ext2_superblock_t *superblock - superblock
// Param:	ext2_inode_t *inode - inode to read
// Param:	void *destination - destination to read
// Return:	int - return status

int ext2_read_inode(mountpoint_t *mountpoint, ext2_superblock_t *superblock, ext2_inode_t *inode, void *destination)
{
	// we have the actual inode metadata, read the direct blocks first
	size_t direct_count = 0;
	int status;
	uint32_t block_size = 1024 << superblock->block_size;

	while(direct_count < 12 && inode->direct_blocks[direct_count] != 0)
	{
		status = ext2_read_block(mountpoint, superblock, inode->direct_blocks[direct_count], 1, destination);
		if(status != 0)
			return status;

		destination += block_size;
		direct_count++;
	}

	size_t indirect_size;

	if(inode->singly_block != 0)
	{
		status = ext2_read_singly(mountpoint, superblock, inode->singly_block, destination, &indirect_size);
		if(status != 0)
			return status;

		destination += indirect_size;
	}

	if(inode->doubly_block != 0)
	{
		status = ext2_read_doubly(mountpoint, superblock, inode->doubly_block, destination, &indirect_size);
		if(status != 0)
			return status;

		destination += indirect_size;
	}

	if(inode->triply_block != 0)
	{
		kprintf("ext2: TO-DO: Implement triply blocks here.\n");
		while(1);
	}

	return 0;
}

// ext2_read_singly(): Reads a singly indirect block
// Param:	mountpoint_t *mountpoint - mountpoint structure
// Param:	ext2_superblock_t *superblock - superblock
// Param:	uint32_t block - block number
// Param:	void *destination - destination to read into
// Param:	size_t *size - destination to store byte count
// Return:	int - status code

int ext2_read_singly(mountpoint_t *mountpoint, ext2_superblock_t *superblock, uint32_t block, void *destination, size_t *size)
{
	size[0] = 0;

	size_t count;
	uint32_t block_size = 1024 << superblock->block_size;

	count = block_size / sizeof(uint32_t);		// number of block pointers

	// read the singly block
	uint32_t *singly_block = kmalloc(block_size);
	int status;
	status = ext2_read_block(mountpoint, superblock, block, 1, singly_block);
	if(status != 0)
		return status;

	// and read the blocks pointed to by them
	size_t i = 0;
	while(i < count && singly_block[i] != 0)
	{
		status = ext2_read_block(mountpoint, superblock, singly_block[i], 1, destination);
		if(status != 0)
			return status;

		destination += block_size;
		size[0] += block_size;
		i++;
	}

	kfree(singly_block);
	return 0;
}

// ext2_read_doubly(): Reads a doubly indirect block
// Param:	mountpoint_t *mountpoint - mountpoint structure
// Param:	ext2_superblock_t *superblock - superblock
// Param:	uint32_t block - block number
// Param:	void *destination - destination to read into
// Param:	size_t *size - destination to store byte count
// Return:	int - status code

int ext2_read_doubly(mountpoint_t *mountpoint, ext2_superblock_t *superblock, uint32_t block, void *destination, size_t *size)
{
	size[0] = 0;

	size_t count;
	uint32_t block_size = 1024 << superblock->block_size;

	count = block_size / sizeof(uint32_t);		// number of block pointers

	// read the doubly block
	uint32_t *doubly_block = kmalloc(block_size);
	int status;
	status = ext2_read_block(mountpoint, superblock, block, 1, doubly_block);
	if(status != 0)
		return status;

	// and read the blocks pointed to by them
	size_t i = 0;
	size_t entry_size;
	while(i < count && doubly_block[i] != 0)
	{
		status = ext2_read_singly(mountpoint, superblock, doubly_block[i], destination, &entry_size);
		if(status != 0)
			return status;

		destination += entry_size;
		size[0] += entry_size;
		i++;
	}

	kfree(doubly_block);
	return 0;
}





