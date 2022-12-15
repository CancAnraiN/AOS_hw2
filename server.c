#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <netinet/in.h> 
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

const char file_dir[] = "file/";

#define CAP_USER 1
#define CAP_GROUP 2

#define READ_PERMISSION 1
#define WRITE_PERMISSION 2

#define STRING_MAX 50
#define ARRAY_MAX 50

struct account
{
    char usr[STRING_MAX];
    char pwd[STRING_MAX];
    char grp[STRING_MAX];
};

struct account_list
{
    struct account account[ARRAY_MAX];
    size_t size;
};

struct cap
{
    short read;
    short write;
    char filename[STRING_MAX];
};

struct cap_list
{
    char usr[STRING_MAX];
    struct cap cap[ARRAY_MAX];
    size_t size;
};

struct account_list *account_list;
struct cap_list *cap_user;
struct cap_list *cap_group;

int user_size, group_size;

// changemode or create file will call this function.
void print_cap()
{
    int cap_size = 0;
    cap_size = cap_user->size;

    for(int user_loc = 0;user_loc < user_size;user_loc++)
    {
        printf("\n%s:\n", cap_user[user_loc].usr);
        for(int file_count = 0;file_count < cap_user[user_loc].size;file_count++)
	{
            printf("\t");
            if(cap_user[user_loc].cap[file_count].read)
                printf("r");
            else
                printf("-");

            if(cap_user[user_loc].cap[file_count].write)
                printf("w");
            else
                printf("-");
            
            printf("\t%s\n", cap_user[user_loc].cap[file_count].filename);
        }
        
    }
    

    for(int group_loc = 0;group_loc < (group_size + 1);group_loc++)
    {
        printf("\n%s:\n", cap_group[group_loc].usr);
        for(int file_count = 0;file_count < cap_group[group_loc].size;file_count++)
	{
            printf("\t");
            if(cap_group[group_loc].cap[file_count].read)
                printf("r");
            else
                printf("-");

            if(cap_group[group_loc].cap[file_count].write)
                printf("w");
            else
                printf("-");
            
            printf("\t%s\n", cap_group[group_loc].cap[file_count].filename);
        }
    }
}



// Search user location
int search_user(struct account *account)
{
    int user_loc = 0;

    while(user_loc < account_list->size)
    {
        if(strcmp(account_list->account[user_loc].usr, account->usr) == 0)
	{
            if(strcmp(account_list->account[user_loc].pwd, account->pwd) == 0)
                return user_loc;
            else
                return -1;
        }
        user_loc++;
    }

    
    return -1;
}

// Search cap location
// args - search_mode: search user/group cap
int search_cap_loc(int search_mode, int cap_size, struct cap_list *cap_list, struct account *account)
{
    int cap_loc = 0;
    
    while(cap_loc < cap_size)
    {
        if((search_mode == CAP_USER) && (strcmp(account->usr, cap_list[cap_loc].usr) == 0))
            return cap_loc;
        if((search_mode == CAP_GROUP) && (strcmp(account->grp, cap_list[cap_loc].usr) == 0))
            return cap_loc;
        
        cap_loc++;
    }

    return -1;
}

// Search file location of cap_list
int search_file_loc(char *filename, int cap_loc, struct cap_list *cap_list)
{

    for(int location = 0;location < cap_list[cap_loc].size;location++)
    {
        if(strcmp(cap_list[cap_loc].cap[location].filename, filename) == 0)
            return location;
    }

    return -1;
}

// Search file permission of user/group/others
int search_file_permission(int search_mode, char *filename, int user_cap_loc, int group_cap_loc, int others_cap_loc)
{
    int location = 0;
    int permission = 0;


    if((location = search_file_loc(filename, user_cap_loc, cap_user)) != -1)
    {
        if(search_mode == READ_PERMISSION)
            permission |= cap_user[user_cap_loc].cap[location].read;
        else if(search_mode == WRITE_PERMISSION)
            permission |= cap_user[user_cap_loc].cap[location].write;
    }

    if((location = search_file_loc(filename, group_cap_loc, cap_group)) != -1)
    {
        if(search_mode == READ_PERMISSION)
            permission |= cap_group[group_cap_loc].cap[location].read;
        else if(search_mode == WRITE_PERMISSION)
            permission |= cap_group[group_cap_loc].cap[location].write;
    }
    else
    {
        location = search_file_loc(filename, others_cap_loc, cap_group);
        if(search_mode == READ_PERMISSION)
            permission |= cap_group[others_cap_loc].cap[location].read;
        else if(search_mode == WRITE_PERMISSION)
            permission |= cap_group[others_cap_loc].cap[location].write;

    }
    
    if(permission)
        return 0;
    else
        return -1;
}

// Change file read/write permission
int change_mode(char *filename, char *auth, int user_cap_loc, int group_cap_loc, int others_cap_loc){
    int file_loc = 0;

    file_loc = search_file_loc(filename, user_cap_loc, cap_user);
    cap_user[user_cap_loc].cap[file_loc].read = auth[0] == 'r';
    cap_user[user_cap_loc].cap[file_loc].write = auth[1] == 'w';

    file_loc = search_file_loc(filename, group_cap_loc, cap_group);
    cap_group[group_cap_loc].cap[file_loc].read = auth[2] == 'r';
    cap_group[group_cap_loc].cap[file_loc].write = auth[3] == 'w';

    file_loc = search_file_loc(filename, others_cap_loc, cap_group);
    cap_group[others_cap_loc].cap[file_loc].read = auth[4] == 'r';
    cap_group[others_cap_loc].cap[file_loc].write = auth[5] == 'w';

    return 0;
}

// Create file
int create_file(char *filename, char *auth, int user_cap_loc, int group_cap_loc, int others_cap_loc){
    char filepath[100];
    sprintf(filepath, "%s%s", file_dir, filename);
    creat(filepath, O_CREAT | S_IRWXU);

    strcpy(cap_user[user_cap_loc].cap[cap_user[user_cap_loc].size].filename, filename);
    strcpy(cap_group[group_cap_loc].cap[cap_group[group_cap_loc].size].filename, filename);
    strcpy(cap_group[others_cap_loc].cap[cap_group[others_cap_loc].size].filename, filename);

    cap_user[user_cap_loc].size += 1;
    cap_group[group_cap_loc].size += 1;
    cap_group[others_cap_loc].size += 1;

    change_mode(filename, auth, user_cap_loc, group_cap_loc, others_cap_loc);

    return 0;
}

// Send success and message reply
void send_reply(int clientfd, int success, char *message)
{

    printf("%s\n", message);

    if(success == 1)
        send(clientfd, "1", STRING_MAX, 0);
    else if(success == 0)
        send(clientfd, "0", STRING_MAX, 0);
    
    send(clientfd, message, STRING_MAX, 0);
}

// main of service client function
void service_client(int clientfd)
{
    int user_loc = 0;
    int user_cap_loc = 0;
    int group_cap_loc = 0;
    int others_cap_loc = 0;

    struct account account_tmp;
    recv(clientfd, account_tmp.usr, STRING_MAX, 0);
    recv(clientfd, account_tmp.pwd, STRING_MAX, 0);

    // If login failed, then close client socket and terminated service process
    if((user_loc = search_user(&account_tmp)) == -1)
    {
        send_reply(clientfd, 0, "Login failed");
        close(clientfd);
    	exit(0);
    }
    
    if((user_cap_loc = search_cap_loc(CAP_USER, user_size, cap_user, &account_list->account[user_loc])) == -1)
    {
	close(clientfd);
    	exit(0);
    }

    if((group_cap_loc = search_cap_loc(CAP_GROUP, group_size, cap_group, &account_list->account[user_loc])) == -1)
    {
	close(clientfd);
    	exit(0);
    }
	    
    
    others_cap_loc = group_size;
    
    send_reply(clientfd, 1, "Successful login.");

    FILE *fp;
    char operation[STRING_MAX];
    char filename[STRING_MAX];
    char filepath[STRING_MAX * 2];
    char auth[STRING_MAX];
    char buffer[STRING_MAX] = {0};

    while(1)
    {
        bzero(filepath, sizeof(filepath));

        recv(clientfd, operation, STRING_MAX, 0);
        recv(clientfd, filename, STRING_MAX, 0);
        sprintf(filepath, "%s%s", file_dir, filename);
        
        if(strcmp(operation, "create") == 0)
	{
            recv(clientfd, auth, STRING_MAX, 0);

            if(access(filepath, F_OK) == -1)
	    {
                create_file(filename, auth, user_cap_loc, group_cap_loc, others_cap_loc);
                send_reply(clientfd, 1, "File successfully created.");
                print_cap();
            }
            else
                send_reply(clientfd, 0, "File is already exist.");
        }
        else if(strcmp(operation, "read") == 0)
	{
            if(search_file_permission(READ_PERMISSION, filename, user_cap_loc, group_cap_loc, others_cap_loc) == 0)
	    {

                fp = fopen(filepath, "rb");

                if(flock(fileno(fp), LOCK_SH | LOCK_NB) == -1)
                    send_reply(clientfd, 0, "File is writing.");
                else
		{
                    send_reply(clientfd, 1, "Ready to read.");

                    int file_size = 0;

                    fseek(fp, 0, SEEK_END);
                    file_size = ftell(fp);
                    fseek(fp, 0, SEEK_SET);

                    sprintf(buffer, "%d", file_size);
                    send(clientfd, buffer, STRING_MAX, 0);
                    
                    size_t string_size = 0;
                    while((string_size = fread(buffer, 1, sizeof(buffer), fp)) > 0)
		    { 
                        send(clientfd, buffer, string_size, 0);

                        bzero(buffer, sizeof(buffer));
                    }   
                }
                
                fclose(fp);
            }
            else
                send_reply(clientfd, 0, "No permission to read or file does not exists.");
        }
        else if(strcmp(operation, "write") == 0)
	{
            recv(clientfd, auth, STRING_MAX, 0);

            if(search_file_permission(WRITE_PERMISSION, filename, user_cap_loc, group_cap_loc, others_cap_loc) == 0)
	    {

                fp = fopen(filepath, "ab");

                if(flock(fileno(fp), LOCK_EX | LOCK_NB) == -1)
                    send_reply(clientfd, 0, "File is reading.");
                else
		{
                    if(auth[0] == 'o')
		    {
                        fclose(fp);
                        fp = fopen(filepath, "wb");
                        flock(fileno(fp), LOCK_EX | LOCK_NB);
                    }
                    
                    send_reply(clientfd, 1, "Ready to write.");
                    
                    recv(clientfd, buffer, STRING_MAX, 0);
                    int file_size = atoi(buffer);
                    
                    size_t string_size = 0;
                    size_t count_size = 0;
                    while(count_size != file_size)
		    {
                        string_size = recv(clientfd, buffer, STRING_MAX, 0);
                        fwrite(buffer, 1, string_size, fp);

                        count_size += string_size;
                        bzero(buffer, sizeof(buffer));
                    }
                }

                fclose(fp);
            }
            else
                send_reply(clientfd, 0, "No permission to write or file does not exists.");

        }
        else if(strcmp(operation, "chmod") == 0)
	{
            recv(clientfd, auth, STRING_MAX, 0);
            if(access(filepath, F_OK) == 0)
	    {
                if(search_file_loc(filename, user_cap_loc, cap_user) != -1)
		{
                    change_mode(filename, auth, user_cap_loc, group_cap_loc, others_cap_loc);
                    send_reply(clientfd, 1, "Successful changemode.");
                    print_cap();
                }
                else
                    send_reply(clientfd, 0, "No permission to changemode.");
            }
            else
                send_reply(clientfd, 0, "File does not exist.");
        }
        else
	{
            close(clientfd);
	    exit(0);
        }
    }
}


void read_cap_list(char *filename, struct cap_list *cap_list){
    FILE *fp;
    int file_count;
    int count = 0;

    char *line = NULL;
    size_t len = 0;
    char *pch;

    if((fp = fopen(filename, "r")) == NULL)
        exit(EXIT_FAILURE);
    
    while (getline(&line, &len, fp) != -1) 
    {

        pch = strtok(line, ":");
        strcpy(cap_list[count].usr, pch);

        pch = strtok(NULL, ":");
        file_count = atoi(pch);

        cap_list[count].size = file_count;

        for(int i=0;i < file_count;i++)
	{
            getline(&line, &len, fp);

            pch = strtok(line, " ");
            strcpy(cap_list[count].cap[i].filename, pch);

            pch = strtok(NULL, " ");

            
            cap_list[count].cap[i].read = pch[0] == 'r';
            cap_list[count].cap[i].write = pch[1] == 'w';
        }

        count++;
    }

    fclose(fp);
}

// Save cap list and close server process
void save_cap_list(int signo)
{
    FILE *fp;
    if (signo == SIGINT)
    {
        printf("\nServer is saving data...\n");

        fp = fopen("account.txt", "w");

        fprintf(fp, "user_size:%d\ngroup_size:%d\n", user_size, group_size);
        for(int acc_loc = 0;acc_loc < account_list->size;acc_loc++)
	{
            fprintf(fp, "%s:%s:%s\n", account_list->account[acc_loc].usr, account_list->account[acc_loc].pwd, account_list->account[acc_loc].grp);
        }
        fclose(fp);

        fp = fopen("capability_user.txt", "w");

        for(int user_loc = 0;user_loc < user_size;user_loc++)
	{
            fprintf(fp, "%s:%zu\n", cap_user[user_loc].usr, cap_user[user_loc].size);

            for(int file_loc = 0;file_loc < cap_user[user_loc].size;file_loc++)
	    {
                fprintf(fp, "%s ", cap_user[user_loc].cap[file_loc].filename);

                if(cap_user[user_loc].cap[file_loc].read)
                    fprintf(fp, "r");
                else
                    fprintf(fp, "-");
                
                if(cap_user[user_loc].cap[file_loc].write)
                    fprintf(fp, "w");
                else
                    fprintf(fp, "-");
                
                fprintf(fp, "\n");
            }
        }

        fclose(fp);

        fp = fopen("capability_group.txt", "w");

        for(int group_loc = 0;group_loc < (group_size + 1);group_loc++)
	{
            fprintf(fp, "%s:%zu\n", cap_group[group_loc].usr, cap_group[group_loc].size);

            for(int file_loc = 0;file_loc < cap_group[group_loc].size;file_loc++)
	    {
                fprintf(fp, "%s ", cap_group[group_loc].cap[file_loc].filename);

                if(cap_group[group_loc].cap[file_loc].read)
                    fprintf(fp, "r");
                else
                    fprintf(fp, "-");
                
                if(cap_group[group_loc].cap[file_loc].write)
                    fprintf(fp, "w");
                else
                    fprintf(fp, "-");
                
                fprintf(fp, "\n");
            }
        }

        fclose(fp);

        printf("Saved successfully.\n");
    }

    exit(EXIT_SUCCESS);
}

int main() 
{
    FILE *fp;
    
    char line[STRING_MAX];
    size_t len = 0;

    char *pch;

    fp = fopen("account.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    
    fscanf(fp, "user_size:%d\n", &user_size);
    fscanf(fp, "group_size:%d\n", &group_size);

    account_list = mmap(NULL, sizeof(struct account_list), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    cap_user = mmap(NULL, sizeof(struct cap_list) * user_size, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    cap_group = mmap(NULL, sizeof(struct cap_list) * (group_size + 1), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    for(int i = 0;i < user_size;i++)
    {
        fscanf(fp, "%s\n", line);

        pch = strtok(line, ":");
        strcpy(account_list->account[i].usr, pch);

        pch = strtok(NULL, ":");
        strcpy(account_list->account[i].pwd, pch);

        pch = strtok(NULL, ":");
        strcpy(account_list->account[i].grp, pch);
    }
    account_list->size = user_size;

    read_cap_list("capability_user.txt", cap_user);
    read_cap_list("capability_group.txt", cap_group);

    int sockfd, clientfd; 
    struct sockaddr_in serverInfo; 

    
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) 
    { 
        perror("opening stream socket");
        exit(-1);
    }
    /*-------------------------------------------------------*/ 
    /* Create the acceptable connection host with Wildcard */ 
    /*-------------------------------------------------------*/ 
    
    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET; 
    serverInfo.sin_addr.s_addr = INADDR_ANY; 
    serverInfo.sin_port = htons(1234);

    if(bind(sockfd, (struct sockaddr *)&serverInfo, sizeof serverInfo) < 0)
    {
        perror ("binding stream socket");
        exit(-1);
    } 
    listen(sockfd, 5);
    printf("Server open successful.\n");
    pid_t pid;

    while(1)
    { 
        clientfd = accept(sockfd,(struct sockaddr *)0, (socklen_t *)0); 
        if (clientfd == -1)
	{
            perror("accept error !\n");
            exit(-1);
        }
        
        // fork a process to service this new client
        pid = fork();
        if(pid == 0)
            service_client(clientfd);
        else
            signal(SIGINT, &save_cap_list);
    }
    close(sockfd);

    return 0; 
}
