/*
 * readconn: read all variables from a given connection and print them to stdout.
 *           optionally, if a group name is given, print only variables from
 *           that group
 *
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
 * $Id: readconn.c,v 1.1 2002/06/28 20:28:43 rreddy Exp $
 */
#include <stdio.h>
#include <stdlib.h>

#include "web100/web100.h"

static const char* argv0 = NULL;

static void
usage(void)
{
    fprintf(stderr,
            "Usage: %s <connection id> [<grp name>]\n",
            argv0);
}

int
main(int argc, char *argv[])
{
	web100_agent	   *agent;
	web100_group	   *group;
	web100_connection  *conn;

	struct web100_connection_spec spec;

	char buf[8];
	int  cid;

	argv0 = argv[0];
	
	if (argc != 2 && argc != 3) {
	    usage();
	    exit(EXIT_FAILURE);
	}

	if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
		web100_perror("web100_attach");
		exit(EXIT_FAILURE);
	}
	
	cid  = atoi(argv[1]);
    	conn = web100_connection_lookup(agent, cid);

	web100_get_connection_spec(conn, &spec);
	{
		unsigned char *src = (unsigned char *)&spec.src_addr;
		unsigned char *dst = (unsigned char *)&spec.dst_addr;
		printf("Connection %d (%u.%u.%u.%u:%u %u.%u.%u.%u:%u)\n",
		       cid,
		       src[0], src[1], src[2], src[3], spec.src_port,
		       dst[0], dst[1], dst[2], dst[3], spec.dst_port);
	}
		 
	if( argc == 2 ){                  // loop through all groups
	  group = web100_group_head(agent);
	}
	else{                             // only interested in this group
	  group = web100_group_find(agent, argv[2]);
	}

	while (group) {
        
	    web100_var      *var;
	    web100_snapshot *snap;

	    printf("Group \"%s\"\n", web100_get_group_name(group));

	    if ((snap = web100_snapshot_alloc(group, conn)) == NULL) {
		 web100_perror("web100_snapshot_alloc");
		 exit(EXIT_FAILURE);
	    }
            
	    if (web100_snap(snap)) {
		 perror("web100_snap");
		 if (web100_errno == WEB100_ERR_NOCONNECTION)
		       continue;
		 exit(EXIT_FAILURE);
	    }

	    var = web100_var_head(group);

	    while (var) {
		 if (web100_snap_read(var, snap, buf)) {
		     web100_perror("web100_snap_read");
		     exit(EXIT_FAILURE);
		 }
				
		 printf("%-20s %s\n",
			web100_get_var_name(var),
			web100_value_to_text(web100_get_var_type(var), buf));

		 var = web100_var_next(var);
	    }
			
	    web100_snapshot_free(snap);

	    if (argc == 3)          // not looping through groups
	         break;
	    group = web100_group_next(group);

	    if (group)
	         printf("\n");
	}
	
	return 0;
}
