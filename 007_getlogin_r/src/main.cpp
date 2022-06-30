#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    	struct  passwd *pwd;
	pwd = getpwuid(getuid());
	char * user = getenv("USER");
	char *name = getlogin();      //使用sudo的时候也显示当前登录的用户名
	char loginName[256];
	getlogin_r(loginName,256);

	printf("getpwuid:%s\n", pwd->pw_name);
	printf("getenv:%s\n", user);
	printf("getlogin:%s\n", name);
	printf("getlogin_r:%s\n", loginName);
	
	return 0;

}

