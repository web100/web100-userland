#ifndef __CONNECTION_INFO_H__
#define __CONNECTION_INFO_H__

#include <sys/types.h> 
#include "web100.h"


struct connection_info*   connection_info_head(web100_agent* _agent);
struct connection_info*   connection_info_next(struct connection_info* _info);

int              connection_info_get_cid(struct connection_info *_info);
pid_t            connection_info_get_pid(struct connection_info *_info);
uid_t            connection_info_get_uid(struct connection_info *_info);
int              connection_info_get_state(struct connection_info *_info);
const char*      connection_info_get_cmdline(struct connection_info *_info); 
WEB100_ADDRTYPE  connection_info_get_addrtype(struct connection_info *_info);
int              connection_info_get_spec(struct connection_info *_info, struct web100_connection_spec *_spec);
int              connection_info_get_spec_v6(struct connection_info *_info, struct web100_connection_spec_v6 *_spec_v6);


#endif  /* __CONNECTION_INFO_H__ */
