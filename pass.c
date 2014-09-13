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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define INVALID (-1)
#define MAX_BLOB (4096)

extern int chmp;


void handle_stdn_fd__pass(int conn_sock) {
    void handle_error(char *where) {
        int tmp = errno;
        perror(where);
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(tmp);
    }

    char line[MAX_BLOB];

    int lngt = read(fileno(stdin), line, sizeof(line));

    if (lngt == INVALID) {
        handle_error("handle_stdn_fd__pass():read()");
    }

    line[lngt] = 0;

    if (chmp && line[lngt - 1] == '\n') {
        line[--lngt] = 0;
    }

    int wrte_rslt = write(conn_sock, line, lngt);

    if (wrte_rslt == INVALID) {
        handle_error("handle_stdn_fd__pass():write()");
    }
}


void handle_conn_sock__pass(int conn_sock) {
    void handle_error(char *where) {
        int tmp = errno;
        perror(where);
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(tmp);
    }

    void handle_success(char *where) {
        if (conn_sock != INVALID) {
            close(conn_sock);
        }
        exit(0);
    }

    char line[MAX_BLOB];

    int lngt = read(conn_sock, line, sizeof(line));

    if (lngt == INVALID) {
        handle_error("handle_conn_sock__pass():read()");
    }

    if (lngt == 0) {
        handle_success("handle_conn_sock__pass()");
    }

    line[lngt] = 0;

    int wrte_rslt = write(fileno(stdout), line, lngt);

    if (wrte_rslt == INVALID) {
        handle_error("handle_conn_sock__pass():write()");
    }
}

// vim:ts=4:sw=4:et:
