/*  */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <web100/web100.h>
#include "connection_info-int.h"


static int
connection_info_refresh(web100_agent *agent, struct connection_info **conninfo)
{ 
    struct connection_info *cid_data, *tcp_data, *fd_data;
    struct connection_info *tmp, *tcp_head, *fd_head;
    struct connection_info *ci;

    web100_connection *cp;
    web100_group *gp;
    web100_var *var;

    char buf[256], path[PATH_MAX];
    FILE *file, *file6; 
    int scan;
    DIR *dir, *fddir;
    struct dirent *direntp, *fddirentp;
    int fd;
    struct stat st;
    int stno;
    pid_t pid;
    int ii=0;
    int tcp_entry, fd_entry;

    // read LocalAddressType
#if 0
    if((gp = web100_group_find(agent, "read")) == NULL)
	return WEB100_ERR_NOGROUP;

    if ((var = web100_var_find(gp, "LocalAddressType")) == NULL)
	locaddrtype = WEB100_ADDRTYPE_IPV4;
    else if (web100_raw_read(var, cp, &locaddrtype) != WEB100_ERR_SUCCESS)            return web100_errno;
#endif
    // associate cid with IP 
    cid_data = NULL;
    cp = web100_connection_head(agent);
    while (cp) { 
	char *addr_name, *port_name;
	void *dst;

        if((tmp = malloc(sizeof (struct connection_info))) == NULL) 
	    return WEB100_ERR_NOMEM;
	
//
//#if 0
        if((gp = web100_group_find(agent, "read")) == NULL) 
	    return WEB100_ERR_NOGROUP;

	if ((var = web100_var_find(gp, "LocalAddressType")) == NULL)
	    tmp->addrtype = WEB100_ADDRTYPE_IPV4;
	else if (web100_raw_read(var, cp, &tmp->addrtype) != WEB100_ERR_SUCCESS)
	    return web100_errno;
//#endif 
	
        if (strncmp(web100_get_agent_version(agent), "1.", 2) == 0) {
            addr_name = "RemoteAddress";
            port_name = "RemotePort";
        } else {
            addr_name = "RemAddress";
            port_name = "RemPort";
        }
        
        if ((var = web100_var_find(gp, "LocalAddress")) == NULL)
            return WEB100_ERR_FILE;
        if (web100_raw_read(var, cp, buf) != WEB100_ERR_SUCCESS)
            return web100_errno;
        if (tmp->addrtype == WEB100_ADDRTYPE_IPV4)
            memcpy(&tmp->spec.src_addr, buf, 4);
        else
            memcpy(&tmp->spec_v6.src_addr, buf, 16);
        
        if ((var = web100_var_find(gp, addr_name)) == NULL)
            return WEB100_ERR_FILE;
        if (web100_raw_read(var, cp, buf) != WEB100_ERR_SUCCESS)
            return web100_errno;
        if (tmp->addrtype == WEB100_ADDRTYPE_IPV4)
            memcpy(&tmp->spec.dst_addr, buf, 4);
        else
            memcpy(&tmp->spec_v6.dst_addr, buf, 16);
        
        if ((var = web100_var_find(gp, "LocalPort")) == NULL)
            return WEB100_ERR_FILE;
        dst = (tmp->addrtype == WEB100_ADDRTYPE_IPV4) ? &tmp->spec.src_port : &tmp->spec_v6.src_port;
        if (web100_raw_read(var, cp, dst) != WEB100_ERR_SUCCESS)
            return web100_errno;
        
        if ((var = web100_var_find(gp, port_name)) == NULL)
            return WEB100_ERR_FILE;
        dst = (tmp->addrtype == WEB100_ADDRTYPE_IPV4) ? &tmp->spec.dst_port : &tmp->spec_v6.dst_port;
        if (web100_raw_read(var, cp, dst) != WEB100_ERR_SUCCESS)
            return web100_errno;
//
//        web100_get_connection_spec(cp, &(tmp->spec));
        tmp->cid = web100_get_connection_cid(cp);

        tmp->next = cid_data;
        cid_data = tmp;

        cp = web100_connection_next(cp); 
    } 
    
    // associate IP with ino
#if 0
    if((file = fopen("/proc/net/tcp", "r")) == NULL) {
        web100_errno = WEB100_ERR_FILE;
	return -WEB100_ERR_FILE;
    }
#endif

    file = fopen("/proc/net/tcp", "r");
    file6 = fopen("/proc/net/tcp6", "r");

    tcp_data = NULL;
    if(file) {
	while(fgets(buf, sizeof(buf), file) != NULL) { 
	    if((tmp = malloc(sizeof (struct connection_info))) == NULL) {
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
	fclose(file);
    }

    if(file6) {
	while(fgets(buf, sizeof(buf), file6) != NULL) { 
	    if((tmp = malloc(sizeof (struct connection_info))) == NULL) {
		web100_errno = WEB100_ERR_NOMEM;
		return -WEB100_ERR_NOMEM;
	    }

	    if((scan = sscanf(buf, "%*u: %s:%x %s:%x %x %*x:%*x %*x:%*x %*x %u %*u %u",
			    (char *) &(tmp->spec.src_addr),
			    (u_int16_t *) &(tmp->spec.src_port),
			    (char *) &(tmp->spec.dst_addr),
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
	fclose(file6);
    }
    tcp_head = tcp_data;
//    fclose(file); 

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
			if((tmp = malloc(sizeof(struct connection_info))) == NULL){
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
	    if ( ((tcp_data->addrtype == WEB100_ADDRTYPE_IPV4) &&

	          (tcp_data->spec.dst_port == cid_data->spec.dst_port &&
                   tcp_data->spec.dst_addr == cid_data->spec.dst_addr &&
                   tcp_data->spec.src_port == cid_data->spec.src_port)) ||

		 ((tcp_data->addrtype == WEB100_ADDRTYPE_IPV6) &&
		  (tcp_data->spec_v6.dst_port == cid_data->spec_v6.dst_port &&
           !strcmp(tcp_data->spec_v6.dst_addr, cid_data->spec_v6.dst_addr) &&
                   tcp_data->spec_v6.src_port == cid_data->spec_v6.src_port)) ) 
	    { 

		tcp_entry = 1;
		fd_entry = 0;
		for(fd_data = fd_head; fd_data; fd_data = fd_data->next) {
		    if(fd_data->ino == tcp_data->ino) {//then create entry 
			fd_entry = 1;

		       	ci = (struct connection_info *) malloc(sizeof (struct connection_info));
			ci->pid = fd_data->pid;
			strncpy(ci->cmdline, fd_data->cmdline, PATH_MAX); 
			ci->uid = tcp_data->uid;
		       	ci->state = tcp_data->state; 

			ci->cid = cid_data->cid;
			ci->addrtype = cid_data->addrtype;

			if(ci->addrtype == WEB100_ADDRTYPE_IPV4)
			    memcpy(&(ci->spec), &cid_data->spec, sizeof (struct web100_connection_spec));
			if(ci->addrtype == WEB100_ADDRTYPE_IPV6)
			    memcpy(&(ci->spec_v6), &cid_data->spec_v6, sizeof (struct web100_connection_spec_v6));

			ci->next = *conninfo;
			*conninfo = ci;
		    }
		}
		if(!fd_entry) { // add entry w/out cmdline 
		    ci = (struct connection_info *) malloc(sizeof (struct connection_info));
		    ci->pid = 0;
		    strcpy(ci->cmdline, "");
		    ci->uid = tcp_data->uid;
		    ci->state = tcp_data->state;

		    ci->cid = cid_data->cid;
//		    memcpy(&(ci->spec), &cid_data->spec, sizeof (struct web100_connection_spec));

		    ci->addrtype = cid_data->addrtype;

		    if(ci->addrtype == WEB100_ADDRTYPE_IPV4)
			memcpy(&(ci->spec), &cid_data->spec, sizeof (struct web100_connection_spec));
		    if(ci->addrtype == WEB100_ADDRTYPE_IPV6)
			memcpy(&(ci->spec_v6), &cid_data->spec_v6, sizeof (struct web100_connection_spec_v6));

		    ci->next = *conninfo;
		    *conninfo = ci;
		}
	    } 
	} 
	if(!tcp_entry) { // then connection has vanished; add residual cid info
                         // (only for consistency with entries in /proc/web100) 
	    ci = (struct connection_info *) malloc(sizeof (struct connection_info));
	    ci->cid = cid_data->cid;
//	    memcpy(&(ci->spec), &cid_data->spec, sizeof (struct web100_connection_spec));

	    ci->addrtype = cid_data->addrtype;

	    if(ci->addrtype == WEB100_ADDRTYPE_IPV4)
		memcpy(&(ci->spec), &cid_data->spec, sizeof (struct web100_connection_spec));
	    if(ci->addrtype == WEB100_ADDRTYPE_IPV6)
		memcpy(&(ci->spec_v6), &cid_data->spec_v6, sizeof (struct web100_connection_spec_v6));

	    ci->next = *conninfo;
	    *conninfo = ci;
	}
	cid_data = cid_data->next;
    }

    return WEB100_ERR_SUCCESS;
}

static int
connection_info_compar(const struct connection_info *aa, const struct connection_info *bb) {
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

static int
connection_info_find(web100_agent *agent, const struct connection_info *hints, struct connection_info **res)
{
    struct connection_info *conninfo, *tmp, *tmp2, *prev;

    if(connection_info_refresh(agent, &conninfo)) {
	*res = NULL;
	return web100_errno;
    }

    if(hints) {
	tmp = conninfo;
	prev = NULL;
       	while(tmp) { 
	    if(connection_info_compar(hints, tmp)) {
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

struct connection_info*
connection_info_head(web100_agent* agent) {
    static struct connection_info *res;

    if(res) {
	// free the list
    }

    connection_info_find(agent, NULL, &res);

    return res;
}

struct connection_info*
connection_info_next(struct connection_info *conninfo) {

    if(conninfo) return conninfo->next;
}

#if 0
struct connection_info*   connection_info_find_by_pid(web100_agent* _agent, pid_t _pid);
struct connection_info*   connection_info_find_by_uid(web100_agent* _agent, uid_t _uid);
struct connection_info*   connection_info_find_by_state(web100_agent* _agent, int _state);
struct connection_info*   connection_info_find_by_cmdline(web100_agent* _agent, const char *_cmdline);
struct connection_info*   connection_info_find_by_spec(web100_agent* _agent);
struct connection_info*   connection_info_find_by_spec_v6(web100_agent* _agent);
#endif

int
connection_info_get_cid(struct connection_info *conninfo) {
    if(conninfo) return conninfo->cid;
}

pid_t
connection_info_get_pid(struct connection_info *conninfo) {
    if(conninfo) return conninfo->pid;
}

uid_t
connection_info_get_uid(struct connection_info *conninfo) {
    if(conninfo) return conninfo->uid;
}

int
connection_info_get_state(struct connection_info *conninfo) {
    if(conninfo) return conninfo->state;
}

const char*
connection_info_get_cmdline(struct connection_info *conninfo) {
    if(conninfo) return (const char *) conninfo->cmdline;
}

int
connection_info_get_spec(struct connection_info *conninfo, struct web100_connection_spec *spec) {
    memcpy(spec, &conninfo->spec, sizeof (struct web100_connection_spec));
}

int
connection_info_get_spec_v6(struct connection_info *conninfo, struct web100_connection_spec_v6 *spec_v6) {
    memcpy(spec_v6, &conninfo->spec, sizeof (struct web100_connection_spec_v6));
}


















