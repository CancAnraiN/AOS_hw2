#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h> 
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STRING_MAX 50
const char file_dir[] = "file/";

int form_server_message(int sockfd)
{
    char flag[STRING_MAX];
    char message[STRING_MAX];

    recv(sockfd, flag, STRING_MAX, 0);
    recv(sockfd, message, STRING_MAX, 0);

    printf("%s\n", message);

    if(strcmp(flag, "1") == 0)
        return 0;
    else
        return -1;
}

int main()
{

    char command[STRING_MAX];
    char filename[STRING_MAX];
    char in_put[STRING_MAX];
    char buffer[STRING_MAX];
    char username[STRING_MAX];
    int sockfd = 0;
    struct sockaddr_in info;

    bzero(&info,sizeof(info));
    info.sin_family = AF_INET;
    info.sin_addr.s_addr = inet_addr("127.0.0.1");
    info.sin_port = htons(1234);

    FILE *fptr;

    /*Create socket and LogIN(丟名子到Server,符合account.txt就可以成功登入)*/
    while(1)
    {
        if((sockfd = socket(PF_INET , SOCK_STREAM , 0)) == -1)
            printf("Fail to create a socket.");
        
        if(connect(sockfd,(struct sockaddr *)&info,sizeof(info)) == -1)
            printf("Error\n");

        printf("Username: ");
        scanf("%s", username);

        send(sockfd, username, STRING_MAX, 0);

        if(form_server_message(sockfd) == 0)
            break;
    }
    

    while(1)
    {
	printf("Example:\n\tcreate homework2.c rwr---\n\tread homework2.c\n\twrite homework2.c o/a\n\tchmod homework2.c rw----\n\texit *anything*\n############################################################################\n");
        printf("Command: ");
        scanf("%s %s", command, filename);
        char filepath[100];
    	sprintf(filepath, "%s%s", file_dir, filename);

        if(strcmp(command, "exit") == 0)
            exit(0);
        
        send(sockfd, command, STRING_MAX, 0);
        send(sockfd, filename, STRING_MAX, 0);

        if(strcmp(command, "create") == 0)
	{
            scanf("%s", in_put);
            send(sockfd, in_put, STRING_MAX, 0);
            form_server_message(sockfd);
        }
        else if(strcmp(command, "read") == 0)
	{

            if(form_server_message(sockfd) == 0)
	    {
                fptr = fopen(filepath, "r");
                recv(sockfd, buffer, STRING_MAX, 0);
               
		int file_size = atoi(buffer);
                size_t string_size = 0;
                size_t count_size = 0;
                while(count_size != file_size)
		{
                    string_size = recv(sockfd, buffer, STRING_MAX, 0);
                    fwrite(buffer, 1, string_size, fptr);
                    count_size += string_size;
                    bzero(buffer, sizeof(buffer));
                }

		char c;
    		while ((c=getc(fptr)) != EOF) 
		{
         		printf("%c", c);
    		}
		printf("\n");

                fclose(fptr);
            }
        }
        else if(strcmp(command, "write") == 0)
	{
            scanf("%s", in_put);
            send(sockfd, in_put, STRING_MAX, 0);

            if(form_server_message(sockfd) == 0)
	    {
                fptr = fopen(filepath, "w");
		//
		int file_size = 0;

                fseek(fptr, 0, SEEK_END);
                file_size = ftell(fptr);
                fseek(fptr, 0, SEEK_SET);
			
                sprintf(buffer, "%d", file_size);
                send(sockfd, buffer, STRING_MAX, 0);
                
                size_t string_size = 0;
                while((string_size = fread(buffer, 1, sizeof(buffer), fptr)) > 0)
		{ 
                    send(sockfd, buffer, string_size, 0);

                    bzero(buffer, sizeof(buffer));
                }

		fprintf(fptr,"%s",in_put);

                fclose(fptr);
                printf("\nDone.\n");
            }   
        }
        else if(strcmp(command, "chmod") == 0)
	{
            scanf("%s", in_put);
            send(sockfd, in_put, STRING_MAX, 0);
            form_server_message(sockfd);
        }
        else
	{
            continue;
        }
    }
}
