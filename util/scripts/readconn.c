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
 * $Id: readconn.c,v 1.4 2002/09/10 21:01:24 jheffner Exp $
 */
#include <stdio.h>
#include <stdlib.h>

#include "web100/web100.h"

static const char *argv0 = NULL;

static void usage(void)
{
    fprintf(stderr, "Usage: %s <connection id> [<grp name>]\n", argv0);
}

int main(int argc, char *argv[])
{
    web100_agent *agent;
    web100_group *group;
    web100_connection *conn;
    web100_group *read_grp;
    web100_var *addr_type, *laddr, *raddr, *lport, *rport;
    int old_kernel = 0;

    char buf[8];
    int cid;
    int type;

    argv0 = argv[0];

    if (argc != 2 && argc != 3) {
        usage();
        exit(EXIT_FAILURE);
    }

    if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
        web100_perror("web100_attach");
        exit(EXIT_FAILURE);
    }

    cid = atoi(argv[1]);
    if ((conn = web100_connection_lookup(agent, cid)) == NULL) {
        web100_perror("web100_connection_lookup");
        exit(EXIT_FAILURE);
    }

    if ((read_grp = web100_group_find(agent, "read")) == NULL) {
        web100_perror("web100_group_find: read");
        exit(EXIT_FAILURE);
    }
    if ((addr_type = web100_var_find(read_grp, "LocalAddressType")) == NULL) {
        /* We have a 1.x kernel */
        old_kernel = 1;
    }
    if ((laddr = web100_var_find(read_grp, "LocalAddress")) == NULL) {
        web100_perror("web100_var_find: LocalAddress");
        exit(EXIT_FAILURE);
    }
    if ((raddr = web100_var_find(read_grp, "RemAddress")) == NULL) {
        web100_perror("web100_var_find: RemAddress");
        exit(EXIT_FAILURE);
    }
    if ((lport = web100_var_find(read_grp, "LocalPort")) == NULL) {
        web100_perror("web100_var_find: LocalPort");
        exit(EXIT_FAILURE);
    }
    if ((rport = web100_var_find(read_grp, "RemPort")) == NULL) {
        web100_perror("web100_var_find: RemPort");
        exit(EXIT_FAILURE);
    }
    
    if (old_kernel) {
        type = WEB100_ADDRTYPE_IPV4;
    } else {
        if (web100_raw_read(addr_type, conn, buf) !=
            WEB100_ERR_SUCCESS) {
            web100_perror("web100_raw_read");
            exit(EXIT_FAILURE);
        }
        type = *(int *) buf;
    }
    type = (type ==
            WEB100_ADDRTYPE_IPV4 ? WEB100_TYPE_INET_ADDRESS_IPV4 :
            WEB100_TYPE_INET_ADDRESS_IPV6);
    if (web100_raw_read(laddr, conn, buf) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_raw_read");
        exit(EXIT_FAILURE);
    }
    printf("%s ", web100_value_to_text(type, buf));
    if (web100_raw_read(lport, conn, buf) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_raw_read");
        exit(EXIT_FAILURE);
    }
    printf("%s  ", web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, buf));
    if (web100_raw_read(raddr, conn, buf) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_raw_read");
        exit(EXIT_FAILURE);
    }
    printf("%s ", web100_value_to_text(type, buf));
    if (web100_raw_read(rport, conn, buf) != WEB100_ERR_SUCCESS) {
        web100_perror("web100_raw_read");
        exit(EXIT_FAILURE);
    }
    printf("%s)\n", web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER, buf));

    if (argc == 2) {            // loop through all groups
        group = web100_group_head(agent);
    } else {                    // only interested in this group
        group = web100_group_find(agent, argv[2]);
    }

    while (group) {
        web100_var *var;
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
