/*
 * readall: read all variables from all connections and print them to stdout.
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
 * $Id: readall.c,v 1.4 2002/09/10 21:01:23 jheffner Exp $
 */
#include <stdio.h>
#include <stdlib.h>

#include "web100.h"


int main(int argc, char *argv[])
{
    web100_agent *agent;
    web100_group *group;
    web100_group *read_grp;
    web100_var *addr_type, *laddr, *raddr, *lport, *rport;
    int old_kernel = 0;

    if ((agent = web100_attach(WEB100_AGENT_TYPE_LOCAL, NULL)) == NULL) {
        web100_perror("web100_attach");
        exit(EXIT_FAILURE);
    }

    if ((read_grp = web100_group_find(agent, "read")) == NULL) {
        web100_perror("web100_group_find: read");
        exit(EXIT_FAILURE);
    }
    if ((addr_type = web100_var_find(read_grp, "LocalAddressType")) == NULL) {
        /* We have an old (1.x) kernel */
        old_kernel = 1;
    }
    if ((laddr = web100_var_find(read_grp, "LocalAddress")) == NULL) {
        web100_perror("web100_var_find: LocalAddress");
        exit(EXIT_FAILURE);
    }
    if ((raddr = web100_var_find(read_grp, old_kernel ? "RemoteAddress" : "RemAddress")) == NULL) {
        web100_perror("web100_var_find: RemAddress");
        exit(EXIT_FAILURE);
    }
    if ((lport = web100_var_find(read_grp, "LocalPort")) == NULL) {
        web100_perror("web100_var_find: LocalPort");
        exit(EXIT_FAILURE);
    }
    if ((rport = web100_var_find(read_grp, old_kernel ? "RemotePort" : "RemPort")) == NULL) {
        web100_perror("web100_var_find: RemPort");
        exit(EXIT_FAILURE);
    }

    group = web100_group_head(agent);

    while (group) {
        web100_connection *conn;

        printf("Group \"%s\"\n", web100_get_group_name(group));

        conn = web100_connection_head(agent);

        while (conn) {
            web100_var *var;
            web100_snapshot *snap;
            int type;
            int cid;
            char buf[256];

            cid = web100_get_connection_cid(conn);
            printf("Connection %d (", cid);

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
            type = (type == WEB100_ADDRTYPE_IPV4 ? WEB100_TYPE_INET_ADDRESS_IPV4 : WEB100_TYPE_INET_ADDRESS_IPV6);
            if (web100_raw_read(laddr, conn, buf) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_raw_read");
                exit(EXIT_FAILURE);
            }
            printf("%s ", web100_value_to_text(type, buf));
            if (web100_raw_read(lport, conn, buf) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_raw_read");
                exit(EXIT_FAILURE);
            }
            printf("%s  ",
                   web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER,
                                        buf));
            if (web100_raw_read(raddr, conn, buf) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_raw_read");
                exit(EXIT_FAILURE);
            }
            printf("%s ", web100_value_to_text(type, buf));
            if (web100_raw_read(rport, conn, buf) != WEB100_ERR_SUCCESS) {
                web100_perror("web100_raw_read");
                exit(EXIT_FAILURE);
            }
            printf("%s)\n",
                   web100_value_to_text(WEB100_TYPE_INET_PORT_NUMBER,
                                        buf));

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
                       web100_value_to_text(web100_get_var_type(var),
                                            buf));

                var = web100_var_next(var);
            }

            web100_snapshot_free(snap);

            conn = web100_connection_next(conn);

            if (conn)
                printf("\n");
        }

        group = web100_group_next(group);

        if (group)
            printf("\n");
    }

    return 0;
}
