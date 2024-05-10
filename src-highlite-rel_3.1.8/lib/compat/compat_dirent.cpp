#include "compat_dirent.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>


DIR *opendir(const char *name)
{
        int len;
        DIR *p;
        p = (DIR*)malloc(sizeof(DIR));
        memset(p, 0, sizeof(DIR));
        strncpy(p->dd_name, name, PATH_MAX);
        len = strlen(p->dd_name);
        p->dd_name[len] = '/';
        p->dd_name[len+1] = '*';

        if (p == NULL)
                return NULL;

        p->dd_handle = _findfirst(p->dd_name, &p->dd_dta);

        if (p->dd_handle == -1) {
                free(p);
                return NULL;
        }
        return p;
}

int closedir(DIR *dir)
{
        _findclose(dir->dd_handle);
        free(dir);
        return 0;
}

struct dirent *readdir(DIR *dir)
{
    struct dirent *result = 0;

    if(dir && dir->dd_handle != -1)
    {
        if(!dir->dd_dir.d_name || _findnext(dir->dd_handle, &dir->dd_dta) != -1)
        {
            result         = &dir->dd_dir;
            strcpy(dir->dd_dir.d_name, dir->dd_dta.name);
            ++dir->dd_stat;
        }
    }
    else
    {
        errno = EBADF;
    }

    return result;
}
