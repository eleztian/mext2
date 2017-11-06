#include<stdio.h>
#include "user.h"
#include "mext2.h"
#include "string.h"
/* 用户信息存储在。。。 */
group_info group;
user_info user[10];
user_info use_user;
extern struct group_desc super_block[1];
extern struct inode inode_area[1];
int getuid()
{
	return -1;
}
int getgid(char name[9])
{
	return 1;
}
void create_user(char name[9])
{
	char u_psw[9];
	int i;
	printf("%s 's password: ", name);
	setbuf(stdin, NULL);
	scanf("%s", u_psw);
	strcpy(user[super_block[0].bg_user_count].u_name, name);
	strcpy(user[super_block[0].bg_user_count].u_psw, u_psw);
	user[super_block[0].bg_user_count].gid = 1;
	user[super_block[0].bg_user_count].uid = super_block[0].bg_user_count + 1;
	super_block[0].bg_user_count++;
	update_group_desc();
	update_user(super_block[0].bg_user_block);
	printf("create sucessful\n");
	reload_user(super_block[0].bg_user_block);
	for (i = 0; i < super_block[0].bg_user_count; i++)
		printf("%s\n", user[i].u_name);
}

int read_user_info()
{
	//TODO: 读取用户信息 从文件
	return 0;
}

int user_login()
{
	char username[8], userpsw[8];
	int num = super_block[0].bg_user_count;
	int i;
	printf("Login as:");
	scanf("%s", username);
	
	for (i = 0; i < num; i++)
	{
		if (strcmp(user[i].u_name, username) == 0)
		{
			printf("Input %s 's password:\n", username);
			setbuf(stdin, NULL);
			scanf("%s", userpsw);
			if (strcmp(user[i].u_psw, userpsw) == 0)
			{
				use_user = user[i]; 
				inode_area[0].i_uid = use_user.uid;
				return 1;
			}
			else
			{
				printf("%s %s", user[i].u_psw, userpsw);
				printf("password error\n");
			}
			break;
		}
	}
	//TODO:
	return 0;
}
void change_user(char name[9])
{
	int num = super_block[0].bg_user_count;
	char u_psw[9] = { 0 };
	int i;
	for (i = 0; i < num; i++)
	{
		if (strcmp(user[i].u_name, name) == 0)
		{
			printf("Input %s 's password:\n", name);
			setbuf(stdin, NULL);
			scanf("%s", u_psw);
			if (strcmp(user[i].u_psw, u_psw) == 0)
			{
				inode_area[0].i_uid = use_user.uid;
				use_user = user[i];
			}
			else
			{
				printf("%s %s", user[i].u_psw, u_psw);
				printf("password error\n");
			}
			return ;
		}
	}
	printf("user not exit\n");
}