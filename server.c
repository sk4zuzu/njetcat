//
// NJETCAT 0.5 20140915 copyright sk4zuzu@gmail.com 2014
//
// This file is part of NJETCAT.
//
// NJETCAT is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// NJETCAT is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with NJETCAT.  If not, see <http://www.gnu.org/licenses/>.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

#define INVALID (-1)

extern int verb;
extern int nodl;

void server(int port, void (*handle_stdn_fd)(int conn_sock),
                      void (*handle_conn_sock)(int conn_sock)) {

    int lstn_sock = INVALID,
        conn_sock = INVALID;

    void handle_error(char *where) {
        int tmp = errno;
        perror(where);
        if (lstn_sock != INVALID) {
            close(lstn_sock);
        }
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(tmp);
    }

    void handle_success() {
        if (lstn_sock != INVALID) {
            close(lstn_sock);
        }
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(0);
    }

    void init_lstn_sock() {
        lstn_sock = socket(AF_INET, SOCK_STREAM, 0);

        if (lstn_sock == INVALID) {
            handle_error("init_lstn_sock():socket()");
        }

        int sopt_vlue = 1;
        int sopt_rslt = setsockopt(lstn_sock, SOL_SOCKET,
                                            SO_REUSEADDR, &sopt_vlue,
                                                    sizeof(sopt_vlue));
        if (sopt_rslt == INVALID) {
            handle_error("init_lstn_sock():setsockopt()");
        }

        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port   = htons(port),
            .sin_addr   = INADDR_ANY
        };

        int bind_rslt = bind(lstn_sock, (struct sockaddr *)&addr,
                                                     sizeof(addr));
        if (bind_rslt == INVALID) {
            handle_error("init_lstn_sock():bind()");
        }

        int lstn_rslt = listen(lstn_sock, 1);

        if (lstn_rslt == INVALID) {
            handle_error("init_lstn_sock():listen()");
        }

        if (verb) {
            printf("listening on %s:%d...\n", "0.0.0.0", port);
        }
    }

    void init_conn_sock() {
        struct sockaddr_in addr;
        int addr_lngt = sizeof(addr);

        conn_sock = accept(lstn_sock, (struct sockaddr *)&addr,
                                                         &addr_lngt);
        if (conn_sock == INVALID) {
            handle_error("init_conn_sock():accept()");
        }

        close(lstn_sock);
        lstn_sock = INVALID;

        if (nodl) {
            int sopt_vlue = 1;
            int sopt_rslt = setsockopt(conn_sock, IPPROTO_TCP,
                                                  TCP_NODELAY, &sopt_vlue,
                                                         sizeof(sopt_vlue));
            if (sopt_rslt == INVALID) {
                handle_error("init_conn_sock():setsockopt()");
            }
        }

        if (verb) {
            printf("connection from %s:%d...\n", inet_ntoa(addr.sin_addr),
                                                     ntohs(addr.sin_port));
        }
    }

    void do_poll() {
        struct pollfd fds[2] = {
            {.fd      = conn_sock,
             .events  = POLLIN,
             .revents = 0},
            {.fd      = fileno(stdin),
             .events  = POLLIN,
             .revents = 0}
        };
        for (;;) {
            int poll_rslt = poll(fds, 2, INVALID);
            
            if (poll_rslt == INVALID) {
                handle_error("do_poll():poll()");
            }

            if (fds[0].revents & POLLIN) {
                handle_conn_sock(conn_sock);
            }
            if (fds[1].revents & POLLIN) {
                handle_stdn_fd(conn_sock);
            }

            if (fds[1].revents & POLLHUP) {
                handle_success();
            }
        }
    }

    init_lstn_sock();
    init_conn_sock();
    do_poll();
}

// vim:ts=4:sw=4:et:
