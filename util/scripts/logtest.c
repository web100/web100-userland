/************************************************************
 * File:      logtest.c					    *
 * Author(s): John Estebrook, NCSA, jestabro@ncsa.uiuc.edu  *
 *            Raghu Reddy, PSC,     rreddy@psc.edu	    *
 * Date:      6/28/2002                                     *
 ************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "web100/web100.h"
//
// Note: Error checking not coded for brevity
//
int main(int argc, char **argv)
{
    web100_agent      *agent;
    web100_group      *group;
    web100_connection *conn;
    web100_snapshot   *snap;
    web100_log 	      *log;
    web100_var 	      *var = NULL;

    char buf[8];
    int ii;

    if(argc<2) {
	printf("Usage: logtest cid\n");
	exit(EXIT_FAILURE);
    }

    agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL);
    group = web100_group_find(agent, "read");
    conn  = web100_connection_lookup(agent, atoi(argv[1]));
    snap  = web100_snapshot_alloc(group, conn); 

    log = web100_log_open_write("logtest.txt", conn, group);

    for(ii=0;ii<3;ii++) {
	web100_snap(snap);
	web100_log_write(log, snap);
	printf("writing snapshot %d\n", ii);
	sleep(1);
    }

    web100_log_close_write(log);
    web100_snapshot_free(snap); 
    snap = NULL;
    log = NULL;
    //
    // Now post process; Normally a different run
    //
    log   = web100_log_open_read("logtest.txt");

    agent = web100_get_log_agent(log);
    group = web100_get_log_group(log);
    conn  = web100_get_log_connection(log);
    snap  = web100_snapshot_alloc(group, conn);

    var   = web100_var_find(group, "CurrTime");

    for(ii=0;ii<3;ii++) { 
       	web100_snap_from_log(snap, log); 
       	web100_snap_read(var, snap, &buf);
       	printf("CurrTime is: %s\n", web100_value_to_text(WEB100_TYPE_COUNTER32, &buf));
    }

    web100_log_close_read(log);

    return;
}
