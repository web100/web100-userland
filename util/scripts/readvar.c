/*
 * readvar: Reads the current value for one or more web100 variables in
 *          a connection.  Web100 variables are separated by spaces.
 *
 * Usage: readvar <connection id> <var name> [<var name> ...]
 * Example: readvar 1359 LocalPort LocalAddress
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
 * $Id: readvar.c,v 1.3 2002/09/30 19:48:27 engelhar Exp $
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
            "Usage: %s <connection id> <var name> [<var name> ...]\n",
            argv0);
}


int
main(int argc, char *argv[])
{
    web100_agent* agent;
    web100_connection* conn;
    web100_group* group;
    web100_var* var;
    char buf[256];
    int cid;
    char** arg;

    argv0 = argv[0];

    if (argc < 3) {
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
   
    for (arg=&argv[2]; *arg; arg++) {
        if ((web100_agent_find_var_and_group(agent, *arg, &group, &var)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_agent_find_var_and_group");
            exit(EXIT_FAILURE);
        }

        if ((web100_raw_read(var, conn, buf)) != WEB100_ERR_SUCCESS) {
            web100_perror("web100_raw_read");
            exit(EXIT_FAILURE);
        }
    
        printf("%-20s: %s\n", *arg, web100_value_to_text(web100_get_var_type(var), buf));
    }

    web100_detach(agent);

    return 0;
}

