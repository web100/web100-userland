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
 * See http://www-unix.mcs.anl.gov/~gropp/manuals/doctext/doctext.html for
 * documentation format.
 *
 * $Id: web100.c,v 1.25 2002/08/05 19:33:08 jheffner Exp $
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <errno.h>

#include "web100-int.h"
#include "web100.h"


/*
 * Global library errno.  XXX: Not threadsafe (needs to be in thread-local
 * storage).
 */ 
int web100_errno;
char web100_errstr[128];

#ifdef QUIET
char web100_quiet = 1;
#else
char web100_quiet = 0;
#endif

/*
 * Array of error code -> string mappings, in the style of sys_errlist.
 * Must be kept in sync with the defined errors in web100.h.
 */
const char* const web100_sys_errlist[] = {
    "success",                             /* WEB100_ERR_SUCCESS */
    "file read/write error",               /* WEB100_ERR_FILE */
    "unsupported agent type",              /* WEB100_ERR_AGENT_TYPE */
    "no memory",                           /* WEB100_ERR_NOMEM */
    "connection not found",                /* WEB100_ERR_NOCONNECTION */
    "invalid arguments",                   /* WEB100_ERR_INVAL */
    "could not parse " WEB100_HEADER_FILE, /* WEB100_ERR_HEADER */
    "variable not found",                  /* WEB100_ERR_NOVAR */
    "group not found",                     /* WEB100_ERR_NOGROUP */
    "socket operation failed",             /* WEB100_ERR_SOCK */
    "unexpected error due to kernel version mismatch", /* WEB100_ERR_KERNVER */
};

/*
 * Number of web100 errors, in the style of sys_nerr.
 */
int web100_sys_nerr = ARRAYSIZE(web100_sys_errlist);


/*
 * PRIVATE FUNCTIONS
 */

static inline void dep_check(web100_var *var)
{
    if (var->flags & WEB100_VAR_FL_DEP) {
        if (!(var->flags & WEB100_VAR_FL_WARNED) && !web100_quiet)
            fprintf(stderr, "libweb100: warning: accessing depricated variable %s\n", var->name);
        var->flags |= WEB100_VAR_FL_WARNED;
    }
}

/*
 * size_from_type - Returns the size in bytes of an object of the specified
 * type.
 */
static int
size_from_type(WEB100_TYPE type)
{
    switch (type) {
    case WEB100_TYPE_INTEGER:
    case WEB100_TYPE_INTEGER32:
    case WEB100_TYPE_INET_ADDRESS_IPV4:
    case WEB100_TYPE_COUNTER32:
    case WEB100_TYPE_GAUGE32:
    case WEB100_TYPE_UNSIGNED32:
    case WEB100_TYPE_TIME_TICKS:
        return 4;
    case WEB100_TYPE_COUNTER64:
        return 8;
    case WEB100_TYPE_INET_PORT_NUMBER:
        return 2;
    case WEB100_TYPE_INET_ADDRESS:
    case WEB100_TYPE_INET_ADDRESS_IPV6:
        return 17;
    default:
        return 0;
    }
}


/*
 * web100_attach_local - Initializes the provided agent with the information
 * from the local Web100 installation.  Returns NULL and sets web100_errno
 * on failure.
 */ 
static web100_agent*
_web100_agent_attach_header(FILE *header)
{
    web100_agent* agent = NULL;
    int c;
    web100_group* gp;
    web100_var* vp; 
    int fsize;
    char tmpbuf[WEB100_VARNAME_LEN_MAX];
    int have_len = 0;

    if ((agent = malloc(sizeof(web100_agent))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
        goto Cleanup;
    }

    /* agent must be 0-filled to get the correct list adding semantics */
    bzero(agent, sizeof(web100_agent));

    if (fscanf(header, "%[^\n]", agent->version) != 1) {
        web100_errno = WEB100_ERR_HEADER;
        goto Cleanup;
    }
    if (strncmp(agent->version, "1.", 2) != 0)
        have_len = 1;

    /* XXX: Watch out for failure cases, be sure to deallocate memory
     * properly */
    
    gp = NULL; 
    while (!feof(header) && !ferror(header)) {
        while (isspace(c = fgetc(header)))
            ;
        
        if (c < 0) {
            break;
        } else if (c == '/') {
            if ((gp = (web100_group*) malloc(sizeof(web100_group))) == NULL) {
                web100_errno = WEB100_ERR_NOMEM;
                goto Cleanup;
            }
                
            gp->agent = agent;
            
            if (fscanf(header, "%s", gp->name) != 1) {
                web100_errno = WEB100_ERR_HEADER;
                goto Cleanup;
            }
            
            IFDEBUG(printf("_web100_agent_attach_local: new group: %s\n", gp->name));
            
            gp->size = 0;
            gp->nvars = 0;
            
            if (strcmp(gp->name, "spec") == 0) {
                agent->info.local.spec = gp;
            } else {
                gp->info.local.var_head = NULL;
                gp->info.local.next = agent->info.local.group_head;
                agent->info.local.group_head = gp;
            }
        } else {
            ungetc(c, header);
            
            if (gp == NULL) {
                web100_errno = WEB100_ERR_HEADER;
                goto Cleanup;
            }
            
            if ((vp = (web100_var *)malloc(sizeof (web100_var))) == NULL) {
                web100_errno = WEB100_ERR_NOMEM;
                goto Cleanup;
            }

            vp->group = gp;
            
            if (!have_len) {
                if (fscanf(header, "%s%d%d", vp->name, &vp->offset, &vp->type) != 3) {
                    web100_errno = WEB100_ERR_HEADER;
                    goto Cleanup;
                }
                vp->len = -1;
            } else {
                if (fscanf(header, "%s%d%d%d", vp->name, &vp->offset, &vp->type, &vp->len) != 4) {
                    web100_errno = WEB100_ERR_HEADER;
                    goto Cleanup;
                }
            }
            
            /* Depricated variable check */
            vp->flags = 0;
            if (vp->name[0] == '_') {
                vp->flags |= WEB100_VAR_FL_DEP;
                strcpy(tmpbuf, vp->name);      /* Strip off leading _ */
                strcpy(vp->name, tmpbuf + 1);
                IFDEBUG(printf("_web100_agent_attach_local: depricated var: %s\n", vp->name));
            }

            IFDEBUG(printf("_web100_agent_attach_local: new var: %s %d %d\n", vp->name, vp->offset, vp->type));

	    /* increment group (== file) size if necessary */ 
            fsize = vp->offset + size_from_type(vp->type);
            gp->size = ((gp->size < fsize) ? fsize : gp->size); 
            
	    /* if size_from_type 0 (i.e., type unrecognized),
	       forgo adding the variable */
	    if(!size_from_type(vp->type)) {
		free(vp);
		continue;
	    }

            gp->nvars++;
            
            vp->info.local.next = gp->info.local.var_head;
            gp->info.local.var_head = vp;
        }
    }
   
    web100_errno = WEB100_ERR_SUCCESS;
    
 Cleanup:
    if (web100_errno != WEB100_ERR_SUCCESS) {
        web100_detach(agent);
        agent = NULL;
    }
    
    return agent;
}


static web100_agent*
_web100_agent_attach_local(void)
{
    web100_agent* agent = NULL;
    FILE* header = NULL;
    int c;
    web100_group* gp;
    web100_var* vp;

    if ((header = fopen(WEB100_HEADER_FILE, "r")) == NULL) {
        web100_errno = WEB100_ERR_HEADER;
        goto Cleanup;
    }

    if((agent = _web100_agent_attach_header(header)) == NULL)
       	goto Cleanup;

    agent->type = WEB100_AGENT_TYPE_LOCAL;

    web100_errno = WEB100_ERR_SUCCESS;
    
 Cleanup:
    if (header != NULL) {
        fclose(header);
        header = NULL;
    }
    
    if (web100_errno != WEB100_ERR_SUCCESS) { 
        web100_detach(agent);
        agent = NULL;
    }
    
    return agent;
}


static web100_agent*
_web100_agent_attach_log(FILE *header)
{
    web100_agent* agent = NULL;

    if((agent = _web100_agent_attach_header(header)) == NULL) {
	return NULL;
    } 

    agent->type = WEB100_AGENT_TYPE_LOG;

    web100_errno = WEB100_ERR_SUCCESS;
    
    return agent;
}


static int
refresh_connections(web100_agent *agent)
{
    struct dirent *ent;
    DIR *dir;
    web100_connection *cp, *cp2;
    FILE *fp;
    char filename[PATH_MAX];
    web100_group *spec_gp;
    web100_var *var;
    
    cp = agent->info.local.connection_head;
    while (cp) {
        cp2 = cp->info.local.next;
        free(cp);
        cp = cp2;
    }
    agent->info.local.connection_head = NULL;
    
    if ((dir = opendir(WEB100_ROOT_DIR)) == NULL) {
        perror("refresh_connections: opendir");
        return WEB100_ERR_FILE;
    }
    
    while ((ent = readdir(dir))) {
        int cid;
        char *addr_name, *port_name;
        void *dst;
        
        cid = atoi(ent->d_name);
        if (cid == 0 && ent->d_name[0] != '0')
            continue;
        
        if ((cp = (web100_connection *)malloc(sizeof (web100_connection))) == NULL)
            return WEB100_ERR_NOMEM;
        cp->agent = agent;
        cp->cid = cid; 
        cp->logstate = 0; 

        cp->info.local.next = agent->info.local.connection_head;
        agent->info.local.connection_head = cp;
        
        spec_gp = agent->info.local.spec;
        
        if ((var = web100_var_find(spec_gp, "LocalAddressType")) == NULL)
            cp->addrtype = WEB100_ADDRTYPE_IPV4;
        else if (web100_raw_read(var, cp, &cp->addrtype) != WEB100_ERR_SUCCESS)
            return web100_errno;
        
        if (strncmp(agent->version, "1.", 2) == 0) {
            addr_name = "RemoteAddress";
            port_name = "RemotePort";
        } else {
            addr_name = "RemAddress";
            port_name = "RemPort";
        }
        
        if ((var = web100_var_find(spec_gp, "LocalAddress")) == NULL)
            return WEB100_ERR_FILE;
        if (web100_raw_read(var, cp, &cp->spec_v6.src_addr) != WEB100_ERR_SUCCESS)
            return web100_errno;
        
        if ((var = web100_var_find(spec_gp, addr_name)) == NULL)
            return WEB100_ERR_FILE;
        if (web100_raw_read(var, cp, &cp->spec_v6.dst_addr) != WEB100_ERR_SUCCESS)
            return web100_errno;
        
        if (cp->addrtype == WEB100_ADDRTYPE_IPV4) {
            memcpy(&cp->spec.src_addr, &cp->spec_v6.src_addr, 4);
            memcpy(&cp->spec.dst_addr, &cp->spec_v6.dst_addr, 4);
        }
        
        if ((var = web100_var_find(spec_gp, "LocalPort")) == NULL)
            return WEB100_ERR_FILE;
        dst = (cp->addrtype == WEB100_ADDRTYPE_IPV4) ? &cp->spec.src_port : &cp->spec_v6.src_port;
        if (web100_raw_read(var, cp, dst) != WEB100_ERR_SUCCESS)
            return web100_errno;
        
        if ((var = web100_var_find(spec_gp, port_name)) == NULL)
            return WEB100_ERR_FILE;
        dst = (cp->addrtype == WEB100_ADDRTYPE_IPV4) ? &cp->spec.dst_port : &cp->spec_v6.dst_port;
        if (web100_raw_read(var, cp, dst) != WEB100_ERR_SUCCESS)
            return web100_errno;
    }
    
    if (closedir(dir))
        perror("refresh_connections: closedir");
    
    return WEB100_ERR_SUCCESS;
}


/*
 * PUBLIC FUNCTIONS
 */

void
web100_perror(const char* str)
{
  if( strlen(web100_errstr) == 0 )
      fprintf(stderr, "%s: %s\n", str, web100_strerror(web100_errno));
  else {
      fprintf(stderr, "%s: %s - %s\n", str, web100_strerror(web100_errno), web100_errstr);
      strcpy(web100_errstr,"");     // Clear the error string
  }
}


const char*
web100_strerror(int errnum)
{
    if (errnum < 0 || errnum >= web100_sys_nerr)
        return "unknown error";
        
    return web100_sys_errlist[errnum];
}


web100_agent*
web100_attach(int method, void *data)
{
    switch (method) {
    case WEB100_AGENT_TYPE_LOCAL:
        return _web100_agent_attach_local();
    default:
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
}


void
web100_detach(web100_agent *agent)
{
    web100_group *gp, *gp2;
    web100_var *vp, *vp2;
    web100_connection *cp, *cp2;
    
    if (agent == NULL) {
        return;
    }
    
    gp = agent->info.local.group_head;
    while (gp) {
        vp = gp->info.local.var_head;
        while (vp) {
            vp2 = vp->info.local.next;
            free(vp);
            vp = vp2;
        }
        
        gp2 = gp->info.local.next;
        free(gp);
        gp = gp2;
    }
    
    cp = agent->info.local.connection_head;
    while (cp) {
        cp2 = cp->info.local.next;
        free(cp);
        cp = cp2;
    }
    
    free(agent);
}


web100_group*
web100_group_head(web100_agent *agent)
{
    if (!(agent->type & (WEB100_AGENT_TYPE_LOCAL | WEB100_AGENT_TYPE_LOG))) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return agent->info.local.group_head;
}


web100_group*
web100_group_next(web100_group *group)
{
    if (!(group->agent->type & (WEB100_AGENT_TYPE_LOCAL | WEB100_AGENT_TYPE_LOG))) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return group->info.local.next;
}


web100_group*
web100_group_find(web100_agent *agent, const char *name)
{
    web100_group *gp;
    
    if (!(agent->type & (WEB100_AGENT_TYPE_LOCAL | WEB100_AGENT_TYPE_LOG))) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    gp = agent->info.local.group_head;
    while (gp) {
        if (strcmp(gp->name, name) == 0)
            break;
        gp = gp->info.local.next;
    }
    
    web100_errno = (gp == NULL ? WEB100_ERR_NOGROUP : WEB100_ERR_SUCCESS);
    return gp;
}


web100_var*
web100_var_head(web100_group *group)
{
    web100_var *vp;
    
    if (!(group->agent->type & (WEB100_AGENT_TYPE_LOCAL | WEB100_AGENT_TYPE_LOG))) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    vp = group->info.local.var_head;
    while (vp && (vp->flags & WEB100_VAR_FL_DEP))
        vp = vp->info.local.next;
    web100_errno = WEB100_ERR_SUCCESS;
    return vp;
}


web100_var*
web100_var_next(web100_var *var)
{
    web100_var *vp;
    
    if (!(var->group->agent->type & (WEB100_AGENT_TYPE_LOCAL | WEB100_AGENT_TYPE_LOG))) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    vp = var->info.local.next;
    while (vp && (vp->flags & WEB100_VAR_FL_DEP))
        vp = vp->info.local.next;
    web100_errno = WEB100_ERR_SUCCESS;
    return vp;
}


web100_var*
web100_var_find(web100_group *group, const char *name)
{
    web100_var *vp;
    
    if (!(group->agent->type & (WEB100_AGENT_TYPE_LOCAL | WEB100_AGENT_TYPE_LOG))) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    vp = group->info.local.var_head;
    while (vp) {
        if (strcmp(vp->name, name) == 0)
            break;
        vp = vp->info.local.next;
    }

    web100_errno = (vp == NULL ? WEB100_ERR_NOVAR : WEB100_ERR_SUCCESS);
    if (vp)
        dep_check(vp);
    return vp;
}


/*@
web100_group_var_find - Find both group and var for a given variable name in agent
@*/
int
web100_agent_find_var_and_group(web100_agent* agent, const char* name,
                                web100_group** group, web100_var** var)
{
    web100_group* g;
    
    g = web100_group_head(agent);
    while (g) {
        web100_var* v = web100_var_find(g, name);
        if (v) {
            *group = g;
            *var = v;
            dep_check(v);
            return WEB100_ERR_SUCCESS;
        }
        g = web100_group_next(g);
    }

    /* var not found in any of the groups */
    web100_errno = WEB100_ERR_NOVAR;
    return WEB100_ERR_NOVAR;
}


web100_connection*
web100_connection_head(web100_agent *agent)
{
    if (agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    if ((web100_errno = refresh_connections(agent)) != WEB100_ERR_SUCCESS)
        return NULL;
    
    return agent->info.local.connection_head;
}


web100_connection*
web100_connection_next(web100_connection *connection)
{
    if (connection->agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return connection->info.local.next;
}


web100_connection*
web100_connection_find(web100_agent *agent,
                       struct web100_connection_spec *spec)
{
    web100_connection *cp;
    
    if (agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    if ((web100_errno = refresh_connections(agent)) != WEB100_ERR_SUCCESS)
        return NULL;
    
    cp = agent->info.local.connection_head;
    while (cp) {
        if (cp->spec.dst_port == spec->dst_port &&
            cp->spec.dst_addr == spec->dst_addr &&
            cp->spec.src_port == spec->src_port &&
            cp->spec.src_addr == spec->src_addr)
            break;
        cp = cp->info.local.next;
    }
    
    web100_errno = (cp == NULL ? WEB100_ERR_NOCONNECTION : WEB100_ERR_SUCCESS);
    return cp;
}


web100_connection*
web100_connection_find_v6(web100_agent *agent,
                       struct web100_connection_spec_v6 *spec_v6)
{
    web100_connection *cp;
    
    if (agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    if ((web100_errno = refresh_connections(agent)) != WEB100_ERR_SUCCESS)
        return NULL;
    
    cp = agent->info.local.connection_head;
    while (cp) {
        if (memcmp(&cp->spec_v6, spec_v6, sizeof (spec_v6)) != 0)
            break;
        cp = cp->info.local.next;
    }
    
    web100_errno = (cp == NULL ? WEB100_ERR_NOCONNECTION : WEB100_ERR_SUCCESS);
    return cp;
}


web100_connection*
web100_connection_lookup(web100_agent *agent, int cid)
{
    web100_connection *cp;
    
    if (!agent) {
        web100_errno = WEB100_ERR_INVAL;
        return NULL;
    }

    if (agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    if ((web100_errno = refresh_connections(agent)) != WEB100_ERR_SUCCESS)
        return NULL;
    
    cp = agent->info.local.connection_head;
    while (cp) {
        if (cp->cid == cid)
            break;
        cp = cp->info.local.next;
    }

    web100_errno = (cp == NULL ? WEB100_ERR_NOCONNECTION : WEB100_ERR_SUCCESS);
    return cp;
}


web100_connection*
web100_connection_from_socket(web100_agent *agent, int sockfd)
{
    struct sockaddr_in ne, fe; /* near and far ends */
    socklen_t namelen; /* may not be POSIX */
    struct web100_connection_spec spec; /* connection tuple */

    /* XXX TODO XXX: Should we only allow local agents? */

    namelen = sizeof(fe);
    if (getpeername(sockfd, (struct sockaddr*) &fe, &namelen) != 0) {
        web100_errno = WEB100_ERR_SOCK;
        return NULL;
    }

    namelen = sizeof(ne);
    if (getsockname(sockfd, (struct sockaddr*) &ne, &namelen) != 0) {
        web100_errno = WEB100_ERR_SOCK;
        return NULL;
    }

    spec.src_addr = ne.sin_addr.s_addr;
    spec.src_port = ntohs(ne.sin_port);
    spec.dst_addr = fe.sin_addr.s_addr;
    spec.dst_port = ntohs(fe.sin_port);

    return web100_connection_find(agent, &spec);
}


int
web100_connection_data_copy(web100_connection *dest, web100_connection *src)
{ 
    if (!dest || !src) {
	web100_errno = WEB100_ERR_INVAL;
        return -WEB100_ERR_INVAL;
    }

    dest->agent = src->agent;
    dest->cid = src->cid;
    memcpy(&dest->spec, &src->spec, sizeof(struct web100_connection_spec)); 
    return WEB100_ERR_SUCCESS;
}

web100_connection*
web100_connection_new_local_copy(web100_connection *src)
{
    web100_connection *conn;

    if (!src) {
	web100_errno = WEB100_ERR_INVAL;
	return NULL;
    }

    if ((conn = malloc(sizeof (web100_connection))) == NULL ) {
       	web100_errno = WEB100_ERR_NOMEM;
       	return NULL;
    }
    conn->agent = src->agent;
    conn->cid = src->cid;
    memcpy(&conn->spec, &src->spec, sizeof(struct web100_connection_spec));

    return conn;
}

void
web100_connection_free_local_copy(web100_connection *conn)
{
    if (!conn) {
	web100_errno = WEB100_ERR_INVAL;
	return;
    }
    free(conn);
}

/*@
web100_snapshot_alloc - allocate a snapshot
@*/
web100_snapshot*
web100_snapshot_alloc(web100_group *group, web100_connection *conn)
{
    web100_snapshot *snap;
    
    if (group->agent != conn->agent) {
        web100_errno = WEB100_ERR_INVAL;
        return NULL;
    }
    
    if ((snap = (web100_snapshot *)malloc(sizeof (web100_snapshot))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
        return NULL;
    }
    
    if ((snap->data = (void *)malloc(group->size)) == NULL) {
        free(snap);
        web100_errno = WEB100_ERR_NOMEM;
        return NULL;
    }
    
    snap->group = group;
    snap->connection = conn;
    
    return snap;
}


/*@
web100_snapshot_alloc_from_log - allocate a snapshot based on logged info
@*/
web100_snapshot*
web100_snapshot_alloc_from_log(web100_log *log)
{
    web100_snapshot *snap;
    
    if (log->group->agent != log->connection->agent) {
        web100_errno = WEB100_ERR_INVAL;
        return NULL;
    }
    
    if ((snap = (web100_snapshot *)malloc(sizeof (web100_snapshot))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
        return NULL;
    }
    
    if ((snap->data = (void *)malloc(log->group->size)) == NULL) {
        free(snap);
        web100_errno = WEB100_ERR_NOMEM;
        return NULL;
    }
    
    snap->group = log->group;
    snap->connection = log->connection;
    
    return snap;
}


/*@
web100_snapshot_free - deallocate a snapshot
@*/
void
web100_snapshot_free(web100_snapshot *snap)
{
    free(snap->data);
    free(snap);
}


/*@
web100_snap - take a snapshot
@*/
int
web100_snap(web100_snapshot *snap)
{
    FILE *fp;
    char filename[PATH_MAX];
    
    if (snap->group->agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return -WEB100_ERR_AGENT_TYPE;
    }
    
    sprintf(filename, "%s/%d/%s", WEB100_ROOT_DIR, snap->connection->cid, snap->group->name);
    if ((fp = fopen(filename, "r")) == NULL) {
        web100_errno = WEB100_ERR_NOCONNECTION;
        return -WEB100_ERR_NOCONNECTION;
    }
    
    if (fread(snap->data, snap->group->size, 1, fp) != 1){
        web100_errno = WEB100_ERR_NOCONNECTION;
        return -WEB100_ERR_NOCONNECTION;
    }

    if (fclose(fp)) {
       	web100_errno = WEB100_ERR_FILE;
       	return -WEB100_ERR_FILE;
    }
    
    return WEB100_ERR_SUCCESS;
}


/*@
web100_raw_read - read a variable from a connection into a buffer
@*/
int
web100_raw_read(web100_var *var, web100_connection *conn, void *buf)
{
    FILE *fp;
    char filename[PATH_MAX];
    
    if (var->group->agent != conn->agent) {
        web100_errno = WEB100_ERR_INVAL;
        return -WEB100_ERR_INVAL;
    }
    
    if (conn->agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return -WEB100_ERR_AGENT_TYPE;
    }
    
    sprintf(filename, "%s/%d/%s", WEB100_ROOT_DIR, conn->cid, var->group->name);
    if ((fp = fopen(filename, "r")) == NULL) {
        web100_errno = WEB100_ERR_NOCONNECTION;
        return -WEB100_ERR_NOCONNECTION;
    }
    
    if (fseek(fp, var->offset, SEEK_SET)) {
        perror("web100_raw_read: fseek");
        web100_errno = WEB100_ERR_FILE;
        return -WEB100_ERR_FILE;
    }
    if (fread(buf, size_from_type(var->type), 1, fp) != 1) {
        perror("web100_raw_read: fread");
        web100_errno = WEB100_ERR_FILE;
        return -WEB100_ERR_FILE;
    }
    
    if (fclose(fp))
        perror("web100_raw_read: fclose");
    
    return WEB100_ERR_SUCCESS;
}


/*@
web100_raw_write - write a variable into a connection from a buffer
@*/
int
web100_raw_write(web100_var *var, web100_connection *conn, void *buf)
{
    FILE *fp;
    char filename[PATH_MAX];
    
    if (var->group->agent != conn->agent) {
        web100_errno = WEB100_ERR_INVAL;
        return -WEB100_ERR_INVAL;
    }
    
    if (conn->agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return -WEB100_ERR_AGENT_TYPE;
    }
    
    sprintf(filename, "%s/%d/%s", WEB100_ROOT_DIR, conn->cid, var->group->name);
    if ((fp = fopen(filename, "w")) == NULL) {
        web100_errno = WEB100_ERR_NOCONNECTION;
        return -WEB100_ERR_NOCONNECTION;
    }
    
    if (fseek(fp, var->offset, SEEK_SET)) {
        perror("web100_raw_write: fseek");
        web100_errno = WEB100_ERR_FILE;
        return -WEB100_ERR_FILE;
    }
    if (fwrite(buf, size_from_type(var->type), 1, fp) != 1) {
        perror("web100_raw_write: fread");
        web100_errno = WEB100_ERR_FILE;
        return -WEB100_ERR_FILE;
    }
    
    if (fclose(fp))
        perror("web100_raw_read: fclose");
    
    return WEB100_ERR_SUCCESS;
}


/*@
web100_snap_read - read a variable from a snapshot into a buffer
@*/
int
web100_snap_read(web100_var *var, web100_snapshot *snap, void *buf)
{
    if (var->group != snap->group) {
        web100_errno = WEB100_ERR_INVAL;
        return -WEB100_ERR_INVAL;
    }
    
    memcpy(buf, (void *)((unsigned long)(snap->data) + var->offset),
           size_from_type(var->type));
    
    return WEB100_ERR_SUCCESS;
}


/*@
web100_delta_any - produce the delta of a variable between two snapshots
@*/
int
web100_delta_any(web100_var *var, web100_snapshot *s1,
                 web100_snapshot *s2, void *buf)
{
    unsigned long long int v1, v2, val;

    if (s1->group != s2->group) {
        web100_errno = WEB100_ERR_INVAL;
        return -WEB100_ERR_INVAL;
    } 

    if ((web100_snap_read(var, s1, &v1) < 0) ||
        (web100_snap_read(var, s2, &v2) < 0))
        return -web100_errno; 

    val = v1 - v2;

    memcpy(buf, &val, size_from_type(var->type));

    return WEB100_ERR_SUCCESS;
}


/*@
web100_snap_data_copy - copy the data from one snapshot to another
@*/
int
web100_snap_data_copy(web100_snapshot *dest, web100_snapshot *src)
{
    if (dest->connection != src->connection) {
        web100_errno = WEB100_ERR_INVAL;
        return -WEB100_ERR_INVAL;
    }
    if (dest->group != src->group) {
        web100_errno = WEB100_ERR_INVAL;
        return -WEB100_ERR_INVAL;
    }

    memcpy(dest->data, src->data, src->group->size);

    return WEB100_ERR_SUCCESS;
}


/*@
web100_value_to_text - return string representation of buf
@*/
char*
web100_value_to_text(WEB100_TYPE type, void* buf)
{
    static char text[WEB100_VALUE_LEN_MAX];	/* XXX This is not threadsafe either */

    if (web100_value_to_textn(text, WEB100_VALUE_LEN_MAX, type, buf) == -1)
        return NULL;

    return text;
}


/*@
web100_value_to_textn - return string representation of buf
@*/
int
web100_value_to_textn(char* dest, size_t size, WEB100_TYPE type, void* buf)
{
    if (type == WEB100_TYPE_INET_ADDRESS)
        type = ((char *)buf)[16] == WEB100_ADDRTYPE_IPV4 ?
               WEB100_TYPE_INET_ADDRESS_IPV4 :
               WEB100_TYPE_INET_ADDRESS_IPV6;
    
    switch(type) {
    case WEB100_TYPE_INET_ADDRESS_IPV4:
    {
        unsigned char *addr = (unsigned char *) buf; 
        return snprintf(dest, size, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
    }
    case WEB100_TYPE_INTEGER:
    case WEB100_TYPE_INTEGER32:
    case WEB100_TYPE_COUNTER32: 
    case WEB100_TYPE_GAUGE32: 
    case WEB100_TYPE_UNSIGNED32:
    case WEB100_TYPE_TIME_TICKS:
        return snprintf(dest, size, "%u", *(u_int32_t *) buf);
    case WEB100_TYPE_COUNTER64:
        return snprintf(dest, size, "%llu", *(u_int64_t *) buf);
    case WEB100_TYPE_INET_PORT_NUMBER:
        return snprintf(dest, size, "%u", *(u_int16_t *) buf);
    case WEB100_TYPE_INET_ADDRESS_IPV6:
    {
        short *addr = (short *)buf;
        int start = -1, end = -1;
        int i, j;
        int pos;
        
        /* Find longest subsequence of 0's in addr */
        for (i = 0; i < 8; i++) {
            if (addr[i] == 0) {
                for (j = i + 1; addr[j] == 0 && j < 8; j++)
                    ;
                if (j - i > end - start) {
                    end = j;
                    start = i;
                }
                i = j;
            }
        }
        if (end - start == 1)
            start = -1;
        
        pos = 0;
        for (i = 0; i < 8; i++) {
            if (i > 0)
                pos += snprintf(dest + pos, size - pos, ":");
                if (pos >= size)
                    break;
            if (i == start) {
                pos += snprintf(dest + pos, size - pos, ":");
                i += end - start - 1;
            } else {
                pos += snprintf(dest + pos, size - pos, "%hx", ntohs(addr[i]));
            }
            if (pos >= size)
                break;
        }
        
        if (pos > size)
        	pos = size;
        return pos;
    }
    default:
        return snprintf(dest, size, "%s", "unknown type");
    }

    /* never reached */
}


/*@
web100_get_agent_type - return the type of an agent
@*/
int
web100_get_agent_type(web100_agent *agent)
{
    return agent->type;
}


/*@
web100_get_agent_version - return the version of an agent
@*/
const char*
web100_get_agent_version(web100_agent *agent)
{
    return (const char *)(agent->version);
}


/*@
web100_get_group_name - return the name from a group
@*/
const char*
web100_get_group_name(web100_group *group)
{
    return (const char *)(group->name);
}


/*@
web100_get_group_size - get the size of a group
@*/
int
web100_get_group_size(web100_group *group)
{
    return group->size;
}


/*@
web100_get_group_nvars - get the number of variables in a group
@*/
int
web100_get_group_nvars(web100_group *group)
{
    return group->nvars;
}


/*@
web100_get_var_name - get the name of a variable
@*/
const char*
web100_get_var_name(web100_var *var)
{
    return (const char *)(var->name);
}


/*@
web100_get_var_type - get the type of a variable
@*/
int
web100_get_var_type(web100_var *var)
{
    return var->type;
}

/*@
web100_get_var_size - get the size of a variable
@*/
size_t
web100_get_var_size(web100_var *var)
{
    return var->len;
}

web100_group*
web100_get_snap_group(web100_snapshot *snap)
{
    return snap->group;
}

/*@
web100_get_snap_group_name - get the name of the group from a snapshot
@*/
const char*
web100_get_snap_group_name(web100_snapshot *snap)
{
    return (const char *)(snap->group->name);
}


/*@
web100_get_connection_cid - get the connection id from a connection
@*/
int
web100_get_connection_cid(web100_connection *connection)
{
    return connection->cid;
}


/*@
web100_get_connection_spec - get the connection spec from a connection
@*/
void
web100_get_connection_spec(web100_connection *connection,
                           struct web100_connection_spec *spec)
{
    memcpy(spec, &connection->spec, sizeof (struct web100_connection_spec));
}

static int
web100_connection_info_refresh(web100_agent *agent, struct web100_connection_info **conninfo)
{ 
    struct tmp_connection_info {
      int cid;
      struct web100_connection_spec spec;
      ino_t ino;
      pid_t pid;
      uid_t uid;
      int state;
      char cmdline[PATH_MAX];
      struct tmp_connection_info *next;
    } *cid_data, *tcp_data, *fd_data, *tmp, *tcp_head, *fd_head;

    struct web100_connection_info *ci;

    web100_connection *cp;
    char buf[256], path[PATH_MAX];
    FILE *file; 
    int scan;
    DIR *dir, *fddir;
    struct dirent *direntp, *fddirentp;
    int fd;
    struct stat st;
    int stno;
    pid_t pid;
    int ii=0;
    int tcp_entry, fd_entry;
   
    // associate cid with IP
    if(refresh_connections(agent))
	return web100_errno;

    cid_data = NULL;
    cp = agent->info.local.connection_head;
    while (cp) {
        if((tmp = malloc(sizeof (struct tmp_connection_info))) == NULL) {
	    web100_errno = WEB100_ERR_NOMEM;
	    return -WEB100_ERR_NOMEM;
	}
        web100_get_connection_spec(cp, &(tmp->spec));
        tmp->cid = cp->cid;

        tmp->next = cid_data;
        cid_data = tmp;

        cp = cp->info.local.next; 
    } 
    
    // associate IP with ino
    if((file = fopen("/proc/net/tcp", "r")) == NULL) {
        web100_errno = WEB100_ERR_FILE;
	return -WEB100_ERR_FILE;
    }

    tcp_data = NULL;
    while(fgets(buf, sizeof(buf), file) != NULL) { 
        if((tmp = malloc(sizeof (struct tmp_connection_info))) == NULL) {
	    web100_errno = WEB100_ERR_NOMEM;
	    return -WEB100_ERR_NOMEM;
	}

        if((scan = sscanf(buf, "%*u: %x:%x %x:%x %x %*x:%*x %*x:%*x %*x %u %*u %u",
                (u_int32_t *) &(tmp->spec.src_addr),
                (u_int16_t *) &(tmp->spec.src_port),
                (u_int32_t *) &(tmp->spec.dst_addr),
                (u_int16_t *) &(tmp->spec.dst_port),
                (u_int *) &(tmp->state),
                (u_int *) &(tmp->uid),
                (u_int *) &(tmp->ino))) == 7) { 
            tmp->next = tcp_data; 
            tcp_data = tmp; 
       	} else {
            free(tmp);
        }
    }
    tcp_head = tcp_data;
    fclose(file); 

    // associate ino with pid
    if(!(dir = opendir("/proc"))) { 
	web100_errno = WEB100_ERR_FILE;
       	return -WEB100_ERR_FILE;
    }

    fd_data = NULL;
    while((direntp = readdir(dir)) != NULL) {
        if((pid = atoi(direntp->d_name)) != 0)
       	{
	    sprintf(path, "%s/%d/%s/", "/proc", pid, "fd"); 
	    if(fddir = opendir(path)) //else lacks permissions 
		while((fddirentp = readdir(fddir)) != NULL) 
		{ 
		    strcpy(buf, path);
		    strcat(buf, fddirentp->d_name); 
		    stno = stat(buf, &st); 
		    if(S_ISSOCK(st.st_mode)) // add new list entry
		    { 
			if((tmp = malloc(sizeof(struct tmp_connection_info))) == NULL){
			    web100_errno = WEB100_ERR_NOMEM;
			    return -WEB100_ERR_NOMEM;
		       	}

			tmp->ino = st.st_ino;
		       	tmp->pid = pid; 

		       	tmp->next = fd_data; 
		       	fd_data = tmp; 

			sprintf(buf, "/proc/%d/status", pid);

			if((file = fopen(buf, "r")) == NULL) continue;

			if(fgets(buf, sizeof(buf), file) == NULL){
			    fclose(file);
			    continue;
		       	} 

			if(sscanf(buf, "Name: %s\n", &(fd_data->cmdline)) != 1){
			    fclose(file);
			    continue;
		       	} 
			fclose(file); 
		    }
	       	}
	    closedir(fddir);
       	} 

    }
    fd_head = fd_data;
    closedir(dir); 

    //finally, collate above information

    *conninfo = NULL;
    while(cid_data) {
	tcp_entry = 0;
	for(tcp_data = tcp_head; tcp_data; tcp_data = tcp_data->next) {
	    if(tcp_data->spec.dst_port == cid_data->spec.dst_port &&
               tcp_data->spec.dst_addr == cid_data->spec.dst_addr &&
               tcp_data->spec.src_port == cid_data->spec.src_port) { 

		tcp_entry = 1;
		fd_entry = 0;
		for(fd_data = fd_head; fd_data; fd_data = fd_data->next) {
		    if(fd_data->ino == tcp_data->ino) {//then create entry 
			fd_entry = 1;

		       	ci = (struct web100_connection_info *) malloc(sizeof (struct web100_connection_info));
			ci->pid = fd_data->pid;
			strncpy(ci->cmdline, fd_data->cmdline, PATH_MAX); 
			ci->uid = tcp_data->uid;
		       	ci->state = tcp_data->state; 

			ci->cid = cid_data->cid;
		       	memcpy(&(ci->spec), &cid_data->spec, sizeof (struct web100_connection_spec));
			ci->next = *conninfo;
			*conninfo = ci;
		    }
		}
		if(!fd_entry) { // add entry w/out cmdline 
		    ci = (struct web100_connection_info *) malloc(sizeof (struct web100_connection_info));
		    ci->pid = 0;
		    strcpy(ci->cmdline, "");
		    ci->uid = tcp_data->uid;
		    ci->state = tcp_data->state;

		    ci->cid = cid_data->cid;
		    memcpy(&(ci->spec), &cid_data->spec, sizeof (struct web100_connection_spec));
		    ci->next = *conninfo;
		    *conninfo = ci;
		}
	    } 
	} 
	if(!tcp_entry) { // then connection has vanished; add residual cid info
                         // (only for consistency with entries in /proc/web100) 
	    ci = (struct web100_connection_info *) malloc(sizeof (struct web100_connection_info));
	    ci->cid = cid_data->cid;
	    memcpy(&(ci->spec), &cid_data->spec, sizeof (struct web100_connection_spec));
	    ci->next = *conninfo;
	    *conninfo = ci;
	}
	cid_data = cid_data->next;
    }

    return WEB100_ERR_SUCCESS;
}

int
web100_get_connection_info(web100_agent *agent, const struct web100_connection_info *hints, struct web100_connection_info **res)
{
    struct web100_connection_info *conninfo, *tmp, *tmp2, *prev;

    static int info_compar(const struct web100_connection_info *aa, const struct web100_connection_info *bb) {
	if(aa->cid) if(aa->cid != bb->cid)
	    return -1; 

	if(aa->spec.dst_port) if(aa->spec.dst_port != bb->spec.dst_port)
	    return -1;
       	if(aa->spec.dst_addr) if(aa->spec.dst_addr != bb->spec.dst_addr)
	    return -1;
	if(aa->spec.src_port) if(aa->spec.src_port != bb->spec.src_port)
	    return -1;
       	if(aa->spec.src_addr) if(aa->spec.src_addr != bb->spec.src_addr)
	    return -1;

	if(aa->pid) if(aa->pid != bb->pid)
	    return -1;
	if(aa->uid) if(aa->uid != bb->uid)
	    return -1;
	if(aa->state) if(aa->state != bb->state)
	    return -1;
	if(aa->cmdline) if(strncmp(aa->cmdline, bb->cmdline, strlen(aa->cmdline)))
	    return -1;
	return 0;
    }

    if(web100_connection_info_refresh(agent, &conninfo)) {
	*res = NULL;
	return web100_errno;
    }

    if(hints) {
	tmp = conninfo;
	prev = NULL;
       	while(tmp) { 
	    if(info_compar(hints, tmp)) {
		if(prev) prev->next = tmp->next;
		else conninfo = conninfo->next;
		tmp2 = tmp;
		tmp = tmp->next;
		free(tmp2);
	    }
	    else {
	       	prev = tmp;
	       	tmp = tmp->next;
	    }
       	} 
    }
    *res = conninfo;
    return WEB100_ERR_SUCCESS;
}

void
web100_free_connection_info(struct web100_connection_info *res)
{
    struct web100_connection_info *tmp;

    while(res) {
	tmp = res;
	res = res->next;
	free(tmp);
    }
}
#if 0
int
web100_diagnose_start(web100_connection *conn)
{
    char logname[PATH_MAX];
    time_t timep;
    struct tm *tmp;
    pid_t pid;
    char tracescript[PATH_MAX];
    char buf[32], buf1[32], buf2[32];

    if(!conn) {
	web100_errno = WEB100_ERR_INVAL;
	goto Cleanup;
    }

    conn->logfile = NULL;
    conn->tracepid = 0;

    timep = time(NULL);
    tmp = localtime(&timep);
    sprintf(logname, "%s.%d_%d.%d.%d.%d.%d", "./web100.log", conn->cid, tmp->tm_mon, tmp->tm_mday, tmp->tm_hour, tmp->tm_min,  tmp->tm_sec);

    if((conn->logfile = fopen(logname, "w")) == NULL) {
	web100_errno = WEB100_ERR_FILE;
	strcpy(web100_errstr, "fopen: "); 
	goto Cleanup;
    }

    pid = fork();
    if(pid == -1) {
       	web100_errno = WEB100_ERR_NOMEM;
       	goto Cleanup;	
    }
    else if(pid == 0) {
	if(strcpy(tracescript, getenv("TRACESCRIPT"))) { 
	    if(execlp(tracescript, tracescript, web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &conn->spec.src_port), web100_value_to_text(WEB100_TYPE_IP_ADDRESS, &conn->spec.dst_addr), web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, &conn->spec.dst_port), 0) == -1) {
		web100_errno = WEB100_ERR_FILE;
		strcpy(web100_errstr, "execlp: ");
		goto Cleanup;
	    }
	}
    }
    else {
	waitpid (-1, 0, WNOHANG);
	conn->tracepid = pid;
    }

    web100_errno = WEB100_ERR_SUCCESS;

Cleanup:
    if(web100_errno != WEB100_ERR_SUCCESS) { 
	strcat(web100_errstr, "web100_diagnose_start"); 
	return -web100_errno;
    } 

    conn->logstate = TRUE;
    return WEB100_ERR_SUCCESS;
}
#endif
int
web100_diagnose_stop(web100_connection *conn)
{
    conn->logstate = FALSE;
    if(conn->logfile) {
	fclose(conn->logfile);
	conn->logfile = NULL;
    }
    if(conn->tracepid) kill(conn->tracepid, SIGTERM);
    return WEB100_ERR_SUCCESS;
}

int
web100_diagnose_define(char *script)
{
    setenv("TRACESCRIPT", script, 1);
    return WEB100_ERR_SUCCESS;
}

/* Logging functionality begins here */

#define END_OF_HEADER_MARKER "----End-Of-Header---- -1 -1"
#define BEGIN_SNAP_DATA      "----Begin-Snap-Data----"
#define MAX_TMP_BUF_SIZE    80
#define WEB100_LOG_CID      -1       /* A dummy CID  */

web100_log*
web100_log_open_write(char *logname, web100_connection *conn,
		      web100_group *group)
{
    FILE      *header;
    int       c; 
    time_t    timep;

    web100_log *log = NULL;

    if (group->agent != conn->agent) {
       	web100_errno = WEB100_ERR_INVAL;
	goto Cleanup; 
    } 

    if ((log = (web100_log *)malloc(sizeof (web100_log))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM; 
	goto Cleanup;
    }

    if ((header = fopen(WEB100_HEADER_FILE, "r")) == NULL) {
        web100_errno = WEB100_ERR_HEADER; 
        goto Cleanup;
    }

    log->group       = group; 
    log->connection  = conn;

    if((log->fp = fopen(logname, "w")) == NULL) {
	web100_errno = WEB100_ERR_FILE; 
	goto Cleanup;
    }

    while ((c=fgetc(header)) != EOF){
      if(fputc(c, log->fp) != c){
	web100_errno = WEB100_ERR_FILE; 
	goto Cleanup;
      }
    }
    fputc('\0', log->fp);

    if(fclose(header)) { 
	web100_errno = WEB100_ERR_FILE; 
	goto Cleanup;
    } 
    //
    // Put an end of HEADER marker
    //
    fprintf(log->fp, "%s\n", END_OF_HEADER_MARKER);
    //
    // Put Date and Time
    //
    log->time = time(NULL);

    if(fwrite(&log->time, sizeof(time_t), 1, log->fp) != 1) {
	web100_errno = WEB100_ERR_FILE;
	goto Cleanup;
    } 
    //
    // Put in group name
    // 
    if(fwrite(group->name, WEB100_GROUPNAME_LEN_MAX, 1, log->fp) != 1) {
       	web100_errno = WEB100_ERR_FILE;
       	goto Cleanup;
    }
    //
    // Put in connection spec
    //
    if(fwrite(&(conn->spec), sizeof(struct web100_connection_spec), 1, log->fp) != 1) {
	web100_errno = WEB100_ERR_FILE;
       	goto Cleanup;
    }

    web100_errno = WEB100_ERR_SUCCESS;

Cleanup:
    if(web100_errno != WEB100_ERR_SUCCESS) { 
	if(log) {
	    if(log->fp)
	       	fclose(log->fp); 
	    free(log);
       	}
       	return NULL;
    } 

    return log;
}

int
web100_log_close_write(web100_log *log) 
{ 
    if(fclose(log->fp) != 0) { 
	web100_errno = WEB100_ERR_FILE; 
	return -WEB100_ERR_FILE;
    } 

    free(log);
    return WEB100_ERR_SUCCESS;
}

int
web100_log_write(web100_log *log, web100_snapshot *snap)
{ 
    if(log->fp == NULL) {
	web100_errno = WEB100_ERR_FILE; 
	return -WEB100_ERR_FILE;
    }

    if(log->group != snap->group) {
	web100_errno = WEB100_ERR_INVAL; 
	return -WEB100_ERR_INVAL;
    }

    if(log->connection->spec.dst_port != snap->connection->spec.dst_port ||
       log->connection->spec.dst_addr != snap->connection->spec.dst_addr ||
       log->connection->spec.src_port != snap->connection->spec.src_port) {

	web100_errno = WEB100_ERR_INVAL; 
	return -WEB100_ERR_INVAL;
    }

    fprintf(log->fp, "%s\n", BEGIN_SNAP_DATA);

    if(fwrite(snap->data, snap->group->size, 1, log->fp) != 1) {
	web100_errno = WEB100_ERR_FILE;
	return -WEB100_ERR_FILE;
    }

    return WEB100_ERR_SUCCESS;
}

web100_log*
web100_log_open_read(char *logname)
{
    int           c; 
    char      	  tmpbuf[MAX_TMP_BUF_SIZE];
    struct tm     *tmp;
    char          group_name[WEB100_GROUPNAME_LEN_MAX];
    web100_agent       *agent = NULL;
    web100_connection  *cp = NULL; 
    FILE               *header = NULL;
    
    web100_log *log = NULL;

    if ((log = (web100_log *)malloc(sizeof (web100_log))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM; 
	goto Cleanup;
    }

    if ((log->fp = fopen(logname, "r")) == NULL) {
        web100_errno  = WEB100_ERR_FILE;
        goto Cleanup;
    }

    if ((header = fopen("./log_header", "w+")) == NULL) {
	web100_errno = WEB100_ERR_FILE;
	goto Cleanup; 
    }

    while ((c = fgetc(log->fp)) != '\0') {
       	fputc(c, header);
    }

    rewind(header);

    agent = _web100_agent_attach_log(header);

    if (fgets(tmpbuf, MAX_TMP_BUF_SIZE, log->fp) == NULL ) {
       	web100_errno = WEB100_ERR_HEADER;
       	goto Cleanup;
    }

    if (strncmp(tmpbuf, END_OF_HEADER_MARKER, strlen(END_OF_HEADER_MARKER)) != 0 ) { 
	web100_errno = WEB100_ERR_FILE;
       	goto Cleanup;
    }

    if(fread(&log->time, sizeof(time_t), 1, log->fp) != 1) {
       	web100_errno = WEB100_ERR_FILE;
       	goto Cleanup;
    }

    if(fread(group_name, WEB100_GROUPNAME_LEN_MAX, 1, log->fp) != 1) {
	web100_errno = WEB100_ERR_FILE;
       	goto Cleanup;
    }

    //
    // Define (dummy) connection with logged spec
    //
    if ((cp = (web100_connection *)malloc(sizeof (web100_connection))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
	goto Cleanup;
    }

    cp->agent    = agent;
    cp->cid      = WEB100_LOG_CID; //dummy
    if(fread(&(cp->spec), sizeof(struct web100_connection_spec), 1, log->fp) != 1) {
	web100_errno = WEB100_ERR_FILE;
       	goto Cleanup;
    }

    cp->info.local.next = NULL;
    agent->info.local.connection_head = cp;

    log->agent = agent;
    log->group = web100_group_find(agent, group_name);
    log->connection = cp;

    web100_errno = WEB100_ERR_SUCCESS;

 Cleanup:

    if (header) fclose(header);
    remove("./log_header"); 

    if (web100_errno != WEB100_ERR_SUCCESS) {
       	if (log) {
	    if (log->fp)
	       	fclose(log->fp);
	    free(log); 
	} 
	if(agent) web100_detach(agent);
       	if(cp) free(cp); 

	return NULL;
    }
    
    return log;
}

int
web100_log_close_read(web100_log *log)
{
    if(log) {
       	if(fclose(log->fp) != 0) { 
	    web100_errno = WEB100_ERR_FILE; 
	    return -WEB100_ERR_FILE;
       	} 
	web100_detach(log->agent);
       	free(log);
    }

    return WEB100_ERR_SUCCESS;
}

int
web100_snap_from_log(web100_snapshot* snap, web100_log *log)
{
    int c, what;
    char tmpbuf[MAX_TMP_BUF_SIZE];

    if (snap->group->agent->type != WEB100_AGENT_TYPE_LOG) {
       	web100_errno = WEB100_ERR_AGENT_TYPE; 
	return -WEB100_ERR_AGENT_TYPE;
    }

    if (log->fp == NULL) {
	web100_errno = WEB100_ERR_FILE; 
	return -WEB100_ERR_FILE; 
    }        

    if(fscanf(log->fp, "%s[^\n]", tmpbuf) == EOF) {
	return EOF;
    }
    while( (fgetc(log->fp)) != '\n' )
        ;    // Cleanup the line

    if( strcmp(tmpbuf,BEGIN_SNAP_DATA) != 0 ){
        web100_errno = WEB100_ERR_FILE; 
        return -WEB100_ERR_FILE; 
    }        

    if(fread(snap->data, snap->group->size, 1, log->fp) != 1) {
	web100_errno = WEB100_ERR_FILE; 
	return -WEB100_ERR_FILE; 
    }

    return WEB100_ERR_SUCCESS;
}

web100_agent*
web100_get_log_agent(web100_log *log)
{
    return log->agent;
}

web100_group*
web100_get_log_group(web100_log *log)
{
    return log->group;
}

web100_connection*
web100_get_log_connection(web100_log *log)
{
    return log->connection;
}

time_t
web100_get_log_time(web100_log *log)
{
    return log->time;
}

int
web100_log_eof(web100_log* log)
{
    return feof(log->fp);
}
