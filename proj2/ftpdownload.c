#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define BUF_SIZE 1024
#define SERVER_ADDR "193.137.29.15"


// sends a message to the server 
int sendMessage(int sockfd, char * message) {
    int bytes = write(sockfd, message, strlen(message));
    if (bytes <= 0) {
        perror("write()");
        exit(-1);
    }
    return 0;
}


// connects to the server
int connectTo(char * address,int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *h;
  
    if ((h = gethostbyname(address)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    printf("Connecting to %s\n", h->h_name);  

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *) h->h_addr)));    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }

    printf("Connection successfully!\n");  
    return sockfd;
}

// finds the index of a character in a string
int findCharIndex(char * word, char v) {
    for (int i = 0; i < strlen(word); i++) {
        if (word[i] == v) return i;
    }
    return -1;
}


// copies a string from src to dst from start to end
int copyString(char * src, char * dst, int start, int end) {
    int j = 0;
    for (int i = start; i < end; i++) {
        dst[j++] = src[i];
    }

    return 0;
}

// parses the input string into user, password, host, and file
int parseInput(char * input, char * user, char * password, char * host, char * file) {
    
    char inputWithoutFTP[BUF_SIZE] = {0};
    copyString(input, inputWithoutFTP, 6, strlen(input));

    char urlPath[BUF_SIZE] = {0};
    strcpy(urlPath, strstr(inputWithoutFTP, "/"));

    if (strstr(input, "@")) {
        copyString(inputWithoutFTP, user, 0, findCharIndex(inputWithoutFTP, ':'));
        copyString(inputWithoutFTP, password, findCharIndex(inputWithoutFTP, ':') + 1, findCharIndex(inputWithoutFTP, '@'));
        copyString(inputWithoutFTP, host, findCharIndex(inputWithoutFTP, '@') + 1, findCharIndex(inputWithoutFTP, '/'));
        copyString(urlPath, file, 1, strlen(urlPath));
    }
    else {
        strcpy(user, "anonymous");
        strcpy(password, "password"); // login creds not provided
        copyString(inputWithoutFTP, host, 0, findCharIndex(inputWithoutFTP, '/'));
        copyString(urlPath, file, 1, strlen(urlPath));
    }
    

}

int login(int sockfd, char * user, char * password) {
    /*send a string to the server*/
    size_t bytes;
    char buf[BUF_SIZE] = {0};
    printf("Login with user : %s and password : %s\n", user, password);

    sprintf(buf,"user %s\r\n",user);
    sendMessage(sockfd, buf);
    sleep(1);
    memset(buf, 0,BUF_SIZE); //clear socket
    
    
    int bytes_read;
    while((bytes_read=read(sockfd, buf, BUF_SIZE) )> 0) {
        buf[bytes_read]='\0';
        if (strstr(buf, "230 Login successful")) return 0;
        if (strstr(buf, "331 Please specify the password")) break;
        memset(buf, 0,BUF_SIZE); //clear socket
    }
    
    memset(buf, 0,BUF_SIZE); //clear socket

    sprintf(buf,"pass %s\r\n",password);

    sendMessage(sockfd, buf);
    memset(buf, 0,BUF_SIZE); //clear socket
    sleep(1);
    bytes_read=read(sockfd, buf, BUF_SIZE);
    buf[bytes_read]='\0';

    if(strstr(buf, "230")== NULL){
        perror("couldn't login");
        printf("Error: %s\n", buf);
        exit(-1);
    }
    
    return 0;
}


int main(int argc, char** argv){

    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }
    char buf[BUF_SIZE] = {0}, address[BUF_SIZE] = {0}, user[BUF_SIZE] = {0}, password[BUF_SIZE] = {0}, host[BUF_SIZE] = {0}, file[BUF_SIZE] = {0};
    
 
    parseInput(argv[1], user, password, host, file);
 
    int sockfdCommands, sockfdData;
    sockfdCommands = connectTo(host,21);


    login(sockfdCommands, user, password);

    printf("Entering Passive Mode.\n");
    sendMessage(sockfdCommands, "pasv\n");
   
    int bytes_read;
    bytes_read=read(sockfdCommands, buf, BUF_SIZE);
    
    buf[bytes_read]='\0';
    if(strstr(buf, "227")== NULL){
        perror("couldn't enter passive mode");
        printf("Error: %s\n", buf);
        exit(-1);
    }

    int h1=0,h2=0,h3=0,h4=0,p1=0,p2=0,pasvport;
    sscanf(buf,"227 Entering Passive Mode (%i,%i,%i,%i,%i,%i).",&h1,&h2,&h3,&h4,&p1,&p2);
    pasvport= p1*256+p2;

   
    sprintf(address,"%i.%i.%i.%i",h1,h2,h3,h4);
    sockfdData = connectTo(address, pasvport);
   

    memset(buf, 0,BUF_SIZE);

    char wantedFile[BUF_SIZE] = {0};
    sprintf(wantedFile,"retr %s\r\n",file);
    printf("Seeking File...\n");
    
    sendMessage(sockfdCommands, wantedFile);
    bytes_read=read(sockfdCommands, buf, BUF_SIZE);
    
    if(strstr(buf, "150")== NULL){
        perror("couldn't find the file");
        printf("Error: %s\n", buf);
        exit(-1);
    }

    printf("Downloading...\n");

    sleep(1);

    memset(buf, 0,BUF_SIZE);

    FILE * f = fopen("output", "w");

    if(f == NULL)
    {
        /* File not created hence exit */
        printf("Unable to create file locally.\n");
        exit(EXIT_FAILURE);
    }

    while((bytes_read=read(sockfdData,buf,BUF_SIZE))>0){
        fprintf(f, "%s", buf);
        memset(buf, 0,BUF_SIZE);
       // bytes = recv(sockfdData, buf, BUF_SIZE, MSG_DONTROUTE);
        //printf("%s",buf);
    }

    printf("File Received!\n");

    fclose(f);

    if (close(sockfdCommands)<0) {
        perror("close()");
        exit(-1);
    }

    if (close(sockfdData)<0) {
        perror("close()");
        exit(-1);
    }

}