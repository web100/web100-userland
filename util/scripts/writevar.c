/*
 * writevar: write a value to a web100 variable in one connection.
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
 * $Id: writevar.c,v 1.2 2002/02/28 20:15:45 jestabro Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "web100.h"

static const char* argv0 = NULL;


static void
usage(void)
{
    fprintf(stderr,
            "Usage: %s <connection id> <var name> <value>\n",
            argv0);
}


int
main (int argc, char *argv[])
{
    web100_agent* agent;
    web100_connection* conn;
    web100_group* group;
    web100_var* var;
    char buf[8];
    int cid;
    int val;

    argv0 = argv[0];

    if (argc != 4) {
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

    val = atoi(argv[3]);

    if ((web100_raw_write(var, conn, &val)) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_raw_write");
        exit(EXIT_FAILURE);
    }

    return 0;
}

