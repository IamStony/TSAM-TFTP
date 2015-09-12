/* A UDP echo server with timeouts.
 *
 * Note that you will not need to use select and the timeout for a
 * tftp server. However, select is also useful if you want to receive
 * from multiple sockets at the same time. Read the documentation for
 * select on how to do this (Hint: Iterate with FD_ISSET()).
 */

#include <assert.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>

void confirmConnection(char* fn, char* m);

int main(int argc, char **argv)
{
        /*if(argc > 1) {
            int i = 0;
            for(i; i < argc; i++) {
                printf("argv[%d] : %s\n", i, argv[i]);
            }
        }*/
        if(argc < 1) {
            return 0;
        }

        int port;
        int sockfd;
        struct sockaddr_in server, client;
        char message[512];

        sscanf(argv[1], "%i", &port);

        /* Create and bind a UDP socket */
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        /* Network functions need arguments in network byte order instead of
           host byte order. The macros htonl, htons convert the values, */
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_port = htons(port);
        bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

        for (;;) {
                fd_set rfds;
                struct timeval tv;
                int retval;

                /* Check whether there is data on the socket fd. */
                FD_ZERO(&rfds);
                FD_SET(sockfd, &rfds);

                /* Wait for five seconds. */
                tv.tv_sec = 5;
                tv.tv_usec = 0;
                retval = select(sockfd + 1, &rfds, NULL, NULL, &tv);

                if (retval == -1) {
                        perror("select()");
                } else if (retval > 0) {
                        /* Data is available, receive it. */
                        assert(FD_ISSET(sockfd, &rfds));
                        
                        /* Copy to len, since recvfrom may change it. */
                        socklen_t len = (socklen_t) sizeof(client);
                        /* Receive one byte less than declared,
                           because it will be zero-termianted
                           below. */
                        ssize_t n = recvfrom(sockfd, message,
                                             sizeof(message) - 1, 0,
                                             (struct sockaddr *) &client,
                                             &len);
                        /* Send the message back. */
                        sendto(sockfd, message, (size_t) n, 0,
                               (struct sockaddr *) &client,
                               (socklen_t) sizeof(client));
                        /* Zero terminate the message, otherwise
                           printf may access memory outside of the
                           string. */
                        message[n] = '\0';
                        /* Print the message to stdout and flush. */
                        //fprintf(stdout, "Received:\n%s\n", message);
                        //fflush(stdout);

                        if(message[1] == 1) {
                            //RRQ
                            char filename[1024], mode[1024];
                            int modePosition = 0;
                            strncpy(filename, &message[2], sizeof(filename));
                            modePosition = (int) strlen(filename) + 3;
                            strncpy(mode, &message[modePosition], sizeof(mode));
                            //fprintf(stdout, "Filename: %s - Mode: %s\n", filename, mode);

                            //Now to handle it
                            confirmConnection(filename, mode);
                        }

                } else {
                        fprintf(stdout, "No message in five seconds.\n");
                        fflush(stdout);
                }
        }
}

void confirmConnection(char* fn, char* m) {
    //fprintf(stdout, "Filename: %s - Mode: %s\n", fn, m);
    FILE *fp;
}
