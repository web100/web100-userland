/*
 * Copyright (c) 2001
 *      Carnegie Mellon University, The Board of Trustees of the University
 *      of Illinois, and University Corporation for Atmospheric Research.
 *      All rights reserved.  This software comes with NO WARRANTY.
 *
 * Since our code is currently under active development we prefer that
 * everyone gets the it directly from us.  This will permit us to
 * collaborate with all of the users.  So for the time being, please refer
 * potential users to us instead of redistributing web100.
 *
 * $Id: web100-int.h,v 1.2 2002/01/14 18:37:59 jestabro Exp $
 */
#ifndef _WEB100_INT_H
#define _WEB100_INT_H

#include "web100.h"

#ifdef DEBUG
#define IFDEBUG(a) (a)
#else
#define IFDEBUG(a)  
#endif

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define WEB100_VERSTR_LEN_MAX       64
#define WEB100_GROUPNAME_LEN_MAX    32
#define WEB100_VARNAME_LEN_MAX      32

#define WEB100_ROOT_DIR     "/proc/web100/"
#define WEB100_HEADER_FILE  WEB100_ROOT_DIR "header"

struct web100_agent_info_local {
    struct web100_group*      group_head;
    struct web100_connection* connection_head;
};

struct web100_agent {
    int  type;
    char version[WEB100_VERSTR_LEN_MAX];
    
    union {
        struct web100_agent_info_local local;
    } info;
};

struct web100_group_info_local {
    struct web100_var*   var_head;
    struct web100_group* next;
};

struct web100_group {
    char                 name[WEB100_GROUPNAME_LEN_MAX];
    int                  size;
    int                  nvars;
    struct web100_agent* agent;
    
    union {
        struct web100_group_info_local local;
    } info;
};

struct web100_var_info_local {
    struct web100_var*   next;
};

struct web100_var {
    char                 name[WEB100_VARNAME_LEN_MAX];
    int                  type;
    int                  offset;
    struct web100_group* group;
    
    union {
        struct web100_var_info_local local;
    } info;
};

struct web100_connection_info_local {
    struct web100_connection    *next;
};

struct web100_connection {
    int                           cid;
    struct web100_connection_spec spec;
    struct web100_agent*          agent;

    int                           errno;
    int                           log;
    
    union {
        struct web100_connection_info_local local;
    } info;
};

struct web100_snapshot {
    struct web100_group*      group;
    struct web100_connection* connection;
    void*                     data;
};

#endif /* _WEB100_INT_H */
