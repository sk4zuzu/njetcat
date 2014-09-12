//
// NJETCAT 0.3 20140912 copyright sk4zuzu@gmail.com 2014
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
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>

#define INVALID (-1)
#define MAX_EVNTS (16)
#define MAX_BLOB (4096)

static int verb = 0;
static int chmp = 0;


static void _chomp(char *string) {
    int lngt = strlen(string);
    if (lngt > 0) {
        if (string[lngt - 1] == '\n') {
            string[lngt - 1] = 0;
        }
    }
}


static void _SERVER(int port) {
    int lstn_sock = INVALID, conn_sock = INVALID;
    int epoll_fd = INVALID;

    void handle_error(char *where) {
        int tmp = errno;
        perror(where);
        if (lstn_sock != INVALID) {
            close(lstn_sock);
        }
        if (epoll_fd != INVALID) {
            close(epoll_fd);
        }
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(tmp);
    }

    void handle_success(char *where) {
        if (lstn_sock != INVALID) {
            close(lstn_sock);
        }
        if (epoll_fd != INVALID) {
            close(epoll_fd);
        }
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(0);
    }

    void init_lstn_sock() {
        lstn_sock = socket(AF_INET, SOCK_STREAM, 0);

        if (lstn_sock == INVALID) {
            handle_error("init_lstn_sock()");
        }

        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port   = htons(port),
            .sin_addr   = INADDR_ANY
        };

        int sopt_vlue = 1;
        int sopt_rslt = setsockopt(lstn_sock, SOL_SOCKET,
                                            SO_REUSEADDR, &sopt_vlue,
                                                    sizeof(sopt_vlue));
        if (sopt_rslt == INVALID) {
            handle_error("init_lstn_sock()");
        }

        int bind_rslt = bind(lstn_sock, (struct sockaddr *)&addr,
                                                     sizeof(addr));
        if (bind_rslt == INVALID) {
            handle_error("init_lstn_sock()");
        }

        int lstn_rslt = listen(lstn_sock, 4);

        if (lstn_rslt == INVALID) {
            handle_error("init_lstn_sock()");
        }

        if (verb) {
            printf("listening on %s:%d...\n", "0.0.0.0", port);
        }
    }

    init_lstn_sock();

    struct epoll_event evnt,
                       evnts[MAX_EVNTS];

    void init_epoll() {
        epoll_fd = epoll_create(MAX_EVNTS);

        if (epoll_fd == INVALID) {
            handle_error("init_epoll()");
        }

        int ctl_rslt;

        evnt.events = EPOLLIN;
        evnt.data.fd = lstn_sock;

        ctl_rslt = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, lstn_sock,
                                                         &evnt);
        if (ctl_rslt == INVALID) {
            handle_error("init_epoll()");
        }

        evnt.events = EPOLLIN;
        evnt.data.fd = fileno(stdin);

        ctl_rslt = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fileno(stdin),
                                                             &evnt);
        if (ctl_rslt == INVALID) {
            handle_error("init_epoll()");
        }
    }

    init_epoll();

    void handle_lstn_sock() {
        struct sockaddr_in addr;
        int addr_lngt = sizeof(addr);

        conn_sock = accept(lstn_sock, (struct sockaddr *)&addr,
                                                         &addr_lngt);
        if (conn_sock == INVALID) {
            handle_error("handle_lstn_sock()");
        }

        if (verb) {
            printf("connection from %s:%d...\n", inet_ntoa(addr.sin_addr),
                                                     ntohs(addr.sin_port));
        }

        int fcntl_rslt = fcntl(conn_sock, F_SETFL, O_NONBLOCK);

        if (fcntl_rslt == INVALID) {
            handle_error("handle_lstn_sock()");
        }

        evnt.events = EPOLLIN | EPOLLET;
        evnt.data.fd = conn_sock;

        int ctl_rslt = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock,
                                                              &evnt);
        if (ctl_rslt == INVALID) {
            handle_error("handle_lstn_sock()");
        }
    }

    void handle_conn_sock() {
        char line[MAX_BLOB];

        int lngt = read(conn_sock, line, sizeof(line));

        if (lngt == INVALID) {
            handle_error("handle_conn_sock()");
        }

        if (lngt == 0) {
            handle_success("handle_conn_sock()");
        }

        line[lngt] = 0;

        int wrte_rslt = write(fileno(stdout), line, lngt);

        if (wrte_rslt == INVALID) {
            handle_error("handle_conn_sock()");
        }
    }

    void handle_stdn_fd() {
        char line[MAX_BLOB];

        int lngt = read(fileno(stdin), line, sizeof(line));

        if (lngt == INVALID) {
            handle_error("handle_stdn_fd()");
        }

        line[lngt] = 0;

        if (chmp) {
            _chomp(line);
        }

        if (conn_sock != INVALID) {
            int wrte_rslt = write(conn_sock, line, lngt);

            if (wrte_rslt == INVALID) {
                handle_error("handle_stdn_fd()");
            }
        }
    }

    for (;;) {
        int nfds = epoll_wait(epoll_fd, evnts, MAX_EVNTS, INVALID);
        
        if (nfds == INVALID) {
            handle_error("server()");
        }

        int n;

        for (n = 0; n < nfds; ++n) {
            if (evnts[n].data.fd == lstn_sock) {
                handle_lstn_sock();
            }
            else
            if (evnts[n].data.fd == conn_sock) {
                handle_conn_sock();
            }
            else
            if (evnts[n].data.fd == fileno(stdin)) {
                handle_stdn_fd();
            }
        }
    }
}


static void _CLIENT(char *hostname, int port) {
    int conn_sock = INVALID;
    int epoll_fd = INVALID;
    int conn = INVALID;

    void handle_error(char *where) {
        int tmp = errno;
        perror(where);
        if (epoll_fd != INVALID) {
            close(epoll_fd);
        }
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(tmp);
    }

    void handle_success(char *where) {
        if (epoll_fd != INVALID) {
            close(epoll_fd);
        }
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(0);
    }

    void resolve_hostname(struct sockaddr_in *addr) {
        struct hostent *hent = gethostbyname(hostname);

        if (hent && hent->h_addr_list[0]) {
            addr->sin_addr = *(struct in_addr *)(hent->h_addr_list[0]);
        } else {
            addr->sin_addr.s_addr = inet_addr(hostname);
        }
    }

    void init_conn_sock() {
        conn_sock = socket(AF_INET, SOCK_STREAM, 0);

        if (conn_sock == INVALID) {
            handle_error("init_conn_sock()");
        }
    }

    init_conn_sock();

    struct epoll_event evnt,
                       evnts[MAX_EVNTS];

    void init_epoll() {
        epoll_fd = epoll_create(MAX_EVNTS);

        if (epoll_fd == INVALID) {
            handle_error("init_epoll()");
        }

        int ctl_rslt;

        evnt.events = EPOLLIN;
        evnt.data.fd = conn_sock;

        ctl_rslt = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock,
                                                         &evnt);
        if (ctl_rslt == INVALID) {
            handle_error("init_epoll()");
        }

        evnt.events = EPOLLIN;
        evnt.data.fd = fileno(stdin);

        ctl_rslt = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fileno(stdin),
                                                             &evnt);
        if (ctl_rslt == INVALID) {
            handle_error("init_epoll()");
        }
    }

    init_epoll();

    void handle_conn_sock() {
        if (conn == INVALID) {
            struct sockaddr_in addr = {
                .sin_family = AF_INET,
                .sin_port   = htons(port),
                .sin_addr   = 0
            };

            resolve_hostname(&addr);

            int conn_rslt = connect(conn_sock, (struct sockaddr *)&addr,
                                                            sizeof(addr));
            if (conn_rslt == INVALID) {
                handle_error("handle_conn_sock()");
            }

            int fcntl_rslt = fcntl(conn_sock, F_SETFL, O_NONBLOCK);

            if (fcntl_rslt == INVALID) {
                handle_error("handle_conn_sock()");
            }

            conn = !INVALID;

            if (verb) {
                printf("connected to %s:%d...\n", hostname, port);
            }
        } else {
            char line[MAX_BLOB];

            int lngt = read(conn_sock, line, sizeof(line));

            if (lngt == INVALID) {
                handle_error("handle_conn_sock()");
            }

            if (lngt == 0) {
                handle_success("handle_conn_sock()");
            }

            line[lngt] = 0;

            int wrte_rslt = write(fileno(stdout), line, lngt);

            if (wrte_rslt == INVALID) {
                handle_error("handle_conn_sock()");
            }
        }
    }

    void handle_stdn_fd() {
        char line[MAX_BLOB];

        int lngt = read(fileno(stdin), line, sizeof(line));

        if (lngt == INVALID) {
            handle_error("handle_stdn_fd()");
        }

        line[lngt] = 0;

        if (chmp) {
            _chomp(line);
        }

        if (conn_sock != INVALID) {
            int wrte_rslt = write(conn_sock, line, lngt);

            if (wrte_rslt == INVALID) {
                handle_error("handle_stdn_fd()");
            }
        }
    }

    for (;;) {
        int nfds = epoll_wait(epoll_fd, evnts, MAX_EVNTS, INVALID);
        
        if (nfds == INVALID) {
            handle_error("server()");
        }

        int n;

        for (n = 0; n < nfds; ++n) {
            if (evnts[n].data.fd == conn_sock) {
                handle_conn_sock();
            }
            else
            if (evnts[n].data.fd == fileno(stdin)) {
                handle_stdn_fd();
            }
        }
    }
}


int main(int argc, char *argv[]) {
    void print_usage_and_exit() {
        printf("Usage: %s [-V] [-v] [-c] [-l] [-p port] hostname port\n", argv[0]);
        exit(0);
    }

    int srvr = 0,
        port = INVALID,
        opt;
    while ((opt = getopt(argc, argv, "Vvclp:")) != INVALID) {
        switch (opt) {
            case 'V':
                printf("%s\n","NJETCAT 0.3 20140912 copyright sk4zuzu@gmail.com 2014");
                exit(0);
            case 'v':
                verb = 1;
                break;
            case 'c':
                chmp = 1;
                break;
            case 'l':
                srvr = 1;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                print_usage_and_exit();
        }
    }

    if (srvr) {
        if (port != INVALID) {
            _SERVER(port);
        } else {
            print_usage_and_exit();
        }
    } else {
        if (argc - optind == 2) {
            _CLIENT(argv[optind], atoi(argv[optind + 1]));
        } else {
            print_usage_and_exit();
        }
    }
}

// vim:ts=4:sw=4:et:
