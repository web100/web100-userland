/*
 * deltavar: print a continuous list of the amount a web100 variable changes
 *           each second for one connection.
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
 * $Id: deltavar.c,v 1.1 2002/01/14 17:50:30 jestabro Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "web100.h"

static const char* argv0 = NULL;


static void
usage(void)
{
    fprintf(stderr,
            "Usage: %s <connection id> <var name>\n",
            argv0);
}


int
main(int argc, char *argv[])
{
    web100_agent* agent;
    web100_connection* conn;
    web100_group* group;
    web100_var* var;
    web100_snapshot* snapold;
    web100_snapshot* snapnew;
    char buf[8];
    int cid;

    argv0 = argv[0];

    if (argc != 3) {
        usage();
        exit(EXIT_FAILURE);
    }

    cid = atoi(argv[1]);
    
    if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
        web100_perror("web100_attach");
        exit(EXIT_FAILURE);
    }
    
    if ((conn = web100_connection_lookup(agent, cid)) == NULL) {
        web100_perror("web100_connection_lookup");
        exit(EXIT_FAILURE);
    }
    
    if ((web100_agent_find_var_and_group(agent, argv[2], &group, &var)) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_agent_find_var_and_group");
        exit(EXIT_FAILURE);
    }
    
    if ((snapold = web100_snapshot_alloc(group, conn)) == NULL) {
        web100_perror("web100_snapshot_alloc");
        exit(EXIT_FAILURE);
    }

    if ((snapnew = web100_snapshot_alloc(group, conn)) == NULL) {
        web100_perror("web100_snapshot_alloc");
        exit(EXIT_FAILURE);
    }

    if ((web100_snap(snapold)) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_snap");
        exit(EXIT_FAILURE);
    }

    if ((web100_snap(snapnew)) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_snap");
        exit(EXIT_FAILURE);
    }

    if ((web100_snap_read(var, snapold, buf)) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_snap_read");
        exit(EXIT_FAILURE);
    }

    printf("Initial value of %s: %s\n", argv[2], web100_value_to_text(web100_get_var_type(var), buf));
    
    while (1) {
        sleep(1);

        /* throw out old, new->old, get new using web100_snap */
        
        if ((web100_snap_data_copy(snapold, snapnew)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_snap_data_copy");
            exit(EXIT_FAILURE);
        }

        if ((web100_snap(snapnew)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_snap");
            exit(EXIT_FAILURE);
        }

        if ((web100_delta_any(var, snapnew, snapold,
                              buf)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_delta_any");
            exit(EXIT_FAILURE);
        }

        printf("Change in %s: %s\n", argv[2], web100_value_to_text(web100_get_var_type(var), buf));
    }

    return 0;
}

