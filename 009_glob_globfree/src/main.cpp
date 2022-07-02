#include <glob.h>
#include <unistd.h>

int main()
{
    glob_t globbuf;

    globbuf.gl_offs = 2;
    glob("*.sh", GLOB_DOOFFS, 0, &globbuf);
    glob("../*.sh", GLOB_DOOFFS | GLOB_APPEND, 0, &globbuf);
    globbuf.gl_pathv[0] = "ls";
    globbuf.gl_pathv[1] = "-l";
    execvp("ls", &globbuf.gl_pathv[0]);

    globfree(&globbuf);

    return 0;
}

