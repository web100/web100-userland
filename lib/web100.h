/*
 * Copyright (c) 2001 Carnegie Mellon University,
 *                    The Board of Trustees of the University of Illinois,
 *                    and University Corporation for Atmospheric Research.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Since our code is currently under active development we prefer that
 * everyone gets the it directly from us.  This will permit us to
 * collaborate with all of the users.  So for the time being, please refer
 * potential users to us instead of redistributing web100.
 *
 * * $Id: web100.h,v 1.6 2002/02/12 16:48:26 engelhar Exp $
 */
#ifndef _WEB100_H
#define _WEB100_H

#include <sys/types.h>
#include <sys/param.h>

typedef enum {
    WEB100_TYPE_INTEGER = 0,
    WEB100_TYPE_INTEGER32,
    WEB100_TYPE_IP_ADDRESS,
    WEB100_TYPE_COUNTER32,
    WEB100_TYPE_GAUGE32,
    WEB100_TYPE_UNSIGNED32,
    WEB100_TYPE_TIME_TICKS,
    WEB100_TYPE_COUNTER64,
    WEB100_TYPE_UNSIGNED16
} WEB100_TYPE;

struct web100_connection_spec {
    u_int16_t dst_port;
    u_int32_t dst_addr;
    u_int16_t src_port;
    u_int32_t src_addr;
};

struct web100_socket_data { 
    int cid; 
    struct web100_connection_spec spec;
    ino_t ino;
    pid_t pid;
    uid_t uid;
    int state;
    char cmdline[PATH_MAX];
    struct web100_socket_data *next;
};

/* Agent types */
#define WEB100_AGENT_TYPE_LOCAL 0

#define WEB100_VERSTR_LEN_MAX       64
#define WEB100_GROUPNAME_LEN_MAX    32
#define WEB100_VARNAME_LEN_MAX      32

#define WEB100_ROOT_DIR     "/proc/web100/"
#define WEB100_HEADER_FILE  WEB100_ROOT_DIR "header"

/* Error codes.  If you update these, be sure to update web100_sys_errlist. */
#define WEB100_ERR_SUCCESS         0
#define WEB100_ERR_FILE            1
#define WEB100_ERR_AGENT_TYPE      2
#define WEB100_ERR_NOMEM           3
#define WEB100_ERR_NOCONNECTION    4
#define WEB100_ERR_INVAL           5
#define WEB100_ERR_HEADER          6
#define WEB100_ERR_NOVAR           7
#define WEB100_ERR_NOGROUP         8

extern int               web100_errno;
extern const char* const web100_sys_errlist[];
extern int               web100_sys_nerr;

/* The following are opaque types. */
typedef struct web100_agent       web100_agent;
typedef struct web100_group       web100_group;
typedef struct web100_var         web100_var;
typedef struct web100_connection  web100_connection;
typedef struct web100_snapshot    web100_snapshot;

void               web100_perror(const char* _str);
const char*        web100_strerror(int _errnum);

web100_agent*      web100_attach(int _type, void* _data);
void               web100_detach(web100_agent* _agent);

int                web100_agent_find_var_and_group(web100_agent* _agent, const char* _varname, web100_group** _group, web100_var** _var);

web100_group*      web100_group_head(web100_agent* _agent);
web100_group*      web100_group_next(web100_group* _group);
web100_group*      web100_group_find(web100_agent* _agent, char* _name);

web100_var*        web100_var_head(web100_group* _group);
web100_var*        web100_var_next(web100_var* _var);
web100_var*        web100_var_find(web100_group* _group, const char* _name);

web100_connection* web100_connection_head(web100_agent* _agent);
web100_connection* web100_connection_next(web100_connection* _conn);
web100_connection* web100_connection_find(web100_agent* _agent, struct web100_connection_spec* _spec);
web100_connection* web100_connection_lookup(web100_agent* _agent, int _cid);
int                web100_connection_data_copy(web100_connection* _dest, web100_connection* _src);

web100_snapshot*   web100_snapshot_alloc(web100_group* _group, web100_connection* _conn);
void               web100_snapshot_free(web100_snapshot* _snap);
int                web100_snap(web100_snapshot* _snap);

web100_group*      web100_snap_group(web100_snapshot* _snap);

int                web100_raw_read(web100_var* _var, web100_connection* _conn, void* _buf);
int                web100_raw_write(web100_var* _var, web100_connection* _conn, void* _buf);

int                web100_snap_read(web100_var* _var, web100_snapshot* _snap, void* _buf);
int                web100_delta_any(web100_var* _var, web100_snapshot* _s1, web100_snapshot* _s2, void* _buf);
int                web100_snap_data_copy(web100_snapshot* _dest, web100_snapshot* _src);

char*              web100_value_to_text(WEB100_TYPE _type, void* buf);

int                web100_get_agent_type(web100_agent* _agent);
const char*        web100_get_agent_version(web100_agent* _agent);

const char*        web100_get_group_name(web100_group* _group);
int                web100_get_group_size(web100_group* _group);
int                web100_get_group_nvars(web100_group* _group);

const char*        web100_get_var_name(web100_var* _var);
int                web100_get_var_type(web100_var* _var);

const char*        web100_get_snap_group_name(web100_snapshot* _snap);

int                web100_get_connection_cid(web100_connection* _conn);
void               web100_get_connection_spec(web100_connection* _conn, struct web100_connection_spec* _spec);

int                web100_socket_data_refresh(web100_agent* _agent);

#define DEF_GAUGE(name, type)\
int web100_get_##name(web100_snapshot* a, void* buf) {\
 static web100_var* va;\
 if (!va)\
   if ((va=web100_var_find(a->group, #name)) == NULL)\
     return -1;\
 return web100_snap_read(va, a, buf);\
}

#define DEF_COUNTER(name, type) DEF_GAUGE(name, type) \
int web100_delta_##name(web100_snapshot* a, web100_snapshot* b, void* buf){\
 static web100_var* va;\
 if (!va)\
   if ((va=web100_var_find(a->group, #name)) == NULL)\
     return -1;\
 return web100_delta_any(va, a, b, buf);\
}

#endif /* _WEB100_H */
