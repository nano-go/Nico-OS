#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#define T_FILE   1
#define T_DIR    2
#define T_DEVICE 3

struct stat {
    short st_type;
    unsigned int st_ino;
    unsigned int st_size;
    short st_nlink;
};

#endif /* _SYS_STAT_H */