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
 * $Id: web100.c,v 1.18 2002/04/16 02:17:24 jestabro Exp $
 */

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
};

/*
 * Number of web100 errors, in the style of sys_nerr.
 */
int web100_sys_nerr = ARRAYSIZE(web100_sys_errlist);


/*
 * PRIVATE FUNCTIONS
 */


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
    case WEB100_TYPE_IP_ADDRESS:
    case WEB100_TYPE_COUNTER32:
    case WEB100_TYPE_GAUGE32:
    case WEB100_TYPE_UNSIGNED32:
    case WEB100_TYPE_TIME_TICKS:
        return 4;
    case WEB100_TYPE_COUNTER64:
        return 8;
    case WEB100_TYPE_UNSIGNED16:
        return 2;
    default:
        assert(FALSE);
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
//    FILE* header = NULL;
    int c;
    web100_group* gp;
    web100_var* vp;
    int discard;

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

    /* XXX: Watch out for failure cases, be sure to deallocate memory
     * properly */
    
    gp = NULL; 
    while (!feof(header) && !ferror(header)) {
        while (isspace(c = fgetc(header)))
            ;
        
        if (c < 0) {
            break;
        } else if (c == '/') {
            if (gp && discard)
                free(gp);
            
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
                discard = 1;
            } else {
                discard = 0;
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
            
            if (fscanf(header, "%s%d%d", vp->name, &vp->offset, &vp->type) != 3) {
                web100_errno = WEB100_ERR_HEADER;
                goto Cleanup;
            }

            IFDEBUG(printf("_web100_agent_attach_local: new var: %s %d %d\n", vp->name, vp->offset, vp->type));
            
            gp->size += size_from_type(vp->type);
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
    int discard;

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
        
        strcpy(filename, WEB100_ROOT_DIR);
        strcat(filename, ent->d_name);
        strcat(filename, "/spec");
        if ((fp = fopen(filename, "r")) == NULL) {
            perror("refresh_connections: fopen");
            return WEB100_ERR_FILE;
        }
        /* This is dangerous.  Should at least check that it
         * actually matches header on attach. */
        if (fread(&cp->spec.dst_port, 2, 1, fp) != 1
            || fread(&cp->spec.dst_addr, 4, 1, fp) != 1
            || fread(&cp->spec.src_port, 2, 1, fp) != 1
            || fread(&cp->spec.src_addr, 4, 1, fp) != 1) {
            perror("fread");
            fprintf(stderr, "refresh_connections: bad spec file format\n");
            return WEB100_ERR_FILE;
        }
        if (fclose(fp))
            perror("refresh_connections: fclose");
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
    if (!(group->agent->type & (WEB100_AGENT_TYPE_LOCAL | WEB100_AGENT_TYPE_LOG))) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return group->info.local.var_head;
}


web100_var*
web100_var_next(web100_var *var)
{
    if (!(var->group->agent->type & (WEB100_AGENT_TYPE_LOCAL | WEB100_AGENT_TYPE_LOG))) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return var->info.local.next;
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
    
    if (fread(snap->data, snap->group->size, 1, fp) != 1) {
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
    static char text[WEB100_VALUE_LEN_MAX];

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
    switch(type) {
    case WEB100_TYPE_IP_ADDRESS:
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
    case WEB100_TYPE_UNSIGNED16:
        return snprintf(dest, size, "%u", *(u_int16_t *) buf);
    default:
        snprintf(dest, size, "%s", "unknown type");
        return -1;
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

int
web100_socket_data_refresh(web100_agent *agent)
{
    struct web100_socket_data *cid_data, *tcp_data, *fd_data, *temp,
                                                *tcp_head, *fd_head;
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
   
    // associate cid with IP
    refresh_connections(agent);

    cid_data = NULL;
    cp = agent->info.local.connection_head;
    while (cp) {
        temp = malloc(sizeof (struct web100_socket_data));
        web100_get_connection_spec(cp, &(temp->spec));
        temp->cid = cp->cid;

        temp->next = cid_data;
        cid_data = temp;

        cp = cp->info.local.next; 
    } 
    
    // associate IP with ino
    file = fopen("/proc/net/tcp", "r");

    tcp_data = NULL;
    while(fgets(buf, sizeof(buf), file) != NULL) { 
        temp = malloc(sizeof (struct web100_socket_data));

        if((scan = sscanf(buf, "%*u: %x:%x %x:%x %x %*x:%*x %*x:%*x %*x %u %*u %u",
                (u_int32_t *) &(temp->spec.src_addr),
                (u_int16_t *) &(temp->spec.src_port),
                (u_int32_t *) &(temp->spec.dst_addr),
                (u_int16_t *) &(temp->spec.dst_port),
                (u_int *) &(temp->state),
                (u_int *) &(temp->uid),
                (u_int *) &(temp->ino))) == 7) { 
            temp->next = tcp_data; 
            tcp_data = temp; 
       	} else {
            free(temp);
        }
    }
    tcp_head = tcp_data;
    fclose(file); 

    // associate ino with pid
    if(!(dir = opendir("/proc")))
    {
        perror("/proc");
        exit(1);
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
			if((temp = malloc(sizeof(struct web100_socket_data))) == NULL){
			    fprintf(stderr, "Out of memory\n");
			    exit(1);
		       	}

			temp->ino = st.st_ino;
		       	temp->pid = pid; 

		       	temp->next = fd_data; 
		       	fd_data = temp; 

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
    if(socket_data) free(socket_data);

    socket_data_arraysize = 2;
    socket_data = calloc(socket_data_arraysize, sizeof (struct web100_socket_data));
    socket_data_arraylength = 0;
    
    while(cid_data) { 
	if(ii == socket_data_arraysize) {
	    socket_data_arraysize *= 2;
	    socket_data = realloc(socket_data, (sizeof (struct web100_socket_data)*socket_data_arraysize));
	}

	socket_data[ii].cid = cid_data->cid; 
	memcpy(&(socket_data[ii].spec), &cid_data->spec, sizeof (struct web100_connection_spec)); 
	socket_data[ii].state = 0; // until we learn otherwise

	tcp_data = tcp_head;
	while(tcp_data) {
	    if(tcp_data->spec.dst_port == cid_data->spec.dst_port &&
               tcp_data->spec.dst_addr == cid_data->spec.dst_addr &&
               tcp_data->spec.src_port == cid_data->spec.src_port) { 

		socket_data[ii].ino = tcp_data->ino;
		socket_data[ii].uid = tcp_data->uid;
		socket_data[ii].state = tcp_data->state;
		
		fd_data = fd_head;
		while(fd_data) {
		    if(fd_data->ino == tcp_data->ino) { 
			socket_data[ii].pid = fd_data->pid;
			strncpy(socket_data[ii].cmdline, fd_data->cmdline, PATH_MAX);
		    } 
		    fd_data = fd_data->next;
		}
	    } 
	    tcp_data = tcp_data->next;
	} 
	socket_data_arraylength = ++ii;
	cid_data = cid_data->next;
    } 

    return WEB100_ERR_SUCCESS;
}

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
	    if(execlp(tracescript, tracescript, web100_value_to_text(WEB100_TYPE_UNSIGNED16, &conn->spec.src_port), web100_value_to_text(WEB100_TYPE_IP_ADDRESS, &conn->spec.dst_addr), web100_value_to_text(WEB100_TYPE_UNSIGNED16, &conn->spec.dst_port), 0) == -1) {
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

    if((fread(snap->data, snap->group->size, 1, log->fp)) != 1) {
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


web100_group* 
web100_get_snapfile_group(web100_snapfile *snapfile)
{
    return snapfile->group;
}

web100_connection*
web100_get_snapfile_connection(web100_snapfile *snapfile)
{
    return snapfile->connection;
}

web100_snapfile*
web100_snapfile_openw(char *logname, web100_connection *conn,
		      web100_group *group)
{
    FILE      *header;
    int       c;
    struct tm *tmp;
    time_t    timep;
    char      tmpbuf[MAX_TMP_BUF_SIZE];
    
    web100_snapfile *snapfile=NULL;

    if(!conn) {
	web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "Invalid Connection: "); 
	goto Cleanup;
    }

    if(!group) {
	web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "Invalid Group: "); 
	goto Cleanup;
    }

    if ((snapfile = (web100_snapfile *)malloc(sizeof (web100_snapfile))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
	strcpy(web100_errstr, "Memory Allocation Failed: "); 
	goto Cleanup;
    }

    if ((header = fopen(WEB100_HEADER_FILE, "r")) == NULL) {
        web100_errno = WEB100_ERR_HEADER;
	strcpy(web100_errstr, "fopen header: "); 
        goto Cleanup;
    }

    snapfile->group       = group;
    snapfile->connection  = conn;
    snapfile->fp          = NULL;

    if((snapfile->fp = fopen(logname, "w")) == NULL) {
	web100_errno = WEB100_ERR_FILE;
	strcpy(web100_errstr, "fopen logfile: "); 
	goto Cleanup;
    }

    while ( (c=fgetc(header)) != EOF ){
      if( fputc( c, snapfile->fp ) != c ){
	web100_errno = WEB100_ERR_FILE;
	strcpy(web100_errstr, "fputc: "); 
	goto Cleanup;
      }
    }

    if( fclose( header )) { 
	web100_errno = WEB100_ERR_FILE;
	strcpy(web100_errstr, "fclose header:"); 
	goto Cleanup;
    } 
    //
    // Put and end of HEADER marker
    //
    fprintf(snapfile->fp, "%s\n", END_OF_HEADER_MARKER);
    //
    // Put Date and Time
    //
    timep          = time(NULL);
    tmp            = localtime(&timep);

    fprintf(snapfile->fp, "%4.4d_%2.2d_%2.2d.%2.2d:%2.2d:%2.2d\n",
	    tmp->tm_year+1900,tmp->tm_mon, tmp->tm_mday,
	    tmp->tm_hour,  tmp->tm_min, tmp->tm_sec);
    //
    // Put in group name
    //
    fprintf(snapfile->fp, "%s\n", group->name);
    //
    // Put in connection spec-ascii
    //
    if( web100_value_to_textn(tmpbuf, MAX_TMP_BUF_SIZE, WEB100_TYPE_IP_ADDRESS,
			      &(conn->spec.src_addr)) < 0 )
    {
        web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "Invalid datatype: "); 
	goto Cleanup;
    }
    fprintf(snapfile->fp, "<%s:", tmpbuf);

    if( web100_value_to_textn(tmpbuf, MAX_TMP_BUF_SIZE, WEB100_TYPE_UNSIGNED16,
			      &(conn->spec.src_port)) < 0 )
    {
        web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "Invalid datatype: "); 
	goto Cleanup;
    }
    fprintf(snapfile->fp, "%s>", tmpbuf);

    if( web100_value_to_textn(tmpbuf, MAX_TMP_BUF_SIZE, WEB100_TYPE_IP_ADDRESS,
			      &(conn->spec.dst_addr)) < 0 )
    {
        web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "Invalid datatype: "); 
	goto Cleanup;
    }
    fprintf(snapfile->fp, "<%s:", tmpbuf);

    if( web100_value_to_textn(tmpbuf, MAX_TMP_BUF_SIZE, WEB100_TYPE_UNSIGNED16,
			      &(conn->spec.dst_port)) < 0 )
    {
        web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "Invalid datatype: "); 
	goto Cleanup;
    }
    fprintf(snapfile->fp, "%s>\n", tmpbuf);
    //
    // Put in connection spec (in binary)
    //
    fwrite(&(conn->spec), sizeof(struct web100_connection_spec), 1, 
	   snapfile->fp);

    web100_errno = WEB100_ERR_SUCCESS;

Cleanup:
    if(web100_errno != WEB100_ERR_SUCCESS) { 
	strcat(web100_errstr, "web100_snapfile_open "); 
	if(snapfile->fp != NULL)
  	    fclose(snapfile->fp);
	free(snapfile);
	return NULL;
    } 
    return snapfile;
}
int
web100_snapfile_closew(web100_snapfile *snapfile)
{
    
    if( fclose( snapfile->fp ) != 0 ) { 
	web100_errno = WEB100_ERR_FILE;
	strcpy(web100_errstr, "web100_snapfile_closew"); 
	free(snapfile);
	return -web100_errno;
    } 
    free(snapfile);
    return WEB100_ERR_SUCCESS;
}

int
web100_snapfile_write(web100_snapfile *snapfile, web100_snapshot *snap)
{
    int count;

    if( snapfile->fp == NULL ){
	web100_errno = WEB100_ERR_FILE;
	strcpy(web100_errstr, "web100_snapfile_write: File not open"); 
	goto Cleanup;
    }        

    if( snapfile->group != snap->group ){
	web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "web100_snapfile_write: Group mismatch"); 
	goto Cleanup;
    }        

    if( snapfile->connection != snap->connection ){
	web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "web100_snapfile_write: Connection mismatch"); 
	goto Cleanup;
    }        

    fprintf(snapfile->fp, "%s\n", BEGIN_SNAP_DATA);
    count = fwrite(snap->data, snap->group->size, 1, snapfile->fp); 

    web100_errno = WEB100_ERR_SUCCESS;
    return count;

Cleanup:
    if(web100_errno != WEB100_ERR_SUCCESS) { 
	strcat(web100_errstr, "web100_snapfile_write"); 
	return -web100_errno;
    } 
}
web100_snapfile*
web100_snapfile_openr(char *logname, web100_agent *agent)
{
    int           c, c2;
    web100_group  *gp;
    web100_var    *vp;
    int           discard;
    char      	  tmpbuf[MAX_TMP_BUF_SIZE];
    char          group_name[WEB100_GROUPNAME_LEN_MAX];
    web100_connection             *cp;
    struct web100_connection_spec spec; /* connection tuple */
    
    web100_snapfile *snapfile=NULL;

    if ((snapfile = (web100_snapfile *)malloc(sizeof (web100_snapfile))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
	strcpy(web100_errstr, "Memory Allocation Failed: "); 
	goto Cleanup;
    }

    if( agent )
        free(agent);

    if ((agent = malloc(sizeof(web100_agent))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
	goto Cleanup;
    }

    /* agent must be 0-filled to get the correct list adding semantics */
    bzero(agent, sizeof(web100_agent));

    agent->type = WEB100_AGENT_TYPE_LOCAL;
    
    if ((snapfile->fp = fopen(logname, "r")) == NULL) {
        web100_errno  = WEB100_ERR_HEADER;
        goto Cleanup;
    }
    
    if (fscanf(snapfile->fp, "%[^\n]", agent->version) != 1) {
        web100_errno = WEB100_ERR_HEADER;
        goto Cleanup;
    }

    /* XXX: Watch out for failure cases, be sure to deallocate memory
     * properly */
    
    IFDEBUG(printf("_web100_agent_attach_local: version = %s\n", agent->version));
   
    gp = NULL; 
    while (!feof(snapfile->fp) && !ferror(snapfile->fp)) {
        while (isspace(c = fgetc(snapfile->fp)))
            ;
        
        if (c < 0) {
            break;
        } else if (c == '/') {
            if (gp && discard)
                free(gp);
            
            if ((gp = (web100_group*) malloc(sizeof(web100_group))) == NULL) {
                web100_errno = WEB100_ERR_NOMEM;
                goto Cleanup;
            }
                
            gp->agent = agent;
            
            if (fscanf(snapfile->fp, "%s", gp->name) != 1) {
                web100_errno = WEB100_ERR_HEADER;
                goto Cleanup;
            }
            
            IFDEBUG(printf("_web100_agent_attach_local: new group: %s\n", gp->name));
            
            gp->size = 0;
            gp->nvars = 0;
            
            if (strcmp(gp->name, "spec") == 0) {
                discard = 1;
            } else {
                discard = 0;
                gp->info.local.var_head = NULL;
                gp->info.local.next = agent->info.local.group_head;
                agent->info.local.group_head = gp;
            }
        } else {
            ungetc(c, snapfile->fp);
            
            if (gp == NULL) {
                web100_errno = WEB100_ERR_HEADER;
                goto Cleanup;
            }
            
            if ((vp = (web100_var *)malloc(sizeof (web100_var))) == NULL) {
                web100_errno = WEB100_ERR_NOMEM;
                goto Cleanup;
            }

            vp->group = gp;
            
	    if( fgets(tmpbuf, MAX_TMP_BUF_SIZE, snapfile->fp) == NULL ) {
                web100_errno = WEB100_ERR_HEADER;
                goto Cleanup;
	    }

	    if ( strncmp(tmpbuf, END_OF_HEADER_MARKER,strlen(END_OF_HEADER_MARKER)) == 0 ) {
	        free(vp);
	        goto Done;
	    }

            if (sscanf(tmpbuf, "%s%d%d", vp->name, &vp->offset, &vp->type) != 3) {
                web100_errno = WEB100_ERR_HEADER;
                goto Cleanup;
            }

            IFDEBUG(printf("_web100_agent_attach_local: new var: %s %d %d\n", vp->name, vp->offset, vp->type));
            
            gp->size += size_from_type(vp->type);
            gp->nvars++;
            
            vp->info.local.next = gp->info.local.var_head;
            gp->info.local.var_head = vp;
        }
    }
 Done:
    fgets( tmpbuf, MAX_TMP_BUF_SIZE, snapfile->fp );
    //    fprintf(stderr, "Date and Time: %s", tmpbuf);

    //fgets( group_name , MAX_TMP_BUF_SIZE, snapfile->fp );
    fscanf(snapfile->fp, "%s[^\n]", group_name);
    while( (c2=fgetc(snapfile->fp)) != '\n' )
        ;    // Cleanup the line

    //fprintf(stderr, "Group name   : %s\n", group_name);

    fgets( tmpbuf, MAX_TMP_BUF_SIZE, snapfile->fp );
    //fprintf(stderr, "Conn-spec    : %s\n", tmpbuf);

    fread( &spec, sizeof(struct web100_connection_spec), 1, snapfile->fp );

    //
    // Set up connection information - similar to refresh_connections
    //
    if ((cp = (web100_connection *)malloc(sizeof (web100_connection))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
	goto Cleanup;
    }
    cp->agent    = agent;
    cp->cid      = WEB100_LOG_CID; 
    cp->logstate = 0; 

    cp->info.local.next = NULL;
    agent->info.local.connection_head = cp;
    cp->spec = spec;

    snapfile->connection = cp;
    snapfile->group      = web100_group_find(agent, group_name);

    return snapfile;

 Cleanup:

    if (snapfile != NULL) {
        if (snapfile->fp != NULL) {
	    fclose(snapfile->fp);
	}
    }
    
    strcpy(web100_errstr, "_web100_agent_attach_local");
    web100_detach(agent);
    agent = NULL;
    
    return NULL;
}
int
web100_snapfile_closer(web100_snapfile *snapfile, web100_agent *agent)
{
    if( snapfile != NULL ) {
        if( fclose( snapfile->fp ) != 0 ) { 
	    web100_errno = WEB100_ERR_FILE;
	    strcpy(web100_errstr, "web100_snapfile_close"); 
	    return -web100_errno;
	} 
    }
    free(snapfile);
    web100_detach(agent);

    return WEB100_ERR_SUCCESS;
}

int
web100_snapfile_read(web100_snapfile *snapfile, web100_snapshot *snap)
{
    int count;
    char tmpbuf[MAX_TMP_BUF_SIZE];

    if( snapfile->fp == NULL ){
	web100_errno = WEB100_ERR_FILE;
	strcpy(web100_errstr, "File not open"); 
	goto Cleanup;
    }        

    if( snapfile->group != snap->group ){
	web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "Group mismatch"); 
	goto Cleanup;
    }        

    if( snapfile->connection != snap->connection ){
	web100_errno = WEB100_ERR_INVAL;
	strcpy(web100_errstr, "Connection mismatch"); 
	goto Cleanup;
    }        

    fscanf(snapfile->fp, "%s[^\n]", tmpbuf);
    while( (fgetc(snapfile->fp)) != '\n' )
        ;    // Cleanup the line

    if( strcmp(tmpbuf,BEGIN_SNAP_DATA) != 0 ){
        web100_errno = 1001;         // User error - outside of errorlist range
	strcpy(web100_errstr, "SNAP DATA Magic Mismatch"); 
	goto Cleanup;
    }        
        
    count = fread(snap->data, snap->group->size, 1, snapfile->fp); 

    web100_errno = WEB100_ERR_SUCCESS;
    return count;

Cleanup:

    if(web100_errno != WEB100_ERR_SUCCESS) { 
	strcat(web100_errstr, "web100_snapfile_read"); 
    } 
    return -web100_errno;
}
