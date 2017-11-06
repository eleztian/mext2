#ifndef _USER_H
#define _USER_H



/* 用户组 信息  */
typedef struct {
	unsigned int gid;
	char g_name[8];
}group_info;

/* 用户 信息 */
typedef struct {
	unsigned int uid;
	unsigned int gid;
	char u_name[8];
	char u_psw[8];
}user_info;

int user_login();
void create_user(char name[9]);
void change_user(char name[9]); 
#endif
