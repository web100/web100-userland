#ifndef __CONNECTION_INFO_H__
#define __CONNECTION_INFO_H__

#include <sys/types.h>
//#include <sys/param.h>
#include <web100/web100.h>

#if 0
struct connection_info {
    int cid;
    struct web100_connection_spec spec;
    struct web100_connection_spec_v6 spec_v6;
    pid_t pid;
    uid_t uid;
    ino_t ino;
    int state;
    char cmdline[PATH_MAX];
    struct connection_info *next, *prev;
};
#endif

struct connection_info*   connection_info_head(web100_agent* _agent);
struct connection_info*   connection_info_next(struct connection_info* _info);

struct connection_info*   connection_info_find_by_pid(web100_agent* _agent, pid_t _pid);
struct connection_info*   connection_info_find_by_uid(web100_agent* _agent, uid_t _uid);
struct connection_info*   connection_info_find_by_state(web100_agent* _agent, int _state);
struct connection_info*   connection_info_find_by_cmdline(web100_agent* _agent, const char *_cmdline);
struct connection_info*   connection_info_find_by_spec(web100_agent* _agent);
struct connection_info*   connection_info_find_by_spec_v6(web100_agent* _agent);

int           connection_info_get_cid(struct connection_info *_info);
pid_t         connection_info_get_pid(struct connection_info *_info);
uid_t         connection_info_get_uid(struct connection_info *_info);
int           connection_info_get_state(struct connection_info *_info);
const char*   connection_info_get_cmdline(struct connection_info *_info);

int           connection_info_get_spec(struct connection_info *_info, struct web100_connection_spec *_spec);
int           connection_info_get_spec_v6(struct connection_info *_info, struct web100_connection_spec_v6 *_spec_v6);



#endif  /* __CONNECTION_INFO_H__ */
