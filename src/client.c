#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#define PORT 8080

//For Visuals : -------------
void reset_color(){  
    printf("\033[0m");

}
void set_bold(){
    printf("\033[1m");
}
void setcolor_fg(char foreground_color[10]){
    int fn;
    if(strcmp("Black",foreground_color)==0)
        fn=30;
    else if(strcmp("Red",foreground_color)==0)
        fn=31;
    else if(strcmp("Red",foreground_color)==0)
        fn=31;
    else if(strcmp("Green",foreground_color)==0)
        fn=32;
    else if(strcmp("Yellow",foreground_color)==0)
        fn=33;
    else if(strcmp("Blue",foreground_color)==0)
        fn=34;
    else if(strcmp("Magenta",foreground_color)==0)
        fn=35;
    else if(strcmp("Cyan",foreground_color)==0)
        fn=36;   
    else if(strcmp("White",foreground_color)==0)
        fn=37;
    printf("\033[%dm",fn);
    
}
void loading(char ch){
    int i;
     
    for(i=0;i<70;i++)
        {printf("%c",ch);
        fflush(0);
        usleep(15000);}
}
void setcolor_fg_and_bg(char foreground_color[10],char background_color[10]){
    int fn,bn;
    /*
        Foreground:
        30 Black
        31 Red
        32 Green
        33 Yellow
        34 Blue
        35 Magenta
        36 Cyan
        37 White*/
    //printf("\033[30m\033[47m"); // Black foreground / White background
    if(strcmp("Black",foreground_color)==0)
        fn=30;
    else if(strcmp("Red",foreground_color)==0)
        fn=31;
    else if(strcmp("Red",foreground_color)==0)
        fn=31;
    else if(strcmp("Green",foreground_color)==0)
        fn=32;
    else if(strcmp("Yellow",foreground_color)==0)
        fn=33;
    else if(strcmp("Blue",foreground_color)==0)
        fn=34;
    else if(strcmp("Magenta",foreground_color)==0)
        fn=35;
    else if(strcmp("Cyan",foreground_color)==0)
        fn=36;   
    else if(strcmp("White",foreground_color)==0)
        fn=37;
    
    if(strcmp(background_color,"Black")==0)
        bn=40;
    else if(strcmp(background_color,"Red")==0)
        bn=41;
    else if(strcmp(background_color,"Green")==0)
        bn=42;
    else if(strcmp(background_color,"Yellow")==0)
        bn=43;
    else if(strcmp(background_color,"Blue")==0)
        bn=44;
    else if(strcmp(background_color,"Magenta")==0)
        bn=45;
    else if(strcmp(background_color,"Cyan")==0)
        bn=46;
    else if(strcmp(background_color,"White")==0)
        bn=47;
    printf("\033[%dm\033[%dm",fn,bn);
   /*Background:

40 Black
41 Red
42 Green
43 Yellow
44 Blue
45 Magenta
46 Cyan
47 White

0 Reset all
1 Bold    
    */
}
void exit_loading(){
     setcolor_fg("Yellow");
        set_bold();
        printf("\n\n\n\n\t ADIOS AMIGO...");
        loading('.');
        system("clear");
        exit(0);
}
//---------------------


// Handles the read & write operations w the server
void connection_handler(int sockFD)
{
    char readBuffer[1000], writeBuffer[1000]; // A buffer used for reading from / writting to the server
    ssize_t readBytes, writeBytes;            // Number of bytes read from / written to the socket

    char tempBuffer[1000];

    do
    {
        bzero(readBuffer, sizeof(readBuffer)); // Empty the read buffer
        bzero(tempBuffer, sizeof(tempBuffer));
        readBytes = read(sockFD, readBuffer, sizeof(readBuffer));
        if (readBytes <= 0)
            perror("\nERROR_LOG : Error while reading from client socket!");
        else if (strchr(readBuffer, '^') != NULL)
        {
            // Skip read from client
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 1);
            printf("%s", tempBuffer);

            getchar();
            getchar();

            set_bold();
            setcolor_fg("Cyan");
            printf("\n\n\t\t\t\t\tLOADING..\n   ");
            loading('|');
            reset_color();
            system("clear");
            writeBytes = write(sockFD, "^", strlen("^"));
            if (writeBytes == -1)
            {
                perror("ERROR_LOG : Error while writing to client socket!");
                break;
            }
        }
        else if (strchr(readBuffer, '$') != NULL)
        {
            // Server sent an error message and is now closing it's end of the connection
            
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 2);
            printf("\n\n\t%s", tempBuffer);
            printf("\n\n\tClosing the connection to the server now!\n\n");
            exit_loading();
            exit(EXIT_SUCCESS);
        }
        else
        {

            bzero(writeBuffer, sizeof(writeBuffer)); // Empty the write buffer
            // password input
            if (strchr(readBuffer, '>') != NULL)
                {   
                    strcpy(writeBuffer, getpass(readBuffer));
                    system("clear");
                }
            else
            {
                // data input
                
                
                printf("\t\t%s", readBuffer);
                scanf("%s", writeBuffer); // Take user input!
                
            }

            writeBytes = write(sockFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("\n\n\t ERROR_LOG : Error while writing to client socket!");
                printf("\n\n\tClosing the connection to the server now!\n");
                
                break;
            }
        }
    } while (readBytes > 0);

    close(sockFD);
}

void main()
{
    int socketFileDescriptor, connectStatus;
    struct sockaddr_in serverAddress;
    struct sockaddr server;

    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFileDescriptor == -1)
    {
        perror("Error while creating server socket!");
        exit(0);
    }

    serverAddress.sin_family = AF_INET;                     // IPv4
    serverAddress.sin_port = htons(PORT);                   // Server will listen to port 8080
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // htonl(INADDR_ANY); // Binds the socket to all interfaces

    connectStatus = connect(socketFileDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectStatus == -1)
    {
        perror("Error while connecting to server!");
        close(socketFileDescriptor);
        exit(0);
    }
 
    system("clear");
  

    setcolor_fg_and_bg("Black","White");
    set_bold();
    printf("\n\n\n\n\n\t -------------------- WELCOME TO WORLD BANK --------------------------\n\n");
    reset_color();


    char ch;
    set_bold();
    setcolor_fg("Cyan");
    printf("\n\n\tHello User!!! \n\n\tPress 1 : if you wish for ADMINISTRATOR Login\n\tPress 2 : if you wish for CUSTOMER Login \n\tPress * : To exit");
    reset_color();

    setcolor_fg("Yellow");
    printf("\n\n\t\tYour Choice : ");
    scanf("%c", &ch);
    int i;
    printf("\n\n\t");

    if (ch != '1' && ch != '2')
    {
       exit_loading();
    }

    setcolor_fg("White");
    set_bold();
    printf("\n\t\t\t     LOADING..\n   ");
    loading('|');
    printf("\n");
    reset_color();
 
    write(socketFileDescriptor, &ch, 1);

    system("clear");
    connection_handler(socketFileDescriptor);
    close(socketFileDescriptor);
}
