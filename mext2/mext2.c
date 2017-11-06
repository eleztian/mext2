
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "mext2.h"
#include "user.h"

extern user_info user[10];
extern user_info use_user;
char User_tips[10];
char Buffer[512];  //针对数据块的 缓冲区
char tempbuf[4097]; //
unsigned char bitbuf[512]; //位图缓冲区
unsigned short index_buf[256];
short fopen_table[16]; //  文件打开表
unsigned short last_alloc_inode; //  最近分配的节点号
unsigned short last_alloc_block; //  最近分配的数据块号
unsigned short current_dir;   //      当前目录的节点号
struct group_desc super_block[1]; //   组描述符缓冲区
struct inode inode_area[1];  //   节点缓冲区
struct dir_entry dir[32];   //  目录项缓冲区
char current_path[256];    //    当前路径名
unsigned short current_dirlen;
FILE *fp;

time_t now_time()
{
	time_t  now;
	ctime(&now);
	return now;
}

/*************               内存分配操作                 ***************/
void update_group_desc()
{
	fseek(fp, DISK_START, SEEK_SET);
	fwrite(super_block, BLOCK_SIZE, 1, fp);
}
void reload_group_desc()//载入组描述符
{
	fseek(fp, DISK_START, SEEK_SET);
	fread(super_block, BLOCK_SIZE, 1, fp);
}
void update_inode_bitmap()//更新inode位图 
{
	fseek(fp, INODE_BITMAP, SEEK_SET);
	fwrite(bitbuf, BLOCK_SIZE, 1, fp);
}
void reload_inode_bitmap()//载入inode位图 
{
	fseek(fp, INODE_BITMAP, SEEK_SET);
	fread(bitbuf, BLOCK_SIZE, 1, fp);
}
void update_block_bitmap()//更新block位图 
{
	fseek(fp, BLOCK_BITMAP, SEEK_SET);
	fwrite(bitbuf, BLOCK_SIZE, 1, fp);
}
void reload_block_bitmap()//载入block位图 
{
	fseek(fp, BLOCK_BITMAP, SEEK_SET);
	fread(bitbuf, BLOCK_SIZE, 1, fp);
}
void update_inode_entry(unsigned short i)//更新第i个inode入口 
{
	fseek(fp, INODE_TABLE + (i - 1)*INODE_SIZE, SEEK_SET);
	fwrite(inode_area, INODE_SIZE, 1, fp);
}
void reload_inode_entry(unsigned short i)//载入第i个inode入口
{
	fseek(fp, INODE_TABLE + (i - 1)*INODE_SIZE, SEEK_SET);
	fread(inode_area, INODE_SIZE, 1, fp);
}

void reload_dir(unsigned short i)//更新第i个目录 
{
	fseek(fp, DATA_BLOCK + i*BLOCK_SIZE, SEEK_SET);
	fread(dir, BLOCK_SIZE, 1, fp);
}
void update_dir(unsigned short i)//载入第i个目录 
{
	fseek(fp, DATA_BLOCK + i*BLOCK_SIZE, SEEK_SET);
	fwrite(dir, BLOCK_SIZE, 1, fp);
}
void reload_block(unsigned short i)//载入第i个数据块 
{
	fseek(fp, DATA_BLOCK + i*BLOCK_SIZE, SEEK_SET);
	fread(Buffer, BLOCK_SIZE, 1, fp);
}
void update_block(unsigned short i)//更新第i个数据块
{
	fseek(fp, DATA_BLOCK + i*BLOCK_SIZE, SEEK_SET);
	fwrite(Buffer, BLOCK_SIZE, 1, fp);
}
void reload_user(unsigned short i)
{
	fseek(fp, DATA_BLOCK + i*BLOCK_SIZE, SEEK_SET);
	fread(user, sizeof(user), 1, fp);
}
void update_user(unsigned short i)
{
	fseek(fp, DATA_BLOCK + i*BLOCK_SIZE, SEEK_SET);
	fwrite(user, sizeof(user), 1, fp);
}
/*  分配inode节点
*	分配的规则是，首先寻找这个块所在的指针块上面之前的近的块，
*	如果没找到，就去参数ind所在的间接块内找，如果还没有找到，
*	就去inode所在的磁道块组内找一个。
**/
int alloc_block()//分配一个数据块,返回数据块号; 
{
	unsigned short cur = last_alloc_block;
	unsigned char con = 128; //1000 0000B
	int flag = 0;
	//查看是否有空位
	if (super_block[0].bg_free_blocks_count == 0)
	{
		printf("There is no block to be alloced!\n");
		return(0);
	}
	// 刷新block 位图
	reload_block_bitmap();
	//__u8 个字节 分别8个数据块，作为一段
	cur = cur / 8;
	//找到一个该段有空位的 即不是全1
	while (bitbuf[cur] == 255)
	{
		if (cur == 511)
			cur = 0;
		else cur++;
	}
	// 顺序找出该段的第一个空位
	while (bitbuf[cur] & con)
	{
		con = con >> 2;
		flag++;
	}
	bitbuf[cur] = bitbuf[cur] + con;
	last_alloc_block = cur * 8 + flag;
	// 更新 block 位图
	update_block_bitmap();
	super_block[0].bg_free_blocks_count--;
	//更新 super block
	update_group_desc();
	return last_alloc_block;
}

/* 删除一个数据块
 * 修改 block 位图 和 super block
 */
void remove_block(unsigned short del_num) 
{
	unsigned short del_tmp = del_num / 8;
	unsigned char  du_t = del_num % 8;
	// 重新载入inode bitmap
	reload_block_bitmap();
	//将du_t位置0    0000 1111 & 1111 1000 = 0000 1000  ~ = 1111 0111
	bitbuf[del_tmp] &= ~((255 >> du_t) & (255 << (7 - du_t)));
	// 更新
	update_block_bitmap();
	// 更新 super block
	super_block[0].bg_free_blocks_count++;
	update_group_desc();
}
/*
 *   分配一个 inode  原理同block
 */
int get_inode()//分配一个inode,返回序号 
{
	unsigned short cur = last_alloc_inode;
	unsigned char con = 128;
	int flag = 0;
	if (super_block[0].bg_free_inodes_count == 0)
	{
		printf("There is no Inode to be alloced!\n");
		return 0;
	}
	reload_inode_bitmap();
	cur = (cur - 1) / 8;
	while (bitbuf[cur] == 255)
	{
		if (cur == 511)cur = 0;
		else cur++;
	}
	while (bitbuf[cur] & con)
	{
		con = con / 2;
		flag++;
	}
	bitbuf[cur] = bitbuf[cur] + con;
	last_alloc_inode = cur * 8 + flag + 1;
	update_inode_bitmap();
	super_block[0].bg_free_inodes_count--;
	update_group_desc();
	return last_alloc_inode;
}
/**
 *  移除一个inode占用
 */
void remove_inode(unsigned short del_num)
{
	unsigned short del_tmp = del_num / 8;
	unsigned char  du_t = del_num % 8;
	// 重新载入inode bitmap
	reload_inode_bitmap();
	//将du_t位置0    0000 1111 & 1111 1000 = 0000 1000  ~ = 1111 0111
	bitbuf[del_tmp] &= ~((255 >> du_t) & (255 << (7 - du_t)));
	// 更新
	update_inode_bitmap();
	// 更新 super block
	super_block[0].bg_free_inodes_count++;
	update_group_desc();
}
/**********          文件系统结构操作            *************/

/*
 *  对新目录初始化 添加 . 和 .. 文件
 *  初始化新文件
 *  目标inode号   文件长度 文件类型
 */
void dir_prepare(unsigned short tmp, unsigned short len, int type) 
{
	//获取新目录的 innode
	reload_inode_entry(tmp);
	if (type == 2)//目录 
	{
		inode_area[0].i_size = 32;
		// 占一个数据块
		inode_area[0].i_blocks = 1;
		inode_area[0].i_mtime = now_time();
		// 分配数据块
		inode_area[0].i_block[0] = alloc_block();
		dir[0].inode = tmp; // 该目录的索引节点 .
		dir[1].inode = current_dir; // 该目录的父节点 ..
		dir[0].name_len = len;	// 路径长度
		dir[1].name_len = current_dirlen;
		dir[0].file_type = dir[1].file_type = 2; //文件类型
		// 清空剩余位置的目录项
		for (type = 2; type<32; type++)
			dir[type].inode = 0; 
		strcpy(dir[0].name, ".");
		strcpy(dir[1].name, "..");
		// 写入数据
		update_dir(inode_area[0].i_block[0]);
		//d  rw-rw-r--
		inode_area[0].i_ctime = now_time();
		inode_area[0].i_mode.mode_bit.rwx_gup = inode_area[0].i_mode.mode_bit.rwx_own = 6;
		inode_area[0].i_mode.mode_bit.rwx_oth = 4;
	}
	else //文件
	{
		inode_area[0].i_size = 0;
		inode_area[0].i_blocks = 0;
		inode_area[0].i_ctime = now_time();
		//  -  rwxrwxr--
		inode_area[0].i_mode.mode_bit.rwx_gup = inode_area[0].i_mode.mode_bit.rwx_own = 7;
		inode_area[0].i_mode.mode_bit.rwx_oth = 4;
	}
	// 用户
	inode_area[0].i_uid = use_user.uid;
	update_inode_entry(tmp);
}

/* 
 *根据文件名 索引当前目录的文件 
 *返回目标文件的inode号 和 block 号, 目录项的位置
 */
unsigned short reserch_file(char tmp[9], int file_type,
							unsigned short *inode_num, 
							unsigned short *block_num, 
							unsigned short *dir_num)
{                      
	unsigned short j, k;
	//得到当前文件夹的节点
	reload_inode_entry(current_dir);
	j = 0;
	// 遍历当前文件夹下的所有文件
	while (j<inode_area[0].i_blocks)
	{
		// 得到文件夹
		reload_dir(inode_area[0].i_block[j]);
		k = 0;
		//遍历所有文件
		while (k<32)
		{
			// 找到对应的文件
			if (!dir[k].inode || dir[k].file_type != file_type || strcmp(dir[k].name, tmp))
				k++;
			else
			{
				*inode_num = dir[k].inode; //节点号
				*block_num = j; // 该项在当前文件夹下的直接文件号
				*dir_num = k; // 该项在文件中的位置
				return 1;
			}
		}
		j++;
	}
	return 0;
}

/*
 *  目录切换  只支持相对位置的目录切换
 *  通过当前文件夹下的目录名称
 */
void cd(char tmp[9])
{
	unsigned short i, j, k, flag;
	// 搜索目标文件夹
	flag = reserch_file(tmp, 2, &i, &j, &k);
	if (flag) // 存在
	{
		//进入目标文件夹
		current_dir = i;
		reload_inode_entry(current_dir);
		inode_area[0].i_atime = now_time();
		reload_inode_entry(current_dir);
		// 在不是根节点的情况下返回父目录 
		if (!strcmp(tmp, "..") && dir[k - 1].name_len)
		{
			// 去掉当前目录的路径
			current_path[strlen(current_path) - dir[k - 1].name_len - 1] = '\0';
			// 回退目录层数
			current_dirlen = dir[k].name_len;
		}
		// 到当前目录 
		else if (!strcmp(tmp, "."));
		// 到其他目录
		else if (strcmp(tmp, ".."))
		{
			// 修改路径长度
			current_dirlen = strlen(tmp);
			// 在路径后面添加
			strcat(current_path, tmp);
			strcat(current_path, "/");
		}
		// 根节点到父级目录 不做处理
	}
	else 
		printf("The directory %s not exists!\n", tmp);
}

/*
 *  删除文件
 *  通过文件名
 */
void del(char tmp[9])
{
	unsigned short i, j, k, m, n, flag;
	m = 0;
	// 收索 普通文件
	flag = reserch_file(tmp, 1, &i, &j, &k);
	if (flag) //  目标存在
	{
		flag = 0;
		// 搜索目标文件是否在已经打开的文件当中
		while (fopen_table[flag] != dir[k].inode&&flag<16)
			flag++;
		// 如果文件已经打开， 则关闭文件 （清0）
		if (flag<16)
			fopen_table[flag] = 0;
		// 刷新文件项缓冲
		reload_inode_entry(i);
		// 移除该文件下的所有已分配的Block
		while (m<inode_area[0].i_blocks)
			remove_block(inode_area[0].i_block[m++]);
		// 清空文件数
		inode_area[0].i_blocks = 0;
		inode_area[0].i_size = 0;
		// 移除该文件的inode
		remove_inode(i);
		// 刷新当前节点缓冲
		reload_inode_entry(current_dir);
		// 清除目录项
		dir[k].inode = 0;
		// 更改文件大小
		update_dir(inode_area[0].i_block[j]);
		inode_area[0].i_size -= sizeof(struct dir_entry);
		m = 1;
		while (m < inode_area[i].i_blocks)
		{
			flag = n = 0;
			reload_dir(inode_area[0].i_block[m]);
			while (n<32)
			{
				if (!dir[n].inode)
					flag++;// 文件s数目
				n++;
			}
			if (flag == 32) //移位
			{
				remove_block(inode_area[i].i_block[m]);
				inode_area[i].i_blocks--;
				while (m<inode_area[i].i_blocks)
				{
					inode_area[i].i_block[m] = inode_area[i].i_block[m+1];
					m++;
				}
			}
		}
		update_inode_entry(current_dir);
	}
	else printf("The file %s not exists!\n", tmp);
}

/*
 *  创建文件夹
 */
void makedir(char tmp[9])
{
	int type = 2;
	unsigned short tmpno, i, j, k, flag;
	//刷新当前目录的索引节点
	reload_inode_entry(current_dir); 
	if (!reserch_file(tmp, type, &i, &j, &k)) //没有同名文件
	{
		if (inode_area[0].i_size == 512*8) //目录项满了
		{
			printf("Directory has no space to be alloced!\n");
			return;
		}
		flag = 1;
		//目录中有某些个块有空位
		if (inode_area[0].i_size != inode_area[0].i_blocks * 512)
		{
			i = 0;
			// 找到空位
			while (flag && i < inode_area[0].i_blocks)
			{
				reload_dir(inode_area[0].i_block[i]);
				j = 0;
				while (j<32)
				{
					if (dir[j].inode == 0)
					{
						flag = 0;
						break;
					}
					j++;
				}
				i++;
			}
			// 分配inode
			tmpno = dir[j].inode = get_inode();
			dir[j].name_len = strlen(tmp);
			dir[j].file_type = type;
			strcpy(dir[j].name, tmp);
			// 写入目录项
			update_dir(inode_area[0].i_block[i - 1]);
		}
		else//全满
		{
			//再添加一块
			inode_area[0].i_block[inode_area[0].i_blocks] = alloc_block();
			inode_area[0].i_blocks++;
			reload_dir(inode_area[0].i_block[inode_area[0].i_blocks - 1]);
			//分配节点
			tmpno = dir[0].inode = get_inode();
			dir[0].name_len = strlen(tmp);
			dir[0].file_type = type;
			strcpy(dir[0].name, tmp);
			//初始化新块
			for (flag = 1; flag<32; flag++)
				dir[flag].inode = 0;
			update_dir(inode_area[0].i_block[inode_area[0].i_blocks - 1]);
		}
		//改变大小
		inode_area[0].i_size += sizeof(struct dir_entry);
		inode_area[0].i_ctime = now_time();
		update_inode_entry(current_dir);
		dir_prepare(tmpno, strlen(tmp), type);
	}
	else  //已经存在同名文件或目录
	{
		if (type == 1)printf("File has already existed!\n");
		else printf("Directory has already existed!\n");
	}
}

/*
 *  新建普通文件， 过程同建立文件夹
 **/
void mkf(char tmp[9])
{
	int type = 1;
	unsigned short tmpno, i, j, k, flag;
	//获得当前目录的索引节点给inode_area[0]
	reload_inode_entry(current_dir); 
	//未找到同名文件
	if (!reserch_file(tmp, type, &i, &j, &k)) 
	{
		inode_area[0].i_uid = use_user.uid;
		if (inode_area[0].i_size == 4096) //目录项已满
		{
			printf("Directory has no room to be alloced!\n");
			return;
		}
		flag = 1;
		//目录中有某些个块中有空位
		if (inode_area[0].i_size != inode_area[0].i_blocks * 512)
		{
			i = 0;
			while (flag&&i<inode_area[0].i_blocks)
			{
				reload_dir(inode_area[0].i_block[i]);
				j = 0;
				while (j<32)
				{
					if (dir[j].inode == 0)
					{
						flag = 0;
						break;
					}
					j++;
				}
				i++;
			}
			tmpno = dir[j].inode = get_inode();
			dir[j].name_len = strlen(tmp);
			dir[j].file_type = type;
			strcpy(dir[j].name, tmp);
			update_dir(inode_area[0].i_block[i - 1]);
		}
		else//全满
		{
			inode_area[0].i_block[inode_area[0].i_blocks] = alloc_block();
			inode_area[0].i_blocks++;
			reload_dir(inode_area[0].i_block[inode_area[0].i_blocks - 1]);
			tmpno = dir[0].inode = get_inode();
			dir[0].name_len = strlen(tmp);
			dir[0].file_type = type;
			strcpy(dir[0].name, tmp);
			//初始化新块
			for (flag = 1; flag<32; flag++)
				dir[flag].inode = 0;
			update_dir(inode_area[0].i_block[inode_area[0].i_blocks - 1]);
		}
		inode_area[0].i_size += sizeof(struct dir_entry);
		inode_area[0].i_ctime = now_time();
		update_inode_entry(current_dir);
		dir_prepare(tmpno, strlen(tmp), type);
	}
	else  //已经存在同名文件
	{
		printf("File has already existed!\n");
	}
}

/*
 * 删除文件夹
 */
void rmdir(char tmp[9])
{
	unsigned short i, j, k, flag;
	unsigned short m, n;
	if (!strcmp(tmp, "..") || !strcmp(tmp, "."))
	{
		printf("The directory can not be deleted!\n");
		return;
	}
	flag = reserch_file(tmp, 2, &i, &j, &k);
	if (flag)
	{
		//找到要删除的目录的节点并载入
		reload_inode_entry(dir[k].inode); 
		// 空文件夹 只有.and ..
		if (inode_area[0].i_size == 32) 
		{
			//信息清0
			inode_area[0].i_size = 0;
			inode_area[0].i_blocks = 0;
			// 移除块
			remove_block(inode_area[0].i_block[0]);
			//得到当前目录项节点
			reload_inode_entry(current_dir);
			// 从目录项中移除
			remove_inode(dir[k].inode);
			dir[k].inode = 0;
			// 更新
			update_dir(inode_area[0].i_block[j]);
			inode_area[0].i_size -= sizeof(struct dir_entry);
			flag = 0;
			m = 1;
			//移位操作
			while (flag<32 && m<inode_area[0].i_blocks)
			{
				flag = n = 0;
				reload_dir(inode_area[0].i_block[m]);
				while (n<32)
				{
					if (!dir[n].inode)flag++;
					n++;
				}
				if (flag == 32)
				{
					remove_block(inode_area[0].i_block[m]);
					inode_area[0].i_blocks--;
					while (m < inode_area[0].i_blocks)
					{
						inode_area[0].i_block[m] = inode_area[0].i_block[m + 1];
						m++;
					}
				}
			}
			update_inode_entry(current_dir);
		}
		else printf("Directory is not null!\n");
	}
	else printf("Directory to be deleted not exists!\n");
}
/*
 *  将权限信息可视化
 */
void permission_show_char(char p[10], unsigned int permiss)
{
	int i;
	unsigned int b = 256; //1 0000 0000
	strcpy(p, "rwxrwxrwx");
	for (i = 0; i < 9; i++, b >>= 1)
	{
		if ((permiss & b) == 0) //保留 对应位 其他位置0
			p[i] = '-';
	}
}

/*
 *  列出向前目录中的所有项目
 **/
void ls()
{
	struct tm *pt;
	char per[10];
	unsigned short i, j, k,u;
	pt = malloc(sizeof(pt));
	i = 0;
	printf("Names       Type           Mode          ctime            Size           User<Uid>\n"); 
	// 获取当前目录项
	reload_inode_entry(current_dir);
	// 遍历当前目录项
	while (i<inode_area[0].i_blocks)
	{
		k = 0;
		reload_dir(inode_area[0].i_block[i]);
		while (k<32)
		{
			// 节点不为空
			if (dir[k].inode)
			{
				
				printf("%s", dir[k].name);
				// 文件夹
				if (dir[k].file_type == 2)
				{
					j = 0;
					reload_inode_entry(dir[k].inode);
					if (!strcmp(dir[k].name, ".."))
						while (j++<10)printf(" ");
					else if (!strcmp(dir[k].name, "."))
						while (j++<11)printf(" ");
					else while (j++<12 - dir[k].name_len)
						printf(" ");
					printf("d              "); 
					permission_show_char(per, inode_area[0].i_mode.i_mode);
					// 权限可视化转换
					printf("%s",per );
					printf("         ----");

					/*pt = localtime((time_t*)&inode_area[0].i_ctime);
					printf("%d:%d",pt->tm_hour+8, pt->tm_min);*/
					for (u = 0; u < super_block[0].bg_user_count; u++)
					{
						if (user[u].uid == inode_area[0].i_uid)
						{
							printf("\t\t%s", user[u].u_name);
							printf("<%d>", inode_area[0].i_uid);
						}
					}
				}
				// 普通文件
				else if (dir[k].file_type == 1)
				{
					j = 0;
					reload_inode_entry(dir[k].inode);
					while (j++<12 - dir[k].name_len)
						printf(" ");
					
					printf("-              ");
					permission_show_char(per, inode_area[0].i_mode.i_mode);
					printf("%s", per);
					//pt = localtime(&inode_area[0].i_ctime);
					//printf("%d:%d", pt->tm_hour+8, pt->tm_min);
					//printf("         %d bytes", inode_area[0].i_size);
					for (u = 0; u < super_block[0].bg_user_count; u++)
					{
						if (user[u].uid == inode_area[0].i_uid)
						{
							printf("    \t%s", user[u].u_name);
							printf("<%d>", inode_area[0].i_uid);
						}
					}
				}
				
				printf("\n");
			} 
			k++;
			reload_inode_entry(current_dir);
		}
		i++;
	}
}

/******           文件操作              *****/
/*
 *  搜索已经打开的文件
 **/
unsigned short search_file(unsigned short Ino)
{
	unsigned short fopen_table_point = 0;
	// 遍历
	while (fopen_table_point<16 && fopen_table[fopen_table_point++] != Ino);
	if (fopen_table_point == 16)
		return 0;
	return 1;
}

/*
 *  读取文件
 **/
void read_file(char tmp[9])
{
	unsigned short flag, i, j, k;
	// 再当前目录中搜索文件
	flag = reserch_file(tmp, 1, &i, &j, &k); 
	if (flag) //文件存在
	{
		// 是否已经打开
		if (search_file(dir[k].inode))
		{

			reload_inode_entry(dir[k].inode);
			// 权限验证
			if(use_user.uid == inode_area[0].i_uid) //拥有者为当前用户
			{
				if (inode_area[0].i_mode.mode_bit.rwx_own  < 4)// 小于 100  
				{
					printf("The file %s can not be read!\n", tmp);
					return;
				}
			}
			else //其他用户
			{
				if (inode_area[0].i_mode.mode_bit.rwx_oth  < 4)// 小于 100  
				{
					printf("The file %s can not be read!\n", tmp);
					return;
				}
			}
			//访问时间
			inode_area[0].i_atime = now_time();

			for (flag = 0; flag<inode_area[0].i_blocks; flag++)
			{
				reload_block(inode_area[0].i_block[flag]);
				Buffer[511] = '\0';
				printf("%s", Buffer);
			}
			if (flag == 0)printf("The file %s is empty!\n", tmp);
			else printf("\n");
		}
		else printf("The file %s has not been opened!\n", tmp);
	}
	else printf("The file %s not exists!\n", tmp);
}

/*
 *  写文件 同读取
 */
void write_file(char tmp[9])
{
	unsigned short flag, i, j, k, size = 0, need_blocks;
	flag = reserch_file(tmp, 1, &i, &j, &k);
	if (flag)
	{
		if (search_file(dir[k].inode))
		{
			reload_inode_entry(dir[k].inode);
			//权限验证
			if(use_user.uid == inode_area[0].i_uid)
			{ 
				if (! (inode_area[0].i_mode.mode_bit.rwx_own & 2))// 010 写权限
				{
					printf("The file %s can not be writed!\n", tmp);
					return;
				}
			}
			else
			{
				if (! (inode_area[0].i_mode.mode_bit.rwx_oth  & 2))// 010 写权限
				{
					printf("The file %s can not be writed!\n", tmp);
					return;
				}
			}
			while (1)
			{
				tempbuf[size] = getchar();
				if (tempbuf[size] == '#')
				{
					tempbuf[size] = '\0';
					break;
				}
				if (size >= 4096)
				{
					printf("Sorry,the max size of a file is 4KB!\n");
					tempbuf[size] = '\0';
					break;
				}
				size++;
			}
			need_blocks = strlen(tempbuf) / BLOCK_SIZE;
			if (strlen(tempbuf) % BLOCK_SIZE)need_blocks++;
			if (need_blocks < 9)
			{
				while (inode_area[0].i_blocks<need_blocks)
				{
					inode_area[0].i_block[inode_area[0].i_blocks] = alloc_block();
					inode_area[0].i_blocks++;
				}
				j = 0;
				while (j<need_blocks)
				{
					if (j != need_blocks - 1)
					{
						reload_block(inode_area[0].i_block[j]);
						memcpy(Buffer, tempbuf + j * BLOCK_SIZE, BLOCK_SIZE);
						update_block(inode_area[0].i_block[j]);
					}
					else
					{
						reload_block(inode_area[0].i_block[j]);
						memcpy(Buffer, tempbuf + j * BLOCK_SIZE, strlen(tempbuf) - j*BLOCK_SIZE);
						if (strlen(tempbuf)>inode_area[0].i_size)
						{
							Buffer[strlen(tempbuf) - j * BLOCK_SIZE] = '\0';
							inode_area[0].i_size = strlen(tempbuf);
						}
						update_block(inode_area[0].i_block[j]);
					}
					j++;
				}
				inode_area[0].i_mtime = now_time();
				update_inode_entry(current_dir);
			}
			else printf("Sorry,the max size of a file is 4KB!\n");
		}
		else printf("The file %s has not opened!\n", tmp);
	}
	else printf("The file %s does not exist!\n", tmp);
}

/*
 * 关闭文件
 **/
void close_file(char tmp[9])
{
	unsigned short flag, i, j, k;
	flag = reserch_file(tmp, 1, &i, &j, &k);
	if (flag) // 文件存在
	{
		if (search_file(dir[k].inode))
		{
			flag = 0;
			// 得到位置
			while (fopen_table[flag] != dir[k].inode)
				flag++;
			fopen_table[flag] = 0;
			printf("File: %s! closed\n", tmp);
		}
		else printf("The file %s has not been opened!\n", tmp);
	}
	else printf("The file %s does not exist!\n", tmp);
}

/*
 *  打开文件
 **/
void open_file(char tmp[9])
{
	unsigned short flag, i, j, k;
	flag = reserch_file(tmp, 1, &i, &j, &k);
	if (flag)
	{
		if (search_file(dir[k].inode))
			printf("The file %s has opened!\n", tmp);
		else
		{
			flag = 0;
			while (fopen_table[flag])
				flag++;
			fopen_table[flag] = dir[k].inode;
			printf("File %s! opened\n", tmp);

		}
	}
	else printf("The file %s does not exist!\n", tmp);
}

/************              内存格式化操作                  ***************/
void initialize_disk()
{
	int i = 0,b;
	printf("Creating the ext2 file system\n");
	printf("Please wait ");
	while (i<1)
	{
		printf("... ");
		//sleep(1);
		i++;
	}
	printf("\n");
	last_alloc_inode = 1;
	last_alloc_block = 0;
	for (i = 0; i<16; i++)
		fopen_table[i] = 0;//清空文件打开缓冲表
	for (i = 0; i<BLOCK_SIZE; i++)
		Buffer[i] = 0;// 清空缓冲区
	fp = fopen("fsmext2", "w+b");
	fseek(fp, DISK_START, SEEK_SET);
	
	reload_group_desc();
	reload_inode_entry(1);
	reload_dir(0);
	strcpy(current_path, " /");  //根
	//改卷名，初始化组描述符内容
	strcpy(super_block[0].bg_volume_name, "MEXT2"); 
	super_block[0].bg_block_bitmap = BLOCK_BITMAP;
	super_block[0].bg_inode_bitmap = INODE_BITMAP;
	super_block[0].bg_inode_table = INODE_TABLE;
	super_block[0].bg_free_blocks_count = 4096;
	super_block[0].bg_free_inodes_count = 4096;
	super_block[0].bg_used_dirs_count = 0;// 初始化组描述符内容
	update_group_desc(); //更新组描述符内容
	// 重载 位图
	reload_block_bitmap();
	reload_inode_bitmap();
	// 创建根目录
	inode_area[0].i_mode.i_mode = 438; // u g o 都具有 r w 权限
	inode_area[0].i_uid = 0;  // root 用户的UID = 0
	inode_area[0].i_blocks = 0;
	inode_area[0].i_size = sizeof(struct dir_entry)*2;
	inode_area[0].i_atime = now_time();
	inode_area[0].i_ctime = now_time();
	inode_area[0].i_mtime = now_time();
	inode_area[0].i_dtime = 0;
	inode_area[0].i_block[0] = alloc_block();
	inode_area[0].i_blocks++;
	current_dir = get_inode();
	update_inode_entry(current_dir);
	dir[0].inode = dir[1].inode = current_dir;
	dir[0].name_len = 0;
	dir[1].name_len = 0;
	dir[0].file_type = dir[1].file_type = 2;//1:文件;2:目录 
	strcpy(dir[0].name, ".");
	strcpy(dir[1].name, "..");
	update_dir(inode_area[0].i_block[0]);
	// 创建 root 用户
	b = alloc_block();
	super_block[0].bg_user_block = b;
	super_block[0].bg_user_count = 1;
	update_group_desc();
	user[0].uid = 0;
	user[0].gid = 0;
	strcpy(user[0].u_name , "root");
	strcpy(user[0].u_psw , "root");
	update_user(b);
	printf("%d,",user[0].uid);
	printf("The ext2 file system has been installed!\n");
}
void initialize_memory()
{
	int i = 0;
	last_alloc_inode = 1;
	last_alloc_block = 0;
	for (i = 0; i<16; i++)
		fopen_table[i] = 0;
	strcpy(current_path, " /");
	current_dir = 1;
	fp = fopen("fsmext2", "r+b");
	// 文件不存在 创建文件并初始化
	if (fp == NULL)
	{
		printf("The File system does not exist!\n");
		initialize_disk();
		while (!user_login());
		return;
	}
	fseek(fp, DISK_START, SEEK_SET);
	reload_group_desc();
	fclose(fp);
	fp = fopen("fs_mext2.mext2", "r+b");
	reload_inode_entry(current_dir);
	reload_user(super_block[0].bg_user_block);
	//用户登录
	printf("%d %s", super_block[0].bg_user_count, user[0].u_name);
	while (!user_login());

}
/*
 * 格式化磁盘
 **/
void format(char tmp[9])
{
	fclose(fp);
	initialize_disk();
}

/* 帮助文档
 **/
void help(char tmp[9])
{
	printf("   ****************************************************************************\n");
	printf("   *                        Mext2 of ext2 file system                         *\n");
	printf("   *                                                        11403070327       *\n");
	printf("   * 1.change dir   : cd+dir_name          7.create  dir  : mkdir+dir_name    *\n");
	printf("   * 2.create file  : touch+file_name      8.delete  dir  : rmdir+dir_name    *\n");
	printf("   * 3.delete file  : rm+file_name         9.read    file : read+file_name    *\n");
	printf("   * 4.open   file  : open+file_name       10.write  file : write+file_name   *\n");
	printf("   * 5.close  file  : close+file_name      11.logoff      : quit              *\n");
	printf("   * 6.list   items : ls                   12.this   menu : help              *\n");
	printf("   * 13.format disk : format               14.change permission :chmod+file   *\n");
	printf("   ****************************************************************************\n");
}

/*
 * 退出程序
 **/
void quit(char name[9])
{
	fclose(fp);
	printf("Bye~\n");
	exit(1);
}
