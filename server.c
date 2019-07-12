/*Program Written By Hasitha Dias
Student ID: 789929
University/GitLab Username: diasi
Email: i.dias1@student.unimelb.edu.au
Last Modified Date: 19/04/2018 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

// Number of pending connections that the queue will hold(listen())
#define BACKLOG 10

#define BYTES 1024

// HTTP response Headers
#define HTML_HEADER "HTTP/1.0 200 OK\nContent-Type: text/html\n\n"
#define JPEG_HEADER "HTTP/1.0 200 OK\nContent-Type: image/jpeg\n\n"
#define CSS_HEADER "HTTP/1.0 200 OK\nContent-Type: text/css\n\n"
#define JAVASCRIPT_HEADER "HTTP/1.0 200 OK\nContent-Type: text/javascript\n\n"
#define NOT_FOUND_HEADER "HTTP/1.0 404 Not Found\n\n"
#define BAD_REQUEST_HEADER "HTTP/1.0 400 Bad Request\n\n"

#define DEFAULT_FILE "index.html"

char *ROOT;


/*This function reads(receives) the first line from the HTTP requests.
Adapted from online resource.*/
int recv_new(int fd,char *buffer){
    #define EOL "\r\n"
    #define EOL_SIZE 2

    char *p = buffer;
    int eol_matched = 0;

    // receives the bytes one by one
    while(recv(fd,p,1,0) != 0){
        // if the byte matched the specific eol byte
        if( *p == EOL[eol_matched]){
            eol_matched++;

            // if both the bytes matched the EOL return the received bytes
            if(eol_matched==EOL_SIZE){
                // end of string defined
                *(p+1-EOL_SIZE) = '\0';
                return(strlen(buffer));
            }
        }
        else{
            eol_matched=0;
        }
        // increment the pointer to receive next byte
        p++;
    }
    return(0);
}

/*This function creates a Socket File Descriptor and initiates the server.
Adapted from workshop-5.*/
int start_server(int portno){
    struct sockaddr_in serv_addr;
    int sockfd;

    /* Create TCP socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    /* Create address we're going to listen on (given port number)
    - converted to network byte order & any IP address for
    this machine */

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);  // store in machine-neutral format

    /* Bind address to the socket */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listen on socket - means we're ready to accept connections -
    incoming connection requests will be queued */
    if (listen(sockfd,BACKLOG) < 0){
        perror("ERROR on listening");
        exit(1);
    }

    return sockfd;
}

/*This function receives the HTTP request and sends the appropriate response.*/
void* respond(void* fd){
    char *ptr;
    char request[BYTES], filename[BYTES], data_to_send[BYTES], header_buffer[BYTES];
    int bytes_read, file, *new_fd;
    new_fd = (int *) fd;

    // gets the first line of the HTTP request
    if(recv_new(*new_fd,request) == 0){
        printf("ERROR in receive");
        shutdown(*new_fd,SHUT_RDWR);
        close(*new_fd);
        return 0;
    }

    // Checking for a valid browser request
    ptr = strstr(request," HTTP/");
    if(ptr == NULL){
        printf("ERROR NOT A HTTP REQUEST\n");
    } else{
        *ptr = 0;
        ptr = NULL;

        // checks if it is a GET request
        if(strncmp(request,"GET ",4) == 0){
            ptr=request+4;
            // if not requesting for any file assign default as "index.html"
            if( ptr[strlen(ptr) - 1] == '/' ){
                strcat(ptr,DEFAULT_FILE);
            }
            // creates the filepath using root and requested file
            strcpy(filename,ROOT);
            strcat(filename,ptr);

            // opens the file in read-only mode
            file = open(filename,O_RDONLY,0);

            // checks if file is FOUND
            if ( file !=-1 ){
                // checks the requested filetype to determine the appropriate
                // header
                if (strstr(request,".html") != NULL){
                    sprintf(header_buffer, HTML_HEADER);
                }else if (strstr(request,".jpg") != NULL){
                    sprintf(header_buffer, JPEG_HEADER);
                }else if (strstr(request,".css") != NULL){
                    sprintf(header_buffer, CSS_HEADER);
                }else if(strstr(request,".js") != NULL){
                    sprintf(header_buffer, JAVASCRIPT_HEADER);
                }else {
                    printf("ERROR on FileType\n");
                }

                // sends the HTTP header through the socket
                send(*new_fd, header_buffer,strlen(header_buffer),0);

                // reads all the bytes from the requested file and writes it to
                // the socket
                while ( (bytes_read=read(file, data_to_send, BYTES))>0 ){
                    if (write(*new_fd, data_to_send, bytes_read) < 0 ){
                        perror("ERROR in write");
                    }
                }
            } else{ // if file NOT FOUND
                sprintf(header_buffer, NOT_FOUND_HEADER);
                if(send(*new_fd,header_buffer,strlen(header_buffer),0)==-1){
                    perror("ERROR in send");
                }
                printf("ERROR 404: File Not Found\n");
            }
        }
        else{ // if not a HTTP request
            sprintf(header_buffer, BAD_REQUEST_HEADER);
            if(send(*new_fd, header_buffer, strlen(header_buffer), 0) == -1){
                perror("ERROR in send");
            }
            printf("ERROR 400: Bad Request\n");
        }
    }
    // closes the current connection
    shutdown(*new_fd,SHUT_RDWR);
    close(*new_fd);
    return 0;
}

/*The main function receives the port number, filepath and it calls
other functions to start the server and to respond to requests.*/
int main(int argc, char **argv){
    int newsockfd, portno;
    struct sockaddr_in cli_addr;
    socklen_t clilen;

    // this is the root filepath
    ROOT = argv[2];

    // checks if correct number of arguments are available
    if (argc < 3) {
        printf("ERROR, port/filepath(or both) not provided");
        exit(1);
    }

    // converts port to integer
    portno = atoi(argv[1]);

    // starts the server and gets the Socket File Descriptor
    int sockfd = start_server(portno);

    // goes through all connections
    while(1) { // main accept() loop
        clilen = sizeof(cli_addr);

        // accepts new connections
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        // creates a new thread
        pthread_t threadID;
        pthread_create(&threadID,NULL,respond,&newsockfd);
    }
    return 0;
}
