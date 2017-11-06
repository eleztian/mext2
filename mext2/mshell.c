#include <stdio.h>
#include <string.h>
#include "mshell.h"
#include "mext2.h"
#include "user.h"
#define ORDER_NUM 15

char Order_list[][ORDER_NUM] = {
	"cd",
	"touch",
	"rm",
	"open",
	"close",
	"mkdir",
	"rmdir",
	"read",
	"write",
	"addusr",
	"su",
	"ls",
	"format",
	"help",
	"quit",
};
void(*order_hs[])(char f_name[9]) = {
	cd,
	mkf,
	del,
	open_file,
	close_file,
	makedir,
	rmdir,
	read_file,
	write_file,
	create_user,
	change_user,
	ls,
	format,
	help,
	quit,
};
extern user_info use_user;
extern char current_path[];
char order_hed[9];
char order_argv[9];

int  mshell_get_shell();
void mshell_start();
void  mshell_parsing();

/* 命令行提示输出  */
void mshell_tips()
{
	printf("\n[%s@mshell %s]#", use_user.u_name,current_path);
}

/* mshell 入口函数 初始化  */
void mshell_init()
{
	mshell_tips();
	mshell_start();
}

/* mshell 开始函数 命令解析入口  */
void mshell_start()
{
	while (1)
	{
		mshell_get_shell();
		mshell_parsing();
	}
}

/* 获取用户命令输入 */
int mshell_get_shell()
{
	mshell_tips();
	setbuf(stdin, NULL);
	scanf("%s", order_hed);
	if (strlen(order_hed))
		return 1;
	return 0;
}

/* mshell 命令解析 */
void mshell_parsing()
{
	int i;
	/* 判断是哪个命令  */
	for (i = 0; i < ORDER_NUM; i++)
	{
		if (strcmp(order_hed, Order_list[i]) == 0)
		{
			//printf("%d %s\n", i, order_hed);
			if (i > 10 )
			{
				order_hs[i](order_argv);
				return ;
			}
			else //存在参数
			{
				scanf("%s", order_argv);
				order_hs[i](order_argv);
				return ;
			}
		}
	}
	printf("no command!");
}
