#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#define BUFSIZE 4096

int opt_port;
int opt_rate;
int opt_maxu;
int opt_stde;

int main( int argc, char *argv[] )
{
	parse_option(argc, argv);

    int sockfd, newsockfd, portno, clilen;
    char buffer[BUFSIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int  n;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        exit(1);
    }
    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = opt_port;	//Set the port number
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
 
    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
                          sizeof(serv_addr)) < 0)
    {
         perror("ERROR on binding");
         exit(1);
    }
    /* Now start listening for the clients, here 
     * process will go in sleep mode and will wait 
     * for the incoming connection
     */
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    int pid;

	int userno = 0;
    while (1) 
    {
	    newsockfd = accept(sockfd, 
	            (struct sockaddr *) &cli_addr, &clilen);
	    if (newsockfd < 0)
	    {
	        perror("ERROR on accept");
	        exit(1);
	    }
		userno++;
		if(userno>opt_maxu)
		{
			writelog("The server is busy, please connect latter.");
			exit(0);
		}
		printf("After join, the current number of user is %d.\n", userno);

		/* Create child process */
		pid = fork();
		if (pid < 0)
		{
		    perror("ERROR on fork");
			exit(1);
		}
		if (pid == 0)  
		{
		    /* This is the client process */
		    close(sockfd);
		    doprocessing(newsockfd);
		    exit(0);
		}
		else
		{
			
		    close(newsockfd);

			pid_t w = 0;
    		int status;
			w = waitpid(pid, &status, WNOHANG);
			if(w!=0)
			{
				userno--;
				printf("After exit, the current number of user is %d.\n", userno);
			}
		}

    } /* end of while */
}

void Alarmhandler(int sig)
{
	printf("Time out!\n");
	writelog("Time out!");
	exit(0);
}

void doprocessing (int sock)
{
    int n;
    char buffer[BUFSIZE];

	int mypid;
	mypid = getpid();
	printf("My pid is %d\n", mypid);

	long time_queue[4096];
	int count = 0;

    while(1)
	{
		bzero(buffer,BUFSIZE);

		n = read(sock,buffer,BUFSIZE);
		if (n < 0)
		{
		    perror("ERROR reading from socket");
		    exit(1);
		}
		printf("Here is the message: %s\n",buffer);

		signal(SIGALRM, Alarmhandler);
		alarm(30);
		
		if(buffer[0]=='e'&&buffer[1]=='x'&&buffer[2]=='i'&&buffer[3]=='t')
		{
			return 1;
		}

		struct timeval req_time;
		gettimeofday(&req_time, NULL);
		long second = req_time.tv_sec;

		if(count>=4096)
		{
			exit(0);
		}
		time_queue[count] = second;
		
		if(count>=opt_rate && (time_queue[count]-time_queue[count-opt_rate])<60)
		{
			printf("You requested too frequently.\n");
			writelog("You requested too frequently.");
			n = write(sock,"You requested too frequently.",29);
			if (n < 0) 
			{
				perror("ERROR writing to socket");
				exit(1);
			}
		}
		else
		{
			if(opt_stde==1)
			{
				writelog("The mode only allow you to trace yourself.");
				traceme(sock);
			}
			else
			{
				if(buffer[0]=='h'&&buffer[1]=='e'&&buffer[2]=='l'&&buffer[3]=='p')
				{
					printf("traceroute [destination machine]\ntraceroute file [file name]\ntraceroute me\nhelp");
					bzero(buffer,BUFSIZE);
					n = write(sock,"traceroute [destination machine]\ntraceroute file [file name]\ntraceroute me\nhelp",99);
					if (n < 0) 
					{
						perror("ERROR writing to socket");
						exit(1);
					}

					bzero(buffer,BUFSIZE);
					n = read(sock,buffer,BUFSIZE);
					if (n < 0)
					{
						perror("ERROR reading from socket");
						exit(1);
					}
				}

				char *ch;
				int cmp;
				const char *tr = "traceroute";
				ch = (char*) malloc(11);
				strncpy(ch, buffer, 10);
				cmp = strcmp(tr, ch);

				if(cmp == 0)
				{
					if(buffer[11]=='m'&&buffer[12]=='e')
					{
						traceme(sock);
					}
					else if(buffer[10]=='#')
					{
						char* getfilename(char*);
						char* fname = getfilename(buffer);
						tracefile(sock, fname);
					}
					else
					{
						traceroute(sock, buffer);
					}
				}
				else
				{
					//tracedefault(sock);
					printf("Invalid command, try to use \"help\" next time.\n");
					exit(1);
				}
			}
		}
		count++;

		n = write(sock,"I got your command",18);
		if (n < 0) 
		{
		    perror("ERROR writing to socket");
		    exit(1);
		}
    }
}

void parse_option (int argc, char *argv[])
{
	if(argc == 1)
	{
		printf("You are using the default settings: \n-p 3001 -r 4 -u 2 -s 0");
		opt_port = 1216;
		opt_rate = 4;
		opt_maxu = 2;
		opt_stde = 0;
	}
	else if(argc == 9)
	{
		int count;
		for(count=0;count<argc;count++)
		{
			printf("%s ", argv[count]);
		}
		printf("\n");

		opt_port = atoi(argv[2]);
		opt_rate = atoi(argv[4]);
		opt_maxu = atoi(argv[6]);
		opt_stde = atoi(argv[8]);
	}
	else
	{
		perror("Wrong option format! \nYou can leave the options blank to use the default setting or use the format below: \n-p XXXX -r X -u X -s X");
		exit(1);
	}

	if(opt_port<1025 || opt_port>65536)
	{
		perror("ERROR port number. Use 1025 - 65536.");
        exit(1);
	}
	if(opt_rate<1)
	{
		perror("ERROR rate limiting. It should be larger than 0.");
        exit(1);
	}
	if(opt_maxu<1)
	{
		perror("ERROR maximum number of users. It should be larger than 0.");
        exit(1);
	}
	if(opt_stde<0 || opt_stde>1)
	{
		perror("ERROR target restriction. It should be 0 or 1.");
        exit(1);
	}
}

int execute (char *command, char *buf, int bufmax)
{
	FILE *fp;
	int i;
	if((fp=popen(command, "r"))==NULL)
	{
		perror(command);
		i = sprintf(buf, "server error: %s cannot execute. \n", command);
	}
	else
	{
		i = 0;
		while((buf[i]=fgetc(fp))!=EOF&&i<bufmax-1)
			i++;
		pclose(fp);
	}
	return i;
}

int tracedefault (int fd)
{
	char buf[BUFSIZE];
	int buf_size = 0;
	buf_size = execute("traceroute www.google.com", buf, BUFSIZE);

	int n;
	n = write(fd, buf, buf_size);
	if (n < 0) 
	{
		perror("ERROR writing to socket");
		exit(1);
	}
	return 0;
}

int traceroute (int fd, char *trace_comm)
{
	char buf[BUFSIZE];
	int buf_size = 0;
	buf_size = execute(trace_comm, buf, BUFSIZE);

	int n;
	n = write(fd, buf, buf_size);
	if (n < 0) 
	{
		perror("ERROR writing to socket");
		exit(1);
	}
	return 0;
}

int traceme (int fd)
{
	char buf[BUFSIZE];
	int buf_size = 0;
	buf_size = execute("traceroute 98.223.232.29", buf, BUFSIZE);

	int n;
	n = write(fd, buf, buf_size);
	if (n < 0) 
	{
		perror("ERROR writing to socket");
		exit(1);
	}
	return 0;
}

//extract commands from the file and call traceroute to execut
int tracefile (int fd, char *file_name)
{
	FILE *file = fopen(file_name, "r");
	if(file!=NULL)
    {
		char line[256];
		while(fgets(line, sizeof line, file)!=NULL)
		{
			fputs(line, stdout);
			printf("Command: %s\n", line);
			traceroute(fd, line);
		}
		fclose(file);
    }
	else
    {
		perror(file_name);
    }
	return 0;
}

char* getfilename(char* str2)
{
	char delims[] = "#";
	char *command=NULL;
	char *filename=NULL;
	char *space=NULL;
	char *result=NULL;
	result=strtok( str2,delims);
	int i;
	volatile int breakingflag=3;
  
	for(i=0;i<3;i++)
    {
		{
	   		printf("command is\" %s\"\n", result);
	   		command=strtok(NULL, delims);
		}
       	if(i=1)
		{
	   		printf("filename is\" %s\"\n", command);
	   		filename=strtok(NULL, delims);
		}
       	if(i=2)
		{
			printf("space is\" %s\"\n", filename);
			space=strtok(NULL, delims);
		}
    }
	return command;

}

int writelog(char* msg)
{
	FILE *file;
	file=fopen("Admin.log", "w");
 	char *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	time_t timep;
	struct tm *p;
	time(&timep);
	p = gmtime(&timep);
	char *message1;
	message1=msg;
	printf(message1);
	printf("\n");
	fprintf(file,"%s",message1);
	fprintf(file,"\n");
	fprintf(file,"%d/%d/%d", (1900+p->tm_year), (1+p->tm_mon), p->tm_mday);
	fprintf(file,"%s %d:%d:%d\n", wday[p->tm_wday], p->tm_hour+8, p->tm_min, p->tm_sec);

	return 1;
}

