
#ifndef __MEXT2_H
#define __MEXT2_H

#define DATA_BLOCK 263680
#define BLOCK_SIZE 512
#define DISK_START 0
#define BLOCK_BITMAP 512
#define INODE_BITMAP 1024
#define INODE_TABLE 1536
#define INODE_SIZE 64

struct group_desc {
	char bg_volume_name[16];		/*卷名*/
	unsigned short bg_block_bitmap; /*保存块位图的块号*/
	unsigned short bg_inode_bitmap; /*保存索引结点位图的块号*/
	unsigned short bg_inode_table;	/*索引结点表的起始块号*/
	unsigned short bg_free_blocks_count; /*本组空闲块的个数*/
	unsigned short bg_free_inodes_count; /*本组空闲索引结点的个数*/
	unsigned short bg_used_dirs_count;	 /*本组目录的个数*/
	unsigned short bg_user_block;		 /*存储用户信息的数据块号*/
	unsigned short bg_user_count;		 /*用户数目*/
};
struct inode {
	union _i_mode
	{
		unsigned short i_mode;
		struct mode_bit_
		{
			unsigned short rwx_oth : 3;
			unsigned short rwx_gup : 3;
			unsigned short rwx_own : 3;
		}mode_bit;
	}i_mode;/*文件类型及访问权限*/
	unsigned short i_uid;	 /* 问价所属 */
	unsigned short i_blocks; /*文件的数据块个数*/
	unsigned long i_size;	 /*大小( 字节)*/
	unsigned long i_atime;	 /*访问时间*/
	unsigned long i_ctime;	 /*创建时间*/
	unsigned long i_mtime;	 /*修改时间*/
	unsigned long i_dtime;	 /*删除时间*/
	unsigned short i_block[8]; /*指向数据块的指针*/
	char i_pad[22];			   /*填充(0xff)*/
};
//16 byes
struct dir_entry {   //目录项结构
	unsigned short inode; /*索引节点号*/
	unsigned short rec_len; /*目录项长度*/
	unsigned short name_len; /*文件名长度  路径长度*/
	char file_type; /*文件类型(1: 普通文件， 2: 目录.. )*/
	char name[9]; /*文件名*/
};

void cd(char tmp[9]);
void del(char tmp[9]);
void makedir(char tmp[9]);
void mkf(char tmp[9]);
void rmdir(char tmp[9]);
void ls();
void read_file(char tmp[9]);
void write_file(char tmp[9]);
void close_file(char tmp[9]);
void open_file(char tmp[9]);
void format(char tmp[9]);
void help(char tmp[9]);
void quit(char name[9]);
void update_user(unsigned short i);
void reload_user(unsigned short i);
void update_group_desc();
void initialize_disk();
void initialize_memory();

#endif // !__MEXT2_H


