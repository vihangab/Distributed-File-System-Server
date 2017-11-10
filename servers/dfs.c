#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/dir.h>

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		400	// Buffer size for socket communication
#define MAX_CMD_LEN 70
#define MAXPATHLEN 100


extern  int alphasort(); 									//Inbuilt sorting function
int auth_user = 0;
char pathname[MAXPATHLEN];								//pathname for server's directory

//function to be used for command "ls"
int file_select(const struct direct *entry)
{
	if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
	return (0);
	else
	return (1);
}

//struct to store configurations read from the conf file
typedef struct configstruct
{
	char portno[10];
	char rootdirec[100];
	char username[20];
	char password[20];
}configstruct_t;


// globals
extern int	errno;
char *configfile="dfs.conf";
int	msock;
int	ssock;
configstruct_t config_server;

int		errexit(const char *format, ...);
int		passivesock(const char *portnum, int qlen);
// function to handle CTRL+C and exit gracefully
void signalHandler(int sig_num)
{
	close(ssock);
	close(msock);
	exit(0);
}
int readconf()
{
       if(access(configfile,F_OK)!=0) //check if config file exists
       {
              printf("Configuration file not found\n");
              exit(0);
       }
       FILE *fp;
       int i=0;
       char configread[200];
       fp=fopen(configfile,"r");

       while (!feof(fp))
       {
              if(fgets(configread,200,fp)!=NULL)
              {
                    char *temp = strtok(configread,":");
                    if(strncmp(temp,"Username",8) == 0)
                    {
                           sprintf(config_server.username,"%s",strtok(NULL,"\0\n"));
                    }
                    if(strncmp(temp,"Password",8) == 0)
                    {
                           sprintf(config_server.password,"%s",strtok(NULL,"\0\n"));
                    }
              }
       }
       fclose(fp);
       return 0;
}
int server_auth(int fd)
{
       char req_read[MAX_CMD_LEN];
       memset(req_read,0,MAX_CMD_LEN);
       int n = read(fd,req_read,MAX_CMD_LEN);

       if(n > 0)
       {

              if(strncmp(strtok(req_read,":"),config_server.username,strlen(config_server.username)-1) != 0)
              {
                    bzero(req_read,MAX_CMD_LEN);
                    strcpy(req_read,"2");
                    n = write(fd,req_read,MAX_CMD_LEN);
                    return 1;
              }
              if(strncmp(strtok(NULL,"\n\0"),config_server.password,strlen(config_server.password)-1) != 0)
              {
                    bzero(req_read,MAX_CMD_LEN);
                    strcpy(req_read,"3");
                    n = write(fd,req_read, MAX_CMD_LEN);
                    return 1;
              }

                    bzero(req_read,MAX_CMD_LEN);
                    strcpy(req_read,"1");
                    n = write(fd,req_read,MAX_CMD_LEN);
                    return 0;
}
}


int webserver(int fd,configstruct_t *config_parse)//considering forking for every such new request
{
	readconf();
	char *cmd[]={"get","put","list","mkdir","auth"};
	FILE *fp;
	char req_read[BUFSIZE];
	memset(req_read,0,BUFSIZE);
	read(fd,req_read,MAX_CMD_LEN);

	//printf("request from client :%s\n",req_read);

	//parse the command word
	uint8_t command[MAX_CMD_LEN];
	char *parse_cmd;
	char file_name[100];
	char *file_name_temp;

	for(int i=0;i<MAX_CMD_LEN;i++)
	{
		command[i] = req_read[i];
	}
	parse_cmd = strtok ((char *)command," \0\n");
	printf("Command: %s\n",parse_cmd);

	if (*parse_cmd == *(cmd[3]))
	{
		//parse directorye name
		char *direc = strtok (NULL," ");
		char sys_command[100];
		sprintf(sys_command,"mkdir %s/%s",config_parse->rootdirec,direc);
		system(sys_command);
	}

	if (*parse_cmd == *(cmd[4]))
	{
		server_auth(fd);
	}

	else if (*parse_cmd == *(cmd[1]))
	{
		//parse file name
		file_name_temp = strtok (NULL," ");
		//printf ("\nFile name: %s\n",file_name_temp);
		sprintf (file_name,"%s/%s",config_parse->rootdirec,file_name_temp);
		//printf("entire path %s\n",file_name);
		//sending the file
		fp = fopen(file_name,"w");
		memset(req_read,0,BUFSIZE);
		sleep(2);
		while(1)
		{
			long int n = read(fd,req_read,BUFSIZE);
			if(n < 1)
			{
				break;
			}
			fwrite(req_read, n, 1, fp);
			memset(req_read,0,BUFSIZE);
		}
		fclose(fp);
	}

	else if(*parse_cmd == *(cmd[0]))
	{
		file_name_temp = strtok (NULL," ");
		//printf ("\nFile name: %s\n",file_name_temp);
		sprintf (file_name,"%s/%s",config_parse->rootdirec,file_name_temp);
		//printf("entire path %s\n",file_name);
		if(access(file_name,F_OK)!=0)
		{
			return 1;
		}
			//start sending the file
			FILE *fp = fopen(file_name,"r");
			memset(req_read,0,BUFSIZE);
			while(1)
			{
				memset(req_read,0,BUFSIZE);
				long int n = fread(req_read,1,1,fp);
				if(n < 1)
				{
					break;
				}
				write(fd,req_read,n);

			}
			fclose(fp);
		}

	else if (*parse_cmd == *(cmd[2]))
	{
		file_name_temp = strtok (NULL," \0\n");

	    int count,i;
	    struct direct **files;
	    char ls_files[BUFSIZE];
	    bzero(ls_files,BUFSIZE);

	    //check path
	    if(!getcwd(pathname, sizeof(pathname)))
	    {
	        printf("\nError getting pathname\n");
	    }

			else
			{
				sprintf(pathname,"%s/%s",pathname,config_parse->rootdirec);
			}

	
	    //total count of files
	    count = scandir(pathname, &files, file_select, alphasort);
	    /* If no files found, make a non-selectable menu item */
	    if(count <= 0)
	    {
	        printf("\nNo files on the server.\n");
					return 1;
	    }

	    //write a string with all file names
	    for (i=1; i<count+1; ++i)
	    {
	        sprintf(ls_files + strlen(ls_files),"%s\n",files[i-1]->d_name);
	    }

	    sprintf(ls_files + strlen(ls_files),"\n");      //to clear the buffer
			write(fd, ls_files, BUFSIZE);
			}


	return 0;
}
/*------------------------------------------------------------------------
* passivesock - allocate & bind a server socket using TCP
*------------------------------------------------------------------------
*/
int
passivesock(const char *portnum, int qlen)
/*
* Arguments:
*      portnum   - port number of the server
*      qlen      - maximum server request queue length
*/
{

	struct sockaddr_in sin; /* an Internet endpoint address  */
	int     s;              /* socket descriptor             */

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;

	/* Map port number (char string) to port number (int) */
	if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
	{
		errexit("can't get \"%s\" port number\n", portnum);
	}

	/* Allocate a socket */
	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
	errexit("can't create socket: %s\n", strerror(errno));

	/* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		fprintf(stderr, "can't bind to %s port: %s",
		portnum, strerror(errno));
		sin.sin_port=htons(0); /* rfequest a port number to be allocated
		by bind */
		if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		errexit("can't bind: %s\n", strerror(errno));
		else {
			int socklen = sizeof(sin);

			if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
			errexit("getsockname: %s\n", strerror(errno));
		}
	}

	if (listen(s, qlen) < 0)
	errexit("can't listen on %s port: %s\n", portnum, strerror(errno));
	return s;
}

/*------------------------------------------------------------------------
* errexit - print an error message and exit
*------------------------------------------------------------------------
*/
int
errexit(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}

int main(int argc, char *argv[])
{
	configstruct_t *config_parse = (configstruct_t *)malloc(10000);
	char *portnum;
	//readconf(config_parse);

	switch (argc)
	{
		case 3:
		strcpy(config_parse->rootdirec,argv[1]);
		portnum = argv[2];
		break;
		/* FALL THROUGH */
		default:
		exit(1);
	}
	
	signal(SIGINT,signalHandler);//whenever u reacive interrupt pls close socportno %s\n
	signal(SIGQUIT,signalHandler);
	signal(SIGKILL,signalHandler);
	signal(SIGTERM,signalHandler);

	struct sockaddr_in fsin;	/* the from address of a client	*/
	/* master server socket		*/
	unsigned int alen;		/* from-address length		*/

	msock = passivesock(portnum, QLEN);

	while (1)
	{

		alen = sizeof(fsin);
		ssock = accept(msock, (struct sockaddr *)&fsin,&alen);
		if (ssock < 0)
		errexit("accept: %s\n",strerror(errno));

		if (ssock>0)
		{
			int p=fork();
			if (p==0)
			{
				//child process to handle individual clients/req
				close(msock);
				webserver(ssock,config_parse);
				close(ssock);
				exit(0);
			}
			else
			{
				//parent goes back to listening on master sock
				close(ssock);
			}

		}
	}
}
