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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>

#define INVALID (-1)

extern int verb;
extern int nodl;

void client(char *host, int port, void (*handle_stdn_fd)(int conn_sock),
                                  void (*handle_conn_sock)(int conn_sock)) {

    int conn_sock = INVALID;

    void handle_error(char *where) {
        int tmp = errno;
        perror(where);
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(tmp);
    }

    void handle_success() {
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(0);
    }

    void resolve_hostname(struct sockaddr_in *addr) {
        struct hostent *hent = gethostbyname(host);

        if (hent && hent->h_addr_list[0]) {
            addr->sin_addr = *(struct in_addr *)(hent->h_addr_list[0]);
        } else {
            addr->sin_addr.s_addr = inet_addr(host);
        }
    }

    void init_conn_sock() {
        conn_sock = socket(AF_INET, SOCK_STREAM, 0);

        if (conn_sock == INVALID) {
            handle_error("init_conn_sock():socket()");
        }

        if (nodl) {
            int sopt_vlue = 1;
            int sopt_rslt = setsockopt(conn_sock, IPPROTO_TCP,
                                                  TCP_NODELAY, &sopt_vlue,
                                                         sizeof(sopt_vlue));
            if (sopt_rslt == INVALID) {
                handle_error("init_conn_sock():setsockopt()");
            }
        }

        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port   = htons(port),
            .sin_addr   = 0
        };

        resolve_hostname(&addr);

        int conn_rslt = connect(conn_sock, (struct sockaddr *)&addr,
                                                        sizeof(addr));
        if (conn_rslt == INVALID) {
            handle_error("init_conn_sock():connect()");
        }

        if (verb) {
            printf("connected to %s:%d...\n", inet_ntoa(addr.sin_addr),
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

    init_conn_sock();
    do_poll();
}

// vim:ts=4:sw=4:et:
