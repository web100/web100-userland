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
 * See http://www-unix.mcs.anl.gov/~gropp/manuals/doctext/doctext.html for
 * documentation format.
 *
 * $Id: web100.c,v 1.3 2002/01/23 18:53:05 jestabro Exp $
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

#include <errno.h>

#include "web100-int.h"
#include "web100.h" /*I <web100/web100.h> I*/


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
    "unable to open connection stats",     /* WEB100_ERR_NOCONNECTION */
    "invalid arguments",                   /* WEB100_ERR_INVAL */
    "could not parse " WEB100_HEADER_FILE, /* WEB100_ERR_HEADER */
    "variable not found",                  /* WEB100_ERR_NOVAR */
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
_web100_agent_attach_local(void)
{
    web100_agent* agent = NULL;
    FILE* header = NULL;
    int c;
    web100_group* gp;
    web100_var* vp;
    int discard;

    if ((agent = malloc(sizeof(web100_agent))) == NULL) {
        web100_errno = WEB100_ERR_NOMEM;
        goto Cleanup;
    }

    agent->type = WEB100_AGENT_TYPE_LOCAL;
    
    if ((header = fopen(WEB100_HEADER_FILE, "r")) == NULL) {
        web100_errno = WEB100_ERR_HEADER;
        goto Cleanup;
    }
    
    if (fscanf(header, "%[^\n]", agent->version) != 1) {
        web100_errno = WEB100_ERR_HEADER;
        goto Cleanup;
    }

    /* XXX: Watch out for failure cases, be sure to deallocate memory
     * properly */
    
    IFDEBUG(printf("_web100_agent_attach_local: version = %s\n", agent->version));
   
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
    if (header != NULL) {
        fclose(header);
        header = NULL;
    }
    
    if (web100_errno != WEB100_ERR_SUCCESS) {
	strcpy(web100_errstr, "_web100_agent_attach_local");
        web100_detach(agent);
        agent = NULL;
    }
    
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


/*@
web100_perror - print a web100 error message in the style of perror(3)

Description:
The routine 'web100_perror()' produces a message on the standard error output
describing the last error encountered during a call to a web100 function.
The argument string str is printed first, then a colon and a blank, then the
message and a new-line.  The error number is taken from the external variable
'web100_errno'.

Return value:
None

Errors:
None

See Also:
web100_strerror(3), perror(3)
@*/
void
web100_perror(const char* str)
{
    fprintf(stderr, "%s: %s\n", str, web100_strerror(web100_errno));
}


/*@
web100_strerror - return string describing error code in the style of
strerror(3)

Description:
The 'strerror()' function returns a string describing the error code passed
in the argument 'errnum'.

Return value:
The 'strerror()' function returns the appropriate description string, or an
unknown error message if the error code is unknown.

Errors:
None

See Also:
web100_perror(3), strerror(3)
@*/
const char*
web100_strerror(int errnum)
{
    if (errnum < 0 || errnum >= web100_sys_nerr)
        return "unknown error";
        
    return web100_sys_errlist[errnum];
}


/*@
web100_attach - attach (SNMP terminology)
@*/
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


/*@
web100_detach - detach (SNMP terminology)
@*/
void
web100_detach(web100_agent *agent)
{
    web100_group *gp, *gp2;
    web100_var *vp, *vp2;
    web100_connection *cp, *cp2;
    
    if (agent == NULL || agent->type != WEB100_AGENT_TYPE_LOCAL) {
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


/*@
web100_group_head - obtain first group in agent
@*/
web100_group*
web100_group_head(web100_agent *agent)
{
    if (agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return agent->info.local.group_head;
}


/*@
web100_group_next - obtain next group in group list
@*/
web100_group*
web100_group_next(web100_group *group)
{
    if (group->agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return group->info.local.next;
}


/*@
web100_group_find - find a specific group in the agent by name
@*/
web100_group*
web100_group_find(web100_agent *agent, char *name)
{
    web100_group *gp;
    
    if (agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    gp = agent->info.local.group_head;
    while (gp) {
        if (strcmp(gp->name, name) == 0)
            break;
        gp = gp->info.local.next;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return gp;
}


/*@
web100_var_head - obtain first var in a group
@*/
web100_var*
web100_var_head(web100_group *group)
{
    if (group->agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return group->info.local.var_head;
}


/*@
web100_var_next - obtain next var in a list of vars
@*/
web100_var*
web100_var_next(web100_var *var)
{
    if (var->group->agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
    return var->info.local.next;
}


/*@
web100_var_find - find a var by name in a group
@*/
web100_var*
web100_var_find(web100_group *group, const char *name)
{
    web100_var *vp;
    
    if (group->agent->type != WEB100_AGENT_TYPE_LOCAL) {
        web100_errno = WEB100_ERR_AGENT_TYPE;
        return NULL;
    }
    
    vp = group->info.local.var_head;
    while (vp) {
        if (strcmp(vp->name, name) == 0)
            break;
        vp = vp->info.local.next;
    }
    
    web100_errno = WEB100_ERR_SUCCESS;
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


/*@
web100_connection_head - obtain the first connection in an agent
@*/
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


/*@
web100_connection_next - obtain next connection in list
@*/
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


/*@
web100_connection_find - find connection in agent by spec
@*/
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
    
    return cp;
}


/*@
web100_connection_lookup - find connection in agent by connection id
@*/
web100_connection*
web100_connection_lookup(web100_agent *agent, int cid)
{
    web100_connection *cp;
    
    if (!agent) return;

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
    
    return cp;
}


/*@
web100_connection_data_copy - copy a connection
@*/
int
web100_connection_data_copy(web100_connection *dest, web100_connection *src)
{ 

    if (!dest || !src) return;

    dest->agent = src->agent;
    dest->cid = src->cid;
    memcpy(&dest->spec, &src->spec, sizeof(struct web100_connection_spec)); 
    return WEB100_ERR_SUCCESS;
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

    if (snap->connection->logstate) web100_log(snap);
    
    if (fclose(fp))
        perror("web100_snap: fclose");
    
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
    static char text[12];

    switch(type) {
    case WEB100_TYPE_IP_ADDRESS:
    {
        unsigned char *addr = (unsigned char *) buf; 
        sprintf(text, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
        break;
    }
    case WEB100_TYPE_INTEGER:
    case WEB100_TYPE_INTEGER32:
    case WEB100_TYPE_COUNTER32: 
    case WEB100_TYPE_GAUGE32: 
    case WEB100_TYPE_UNSIGNED32:
    case WEB100_TYPE_TIME_TICKS:
        sprintf(text, "%lu", *(u_int32_t *) buf);
        break;
    case WEB100_TYPE_COUNTER64:
        sprintf(text, "%llu", *(u_int64_t *) buf);
        break;
    case WEB100_TYPE_UNSIGNED16:
        sprintf(text, "%u", *(u_int16_t *) buf);
        break;
    default:
        sprintf(text, "%s", "unknown type");
    }

    return text;
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
    while(fgets(buf, sizeof(buf), file) != NULL){  
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
       	}
       	else free(temp); 
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
}

int
web100_log(web100_snapshot *snap)
{
    static int filesize = 0; 

    fwrite(snap->group->name, WEB100_GROUPNAME_LEN_MAX, 1, snap->connection->logfile); 
    filesize += WEB100_GROUPNAME_LEN_MAX;
    fwrite(snap->data, snap->group->size, 1, snap->connection->logfile); 
    filesize += snap->group->size; 
    printf("%d\n", filesize);
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
}

int
web100_diagnose_define(char *script)
{
    setenv("TRACESCRIPT", script, 1);
}

