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

void handle_stdn_fd__pass(int conn_sock);
void handle_conn_sock__pass(int conn_sock);
void handle_conn_sock__echo(int conn_sock);

void server(int port, void (*handle_stdn_fd)(int conn_sock),
                      void (*handle_conn_sock)(int conn_sock));
void client(char *host, int port, void (*handle_stdn_fd)(int conn_sock),
                                  void (*handle_conn_sock)(int conn_sock));

#define INVALID (-1)

int verb = 0;
int chmp = 0;
int echo = 0;


int main(int argc, char *argv[]) {
    void print_usage_and_exit() {
        printf("Usage: %s [-h] [-V] [-v] [-E] [-C] [-l] [-p port]"
                                                       " hostname port\n"
               "    -h       show help\n"
               "    -V       show version\n"
               "    -v       be verbose\n"
               "    -E       enable echo mode\n"
               "    -C       chomp newline from stdin\n"
               "    -l       enable server mode\n"
               "    -p port  provide port for server mode\n", argv[0]);
        exit(0);
    }
    void print_version_and_exit() {
        printf("%s\n","NJETCAT 0.4 20140913 copyright sk4zuzu@gmail.com 2014");
        exit(0);
    }

    int srvr = 0,
        port = INVALID,
        opt;
    while ((opt = getopt(argc, argv, "hVvEClp:")) != INVALID) {
        switch (opt) {
            case 'h':
                print_usage_and_exit();
            case 'V':
                print_version_and_exit();
            case 'v':
                verb = 1;
                break;
            case 'E':
                echo = 1;
                break;
            case 'C':
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
            server(port, handle_stdn_fd__pass,
                          echo ? handle_conn_sock__echo
                               : handle_conn_sock__pass);
        } else {
            print_usage_and_exit();
        }
    } else {
        if (argc - optind == 2) {
            client(argv[optind], atoi(argv[optind + 1]),
                    handle_stdn_fd__pass,
                          echo ? handle_conn_sock__echo
                               : handle_conn_sock__pass);
        } else {
            print_usage_and_exit();
        }
    }
}

// vim:ts=4:sw=4:et:
