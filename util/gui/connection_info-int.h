#include <sys/types.h>
#include <sys/param.h>
#include <web100/web100.h>
#include "connection_info.h"

struct connection_info {
    int cid; 
    pid_t pid;
    uid_t uid;
    ino_t ino;
    int state;
    char cmdline[PATH_MAX];
    int addrtype;
    struct web100_connection_spec spec;
    struct web100_connection_spec_v6 spec_v6;
    struct connection_info *next, *prev;
};
