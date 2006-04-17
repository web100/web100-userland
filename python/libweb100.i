%module libweb100
%{
#include "web100.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct web100_readbuf {
	char padding[128];
};
%}

%include "web100.h"
%include cpointer.i

%pointer_functions(unsigned short, u16p)
%pointer_functions(int, s32p)
%pointer_functions(unsigned int, u32p)
%pointer_functions(unsigned long long, u64p)

%pointer_functions(struct web100_readbuf, bufp)
%pointer_cast(struct web100_readbuf *, unsigned short *, bufp_to_u16p)
%pointer_cast(struct web100_readbuf *, int *, bufp_to_s32p)
%pointer_cast(struct web100_readbuf *, unsigned int *, bufp_to_u32p)
%pointer_cast(struct web100_readbuf *, unsigned long long *, bufp_to_u64p)

%pointer_cast(char *, struct web100_readbuf *, str_to_bufp)
