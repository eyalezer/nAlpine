#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#define DLOG 0
#define MAX_BUF_LEN 1024
#define MAX_METHOD_LEN 10
#define MAX_PATH_LEN 800
#define MAX_HTTPSTR_LEN 12

int parse_first_http(const char* http_req, const int req_len)
{
	char method[MAX_METHOD_LEN];
	char path[MAX_PATH_LEN];
	char http[MAX_HTTPSTR_LEN];
	int i = 0;
	int j = 0;
	
	if(http_req == NULL)
		return -1;

#if DLOG == 1
	printf("REQUEST-LEN: %d\n",req_len); 
#endif

	/* parse method-string */
	while( http_req[i] != ' ' )
	{
		if(http_req[i] == '\0')
			return -2;

		if(j >= MAX_METHOD_LEN-1)
			return -3;

		if(i == req_len)
			return -4;

		method[j++] = http_req[i++];
	}

	method[j] = '\0';

#if DLOG == 1
	printf("METHOD: %s\n",method);
#endif

	/* At the moment only GET is supported */
	if(strcmp(method,"GET") != 0)
		return -11;
	j = 0;
	i++;

	/* parse path-string */
	while( http_req[i] != ' ' && (http_req[i] != '\0')) 
	{
		if(j >= MAX_PATH_LEN-1)
			return -5;

		if(i == req_len)
			return -6;

		path[j++] = http_req[i++];
	}

	path[j] = '\0';

#if DLOG == 1
	printf("PATH: %s\n",path);
#endif

	//////////////////////////////
	// handle specific requests //
	//////////////////////////////
	if (!strncmp(&path[0], "/restart\0", 9))
	{
		system("service minidlna restart");
	} 
 	else if (!strncmp(&path[0], "/rescan\0", 8))
	{
		system("sed -i 's/_RESCAN=\"false\"/_RESCAN=\"true\"/' /etc/conf.d/minidlna; service minidlna restart; sed -i 's/_RESCAN=\"true\"/_RESCAN=\"false\"/' /etc/conf.d/minidlna;");
	}
	else if (!strncmp(&path[0], "/rebuild\0", 9))
	{
		system("sed -i 's/RESCAN=\"false\"/RESCAN=\"true\"/' /etc/conf.d/minidlna; service minidlna restart; sed -i 's/RESCAN=\"true\"/RESCAN=\"false\"/' /etc/conf.d/minidlna;");
	}
	else if (!strncmp(&path[0], "/ssh-restart\0", 12))
	{
		system("service sshd restart");
	}

	/* it's a simple request */
	if(i == req_len)
		return 0;

	i++;
	j = 0;

	/* parse http-string */
	while( http_req[i] != ' ' && http_req[i] != '\r' && (http_req[i] != '\0'))
	{
		if(j >= MAX_HTTPSTR_LEN-1)
			return -7;

		if(i == req_len)
			return -8;

		http[j++] = http_req[i++];
	}

	http[j] = '\0';
#if DLOG == 1	
	printf("HTTP-STR: %s\n",http);
#endif
	if((strcmp(http,"HTTP/1.0") != 0) && (strcmp(http,"HTTP/1.1") != 0))
		return -12;

	return 1;
}

static const char sendok[MAX_BUF_LEN] = "HTTP/1.0 200 OK\r\nServer: SimpleWebserver\r\nContent-Type: text/html\r\n\n";
static const char htmlBuf[MAX_BUF_LEN] = "<!doctypehtml><html lang=en><title>AlpineDLNA</title><meta content='width=device-width,initial-scale=1'name=viewport><style>ul{margin:0;padding:0}li{display:inline-block;margin:10px}a{text-decoration:none;color:initial;font-weight:700;border:1px solid gray;padding:5px 20px}</style><h2>AlpineDLNA</h2><ul><li><a href=/restart>ReStart</a><li><a href=/rescan>ReScan</a><li><a href=/rebuild>ReBuild</a></ul><h2>General</h2><ul><li><a href=/ssh>ReStart SSH</a><li><a href=/# onclick=frame()>Refresh iFrame</a></ul><br><script>var frame=function(){var e='nAlpineFrame';document.getElementById(e)&&document.body.removeChild(document.getElementById(e));var n=document.createElement('iframe');Object.assign(n,{id:e,frameBorder:0,src:'//'+window.location.hostname+':8200'}),Object.assign(n.style,{width:'100%','min-height':'750px'}),document.body.appendChild(n)};window.onload=function(){'/'!=window.location.pathname&&(window.location.pathname='/'),frame()}</script>";

int handle_client(int connfd)
{
	char buffer[MAX_BUF_LEN+1];
	int n = 0;
	int req_type = 0;

	/* recieve the http-request */
	n = recv(connfd,buffer,MAX_BUF_LEN,0);
	if(n <= 0)
	{
		perror("recv() error..");
		close(connfd);
		return 0;
	}
	buffer[n] = '\0';
#if DLOG == 1
	printf("HTTP-REQUEST: ..%s..\n",buffer);
#endif

	req_type = parse_first_http(buffer,n);
	if(req_type == 0)
	{
#if DLOG == 1		
		printf("it's a simple request\n");
#endif		
		send(connfd,sendok,strlen(sendok),0);
#if DLOG == 1		
		printf("HTTP-OK SENT\n");
#endif		
	}
	else if(req_type == 1)
	{
#if DLOG == 1		
		printf("it's a complex multiline request\n");
#endif		
		send(connfd,sendok,strlen(sendok),0);
#if DLOG == 1		
		printf("HTTP-OK SENT\n");
#endif		
	}
	else
	{
		fprintf(stderr,"parse_first_http() error: %d\n",req_type);
		close(connfd);
		return -1;
	}

	if(send(connfd, htmlBuf, strlen(htmlBuf),0) < 0)
		perror("send() error..");

	close(connfd);
	return 0;
}

/* listening port */
#define SERV_PORT 8202

/* maxium of allowed clients at the same time */
#define MAX_CLIENTS 10

/* counts our clients */
unsigned int clientcount;

/* maximum length for the queue of pending connections */

#define LISTENQ 1024

/*
   our child-handler. wait for all children to avoid zombies
 */
void sigchld_handler(int signo)
{
	int status;
	pid_t pid;

	/*
	   -1 means that we wait until the first process is terminated
	   WNOHANG tells the kernel not to block if there are no terminated
	   child-processes.
	 */
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		clientcount--;
#if DLOG == 1
		printf("%i exited with %i\n", pid, WEXITSTATUS(status));
#endif
	}

	return;
}

int main(int argc, char *argv[])
{
	int sockfd, connfd;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	struct sigaction sa; /* for wait-child-handler */

	clientcount = 0;

	char *server_port = getenv("SERVER_PORT");
	unsigned int port = atoi(server_port ? server_port : "null");
	port = port ? port : SERV_PORT;

	printf("starting server, port: %i, pid: %i\n", port, getpid());

	/* Create Server-Socket of type IP( IP: 0) */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("socket() error..");
		exit(EXIT_FAILURE);
	}

	/* Prepare our address-struct */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* bind server to all interfaces */
	servaddr.sin_port = htons(port);		/* bind server to port */

	// ReUsing time_wait state connections
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	/* bind sockt to address + port */
	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
	{
		perror("bind() failed..");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	/* start listening */
	if (listen(sockfd, LISTENQ) < 0)
	{
		perror("listen() error..");
		exit(EXIT_FAILURE);
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigchld_handler;
	sigaction(SIGCHLD, &sa, NULL);

	/* endless listening-loop
	   this loop waits for client-connections.
	   if a client is connectd it forks into another
	   subprocess.
	 */
	while (1)
	{
		if (clientcount < MAX_CLIENTS)
		{
			clilen = sizeof(cliaddr);
			/* accept connections from clients */
			connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
			if (connfd < 0)
			{
				/*
				   withouth this we would recieve:
					"accept() error..: Interrupted system call" after
					each client disconnect. this happens because SIGCHLD
					blocks our parent-accept().
				 */
				if (errno == EINTR)
					continue;

				perror("accept() error..");
				close(sockfd);
				exit(EXIT_FAILURE);
			}

			clientcount++;

			/* lets create a subprocess */
			childpid = fork();
			if (childpid < 0)
			{
				perror("fork() failed");
				exit(EXIT_FAILURE);
			}

			/* let's start our child-subprocess */
			if (childpid == 0)
			{
				close(sockfd);
				exit(handle_client(connfd));
			}

			/* continue our server-routine */
#if DLOG == 1
			printf("Client has PID %i\n", childpid);
#endif

			close(connfd);
		}
	}

	return EXIT_SUCCESS;
}
