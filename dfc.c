#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		400	// Buffer size for socket communication
#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif  /* INADDR_NONE */
#define MAX_CMD_LEN 70

//struct to store parsed command from user
typedef struct configstruct_t
{
	char parse_IP[4][20];
	char parse_portno[4][20];
	char server_root[4][100];
	char parse_username[20];
  char parse_password[20];
}configstruct_t;

// globals
char *configfile="dfc.conf";
extern int	errno;
configstruct_t config_client;
//function declarations
int	errexit(const char *format, ...);
int	connectsock(const char *host, const char *portnum);

int encryption(char *file_name)
{
       char syscommand[100];
       sprintf(syscommand,"./encrypt.py %s %s",file_name,config_client.parse_password);
       system(syscommand);
       return 0;
}
int decryption(char *file_name)
{
       char syscommand[100];
       sprintf(syscommand,"./decrypt.py %s %s",file_name,config_client.parse_password);
       system(syscommand);
       return 0;
}
int serverroot()
{
	//printf("inside server root function\n");
	FILE *fp;
	int i=0;
	char configread[200];
	fp=fopen(configfile,"r");

	while (!feof(fp))
	{
		if(fgets(configread,200,fp)!=NULL)
		{
			char *temp = strtok(configread," ");
			if(strncmp(temp,"Server",6)==0)
			{
				sprintf(config_client.server_root[i],"%s",strtok(NULL," "));
				//printf("Server root : %s\n",config_client.server_root[i]);
				i++;
			}
		}
	}
	fclose(fp);
	//printf("Read server root return\n");
	return 0;
}

int readIP()
{
	//printf("inside readIP function\n");
	FILE *fp;
	int i=0;
	char configread[200];
	fp=fopen(configfile,"r");

	while (!feof(fp))
	{
		if(fgets(configread,200,fp)!=NULL)
		{
			char *temp = strtok(configread," ");
			if(strncmp(temp,"Server",6)==0)
			{
				strcpy(temp,strtok(NULL," "));
				strcpy(config_client.parse_IP[i],strtok(NULL,":"));
				//printf("Server IP : %s\n",config_client.parse_IP[i]);
				i++;
			}
		}
	}
	fclose(fp);
	//printf("Read serverIPreturn\n");
	return 0;
}
int readport()
{
	//printf("inside readport function\n");
	FILE *fp;
	int i=0;
	char configread[200];
	fp=fopen(configfile,"r");

	while (!feof(fp))
	{
		if(fgets(configread,200,fp)!=NULL)
		{
			char *temp = strtok(configread," ");
			if(strncmp(temp,"Server",6)==0)
			{
				strcpy(temp,strtok(NULL," "));
				strcpy(temp,strtok(NULL,":"));
				strcpy(config_client.parse_portno[i],strtok(NULL,"\n\0"));
				//printf("Server Port : %s\n",config_client.parse_portno[i]);
				i++;
			}
		}
	}
	fclose(fp);
	return 0;
}

int authentication(int sock)
{
       //sending username to server
       char auth_buffer[MAX_CMD_LEN];
       bzero(auth_buffer,MAX_CMD_LEN);
			 sprintf(auth_buffer,"auth");
			 write(sock, auth_buffer, MAX_CMD_LEN);

       sleep(1);
       bzero(auth_buffer,MAX_CMD_LEN);
       sprintf(auth_buffer,"%s:%s",config_client.parse_username,config_client.parse_password);
       int n = write(sock, auth_buffer, MAX_CMD_LEN);

       sleep(1);
       bzero(auth_buffer,MAX_CMD_LEN);
       n = read(sock, auth_buffer, MAX_CMD_LEN);

			 if((strncmp(auth_buffer,"1",1)) == 0)
			 {
	       return 0;
			 }
      return 1;

}


int get(char *file_name_user)
{
	char file_name[100];
	sprintf(file_name,"%s",file_name_user);
	int	sock[4];			/* socket descriptor*/
	char command[100];
	sprintf(command,"get %s",file_name_user);
	char buffer[BUFSIZE];
	bzero(buffer,BUFSIZE);
	FILE *fp;
	char temp_check[100];

	for(int s=0;s<4;s++)
	{
		sock[s] = connectsock((config_client.parse_IP[s]),(config_client.parse_portno[s]));
		if(sock[s] < 0)
		{
			close(sock[s]);
			continue;
		}

		for(int c=0;c<4;c++)
		{
			sock[s] = connectsock((config_client.parse_IP[s]),(config_client.parse_portno[s]));

			bzero(temp_check,200);
			sprintf(buffer,"get .%s.%d",file_name,c+1);
			sprintf(temp_check,".%s.%d",file_name,c+1);
			char sys_command[100];
			sprintf(sys_command,"find . -size 0 -delete");
			system(sys_command);
			if(access(temp_check,F_OK)==0)
			{
				continue;
				close(sock[s]);
			}
			write(sock[s], buffer, MAX_CMD_LEN);

			bzero(buffer,BUFSIZE);
			sleep(1);
			long int n = read(sock[s], buffer, 1);
			if(n>0)
			{
				fp = fopen(temp_check,"w");
			}
			else
			{
				close(sock[s]);
				continue;
			}
			while(n > 0)
			{
				fwrite(buffer, 1, 1, fp);
				bzero(buffer,BUFSIZE);
				n = read(sock[s], buffer, 1);
			}
			fclose(fp);

			close(sock[s]);

		}
	}

	for(int c=0;c<4;c++)
	{
		bzero(temp_check,200);
		sprintf(temp_check,".%s.%d",file_name,c+1);
		if(access(temp_check,F_OK)!=0)
		{
			printf("Incomplete file.\n");
			break;
		}
		if(c==3)
		{
			FILE *fp = fopen(file_name,"w");
			for(int k=0;k<4;k++)
			{
				bzero(temp_check,200);
				sprintf(temp_check,".%s.%d",file_name,k+1);
				FILE *cp = fopen(temp_check,"r");
				bzero(buffer,BUFSIZE);
				long int n=fread(buffer,1,1,cp);
				while(!feof(cp))
				{
					fwrite(buffer,1,1,fp);
					bzero(buffer,BUFSIZE);
					long int n=fread(buffer,1,1,cp);
				}
				fclose(cp);
			}
			fclose(fp);
			decryption(file_name);
		}
	}

	return 0;
}
int make_dir(char *file_name_user)
{
	printf("mkdir - %s\n",file_name_user);
	char file_name[100];
	sprintf(file_name,"%s",file_name_user);
	int	sock[4];			/* socket descriptor*/
	char command[100];
	sprintf(command,"mkdir %s",file_name_user);

	for(int s=0;s<4;s++)
	{
		sock[s] = connectsock((config_client.parse_IP[s]),(config_client.parse_portno[s]));
		if(sock[s] < 0)
		{
			close(sock[s]);
			continue;
		}
		write(sock[s], command, MAX_CMD_LEN);
		close(sock[s]);
	}

	return 0;

}

int put(char *file_user)
{
	char file_name[100];
	sprintf(file_name,"%s",file_user);
	int	sock[4];			/* socket descriptor*/
	char command[100];
	sprintf(command,"put %s",file_name);
	long int file_size;

	FILE *fp;
	char buffer[BUFSIZE];
	bzero(buffer,BUFSIZE);

	if (!(access(file_name, F_OK) != -1))
	{
		printf("\nFile doesn't exist.\n");
		return 1;
	}

	else
	{
		encryption(file_name);
		int fd=open(file_name,O_RDONLY);
		struct stat fileStat;
		if(fstat(fd,&fileStat) < -1)
		{
			printf("\nFile doesn't exist.1\n");
			return 1;
		}
		else
		{
			file_size = fileStat.st_size;
			//printf("File size=%ld\n",file_size);
		}
		close(fd);

		//start sending the file
		fp = fopen(file_name,"r");
		//md5 checksum logic

		char *cbuf="md5sum";
		char result[100];
		char hash_result[128];
		char *temp;
		sprintf(result,"%s %s | awk '{print $1}' > md5.txt",cbuf,file_name);
		system(result);
		FILE *md5_fp;
		md5_fp = fopen("md5.txt","r");
		fgets(hash_result,128,md5_fp);
		char last_two[2];
		sprintf(last_two,"%c%c",hash_result[30],hash_result[31]);
		fclose(md5_fp);
		char sys_command[100];
		sprintf(sys_command,"rm md5.txt");
		system(sys_command);
		int mod = ((int)strtol(last_two, NULL, 16)) % 4;
		int j = mod;
		int k;
		switch(j)
		{
			case 0:
			k = 3;
			break;
			case 1:
			k = 0;
			break;
			case 2:
			k = 1;
			break;
			case 3:
			k = 2;
			break;
		}

		for(int i=0;i<4;i++)
		{
			for(int s=0;s<4;s++)
			{
				sock[s] = connectsock((config_client.parse_IP[s]),(config_client.parse_portno[s]));
			}

			if(j>3)
			{
				j = (j - 4);
			}

			if(k>3)
			{
				k = (k - 4);
			}

			bzero(buffer,BUFSIZE);
			//SENDING COMMAND
			sprintf(buffer,"put .%s.%d",file_name,(i+1));
			write(sock[j], buffer, MAX_CMD_LEN);
			write(sock[k], buffer, MAX_CMD_LEN);
			bzero(buffer,BUFSIZE);
			sleep(1);
			if(i==3)
			{
				//for last chunk
				bzero(buffer,BUFSIZE);
				long int n = fread(buffer, 1, 1, fp);
				while(n > 0)
				{
					write(sock[j], buffer, n);
					write(sock[k], buffer, n);
					bzero(buffer,BUFSIZE);
					n = fread(buffer, 1, 1, fp);
				}
				for(int s=0;s<4;s++)
				{
					close(sock[s]);
				}
				break;
			}

			for (int t=0; t<(file_size/4);t++)
			{
				bzero(buffer,BUFSIZE);
				long int n = fread(buffer, 1, 1, fp);
				write(sock[j], buffer, n);
				write(sock[k], buffer, n);
				bzero(buffer,BUFSIZE);
			}

			j++;
			k++;
			for(int s=0;s<4;s++)
			{
				close(sock[s]);
			}

		}

		fclose(fp);
	}
	return 0;
}


int list(char *direc)
{
	char files_server[20][200];
	int sock[4];

	char command[100] = "list";
	if((strlen(direc)) > 2)
	{
		sprintf(command,"list %s",direc);
	}
	char buffer[BUFSIZE];
	bzero(buffer,BUFSIZE);
	char list[4][BUFSIZE];
	bzero(files_server[1],200);

	FILE *lp = fopen("list_resp","w");
	for(int s=0;s<4;s++)
	{
		bzero(list[s],BUFSIZE);
		sock[s] = connectsock((config_client.parse_IP[s]),(config_client.parse_portno[s]));
		if(sock[s] < 0)
		{
			close(sock[s]);
			continue;
		}
		write(sock[s], command, MAX_CMD_LEN);
		sleep(1);

		int j = 0;
		while((read(sock[s],buffer,1)) > 0)
		{
			list[s][j] = buffer[0];
			j++;
		}
		fwrite(list[s],1,strlen(list[s]),lp);

		close(sock[s]);
	}
	char end_f[] = "\n.zzendoffile.end.1";
	fwrite(end_f,1,strlen(end_f),lp);
	fclose(lp);

	bzero(buffer,BUFSIZE);

	char sys_command[100];
	strcpy(sys_command, "sort list_resp | uniq > list_resp1" );
 system(sys_command);
	strcpy(sys_command, "rm list_resp" );
 system(sys_command);

	lp = fopen("list_resp1","r");
	char file_server_temp[200];
	bzero(file_server_temp,200);
	int r = 0;
	int f = 0;

	while(!feof(lp))
	{
		fgets(buffer, 200, lp);
		bzero(file_server_temp,200);
		if(buffer[0]=='.')
		{
			char *temp = strtok(((char *)&buffer[1]),".");
			sprintf(file_server_temp,"%s",temp);

			if(f == 0)
			{
				bzero(files_server[f],200);
				strncpy(files_server[f],file_server_temp,strlen(file_server_temp));
				f++;
			}

			else if((strncmp(file_server_temp,files_server[f-1],strlen(file_server_temp))) == 0)
			{
				r++;
				if(r == 3)
				{
					printf("%s\n", files_server[f-1]);
					r = 0;
				}
			}

			else
			{
				if(r != 0)
				{
						printf("%s Incomplete\n", files_server[f-1]);
				}
				bzero(files_server[f],200);
				strncpy(files_server[f],file_server_temp,strlen(file_server_temp));
				f++;
				r = 0;
			}
		}
	}

	fclose(lp);
	strcpy(sys_command, "rm list_resp1" );
 system(sys_command);
	return 0;

}

/*------------------------------------------------------------------------
* errexit - print an error message and exit
*------------------------------------------------------------------------
*/
int errexit(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}
/*------------------------------------------------------------------------
* connectsock - allocate & connect a socket using TCP
*------------------------------------------------------------------------
*/
int connectsock(const char *host, const char *portnum)
{
	struct hostent  *phe;   /* pointer to host information entry    */
	struct sockaddr_in sin; /* an Internet endpoint address         */
	int     s;              /* socket descriptor                    */

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	/* Map port number (char string) to port number (int)*/
	if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
	errexit("can't get \"%s\" port number\n", portnum);

	/* Map host name to IP address, allowing for dotted decimal */
	if ( phe = gethostbyname(host) )
	memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
	errexit("can't get \"%s\" host entry\n", host);

	/* Allocate a socket */
	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
	errexit("can't create socket: %s\n", strerror(errno));

	/* Connect the socket */

		fcntl(s, F_SETFL, O_NONBLOCK);

	connect(s, (struct sockaddr *)&sin, sizeof(sin));
	return s;
}
int credential()
{
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
                           sprintf(config_client.parse_username,"%s",strtok(NULL,"\n"));
                    }
                    if(strncmp(temp,"Password",8) == 0)
                    {
                           sprintf(config_client.parse_password,"%s",strtok(NULL,"\n"));
                    }
              }
       }
       fclose(fp);
       return 0;
}
//wrapper function to read all the configurations from the conf file
int readconf()
{
	if(access(configfile,F_OK)!=0) //check if config file exists
	{
		printf("Configuration file not found\n");
		exit(0);
	}
	//printf("Reading conf file server root\n");
	serverroot();	//read server root
	readIP(); //read server IP
	readport(); //read server port
	credential();
	//printf("Read conf return\n");
	return 1;
}

int main(int argc, char *argv[])
{
	readconf(); // function to read all values from configuration file
	int sock[4];
	for(int s=0;s<4;s++)
	{
		sock[s] = connectsock((config_client.parse_IP[s]),(config_client.parse_portno[s]));
		if(authentication(sock[s])==0)
		{
			close(sock[s]);
		break;
		}
		if(s==3)
		{

				close(sock[s]);

			printf("Invalid Username or Password\n");
			exit(1);
		}
		close(sock[s]);
	}

	char *cmd[]={"get","put","list","mkdir"};
	char command[MAX_CMD_LEN];

	while(1)
	{

	char *file_name;
	bzero(command,MAX_CMD_LEN);
	printf("\nAvailable Commands:-\n");
	printf("\n1. get");
	printf("\n2. put");
	printf("\n3. list");
	printf("\n4. mkdir\n");

	printf("\nEnter Command: ");
	scanf ("%[^\n]%*c", command);

	// Remove trailing newline, if there.
	if ((strlen(command)>0) && (command[strlen (command) - 1] == '\n'))
	command[strlen (command) - 1] = '\0';

	//parse the user input to get the main command word i.e. get or put or ls or exit
	char command_temp[MAX_CMD_LEN];
	for(int i=0;i<strlen(command)+1;i++)
	{
		command_temp[i] = command[i];
	}
	char *parse_cmd;
	parse_cmd = strtok ((char *)command_temp," ");

	//code for command "get"
	if (*parse_cmd == *(cmd[0]))
	{
		file_name = strtok (NULL," \0\n");
		get(file_name);
	}

	else if (*parse_cmd == *(cmd[3]))
	{
		file_name = strtok (NULL," \0\n");
		make_dir(file_name);
	}

	else if (*parse_cmd == *(cmd[1]))
	{
		file_name = strtok (NULL," \0\n");
		put(file_name);
	}

	else if (*parse_cmd == *(cmd[2]))
	{
		if(strlen(command)>4)
		{
			file_name = strtok (NULL,"\0");
			if(strlen(file_name) > 1)
			{
				list(file_name);
			}
		}

		else{
			list("1");
		}

	}

}

	exit(0);
}
