#include <sys/types.h>
#include "web100.h"
#include "connectioninfo.h"

struct connection_info {
    int                              cid; 
    pid_t                            pid;
    uid_t                            uid;
    ino_t                            ino;
    int                              state;
    char                             cmdline[256];
    WEB100_ADDRTYPE                  addrtype;
    struct web100_connection_spec    spec;
    struct web100_connection_spec_v6 spec_v6;
    struct connection_info           *next, *prev;
};
