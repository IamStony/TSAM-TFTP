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

long lSize;
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
                            printf("ReadRequest\n");
                            fflush(stdout);
                            char filename[1024], mode[1024];
                            int modePosition = 0;
                            strncpy(filename, &message[2], sizeof(filename));
                            modePosition = (int) strlen(filename) + 3;
                            strncpy(mode, &message[modePosition], sizeof(mode));
                            //fprintf(stdout, "Filename: %s - Mode: %s\n", filename, mode);

                            //Now to handle it

                            if(confirmConnection(filename, argv[2], mode)) {
                                printf("Managed to open file\n");
                                
                                char response[516];
                                memset(&response, '\0', sizeof(response));
                                response[0] = (char)0;
                                response[1] = (char)3;
                                response[2] = (char)0;
                                response[3] = (char)1;
                                snprintf(&response[4], 513,"%s", &fileBuffer[0]);
                                sendto(sockfd, response, (size_t) sizeof(response), 0,
                                        (struct sockaddr *) &client,
                                        (socklen_t) sizeof(client));
                            }
                            else {
                                printf("Could not open file\n");
                            }
                        }
                       else if(message[1] == 4)
                       {
                           /*Ready to send rest of the file */

                           short package = ntohs(*(short *) (message + 2));//(message[2] << 8) | message[3];
                           //short package2 = ((short) ntohs((message[2])));
                           short nextPackage = (package + 1);
                           int fileS = lSize;
                           int left = fileS - (512 * package);
                           //printf("Package #%d\n->Next package #%hu\n", package, nextPackage);
                           //printf("Package #%d\n", package);
                           //printf("Total filesize: %d     ->     Left of file: %d\n", fileS, left);
                           fflush(stdout);
                           if(left < 0) {
                               printf("Should do nothing\n");
                               fflush(stdout);
                           }
                           else if(left < 512) {
                               //Last package
                               char response[left + 4];
                               response[0] = (char)0;
                               response[1] = (char)3;
                               response[2] = (char)((nextPackage >> 8) & 0xff);
                               response[3] = (char)(nextPackage & 0xff);
                               snprintf(&response[4], left + 1, "%s", &fileBuffer[512 * package]);
                               //printf("Last package being sent\nPackage #%d\n", package);
                               //fflush(stdout);
                               sendto(sockfd, response, (size_t) sizeof(response), 0,
                                       (struct sockaddr *) &client,
                                                       (socklen_t) sizeof(client));
                           }
                           else {
                               //Next package
                               char response[516];
                               response[0] = (char)0;
                               response[1] = (char)3;
                               response[2] = (char)((nextPackage >> 8) & 0xff);
                               response[3] = (char)(nextPackage & 0xff);
                               //short package2 = ntohs(*(short *) (response + 2));
                               //printf("->Package #%d\n", package2);
                               snprintf(&response[4], 513, "%s", &fileBuffer[512 * package]);
                               sendto(sockfd, response, (size_t) sizeof(response), 0,
                                       (struct sockaddr *) &client,
                                               (socklen_t) sizeof(client));   
                           }
                       }
                       else if(message[1] == 5)
                       {
                           printf("Got error");
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
    //long lSize;
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
    printf("Size of file: %lu\n , Size of fileBuffer: %lu\n", lSize, sizeof(fileBuffer));

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
    //free(fileBuffer);
    return true;
}
