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
#include <stdlib.h>
#include <stdbool.h>

bool confirmConnection(char* fn, char *dir, char* m);

char *fileBuffer;

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

                            if(confirmConnection(filename, argv[2], mode)) {
                                printf("Managed to open file\n");
                                char response[512];
                                memset(&response, '\0', sizeof(response));
                                response[0] = (char)0;
                                response[1] = (char)3;
                                response[2] = (char)0;
                                response[3] = (char)1;
                                snprintf(&response[4], 511,"%s", &fileBuffer[0]);
                            }
                            else {
                                printf("Could not open file\n");
                            }
                        }

                } else {
                        fprintf(stdout, "No message in five seconds.\n");
                        fflush(stdout);
                }
        }
}

bool confirmConnection(char *fn, char *dir, char *m) {
    fprintf(stdout, "File: %s/%s - Mode: %s\n", dir, fn, m);
    FILE *f;
    long lSize;
    char dirFile[strlen(dir) + 1 + strlen(fn) + 1];
    snprintf(dirFile, sizeof(dirFile), "%s/%s", dir, fn);
    
    //printf("Currently: %s.\n", dirFile);
    //printf("Should be: %s/%s.\n", dir, fn);

    f = fopen(dirFile, "r");
    if( !f )
    {
        printf("File open failed\n");
        return false;
    }

    fseek(f, 0L, SEEK_END);
    lSize = ftell(f);
    rewind(f);

    /*allocate memory for entire content */
    fileBuffer = calloc( 1, lSize+1 );
    if( !fileBuffer )
    {
        printf("Calloc failed\n");
        fclose(f);
        return false;
    }

    /* copy the file into the fileBuffer */
    if( 1!=fread( fileBuffer , lSize, 1 , f) )
    {
        printf("Read failed :/\n");
        free(fileBuffer);
        return false;
    }

    fclose(f);
    free(fileBuffer);
    return true;
}
