// It is from Stevens's UNIX Network Programming book, had to drug in a bunch of wrappers to the actual snippet to work.

//
// ./run.sh
// $ ./a.out 129.6.15.28                 <---------- time-a.nist.gov , NIST, Gaithersburg, Maryland
// 57978 17-08-13 07:54:37 50 0 0 645.9 UTC(NIST) *
// 
// $ ./a.out 129.6.15.29      <--- should work too, but does not. :-\
// 
// $ ./a.out 129.6.15.30
// 57978 17-08-13 08:00:20 50 0 0 779.5 UTC(NIST) *
// 
// http://tf.nist.gov/tf-cgi/servers.cgi# Says the requests should NEVER be
// more frequent than 4 seconds apart, or it will be treated as DoS attack.
//

// Port 37 responds with seconds since 00:00 (midnight) 1 January 1900 GMT, such that
// the time 1 is 12:00:01 am on 1 January 1900 GMT; this base will serve until the year 2036.
// https://tools.ietf.org/html/rfc868

// $ ./client.out 127.0.0.1
// got 26 bytes: 53 75 6e 20 41 75 67 20 31 33 20 30 34 3a 33 37 3a 32 32 20 32 30 31 37 0d 0a 
// Sun Aug 13 04:37:22 2017
//
// Sun Aug 13 04:37:22 2017
//


#include	<sys/types.h>	/* basic system data types */
#include	<sys/socket.h>	/* basic socket definitions */
#include	<sys/time.h>	/* timeval{} for select() */
#include	<time.h>		/* timespec{} for pselect() */
#include	<netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<fcntl.h>		/* for nonblocking */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	/* for S_xxx file mode constants */
#include	<sys/uio.h>		/* for iovec{} and readv/writev */
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		/* for Unix domain sockets */
#include	<stdarg.h>		/* ANSI C header file */
#include	<syslog.h>		/* for syslog() */


#define	MAXLINE		4096	/* max text line length */
#define	MAXSOCKADDR  128	/* max socket address structure size */
#define	BUFFSIZE	8192	/* buffer size for reads and writes */

#define	SA	struct sockaddr


int		daemon_proc;		/* set nonzero by daemon_init() */
static void	err_doit(int, int, const char *, va_list);

/* Fatal error unrelated to a system call.
 * Print a message and terminate. */

void
err_quit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Fatal error related to a system call.
 * Print a message and terminate. */

void
err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Print a message and return to caller.
 * Caller specifies "errnoflag" and "level". */

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
	int		errno_save, n;
	char	buf[MAXLINE];

	errno_save = errno;		/* value caller might want printed */
#ifdef	HAVE_VSNPRINTF
	vsnprintf(buf, sizeof(buf), fmt, ap);	/* this is safe */
#else
	vsprintf(buf, fmt, ap);					/* this is not safe */
#endif
	n = strlen(buf);
	if (errnoflag)
		snprintf(buf+n, sizeof(buf)-n, ": %s", strerror(errno_save));
	strcat(buf, "\n");

	if (daemon_proc) {
		syslog(level, "%s", buf);
	} else {
		fflush(stdout);		/* in case stdout and stderr are the same */
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;
}

void
Close(int fd)
{
	if (close(fd) == -1)
		err_sys("close error");
}

void *
Malloc(size_t size)
{
	void	*ptr;

	if ( (ptr = malloc(size)) == NULL)
		err_sys("malloc error");
	return(ptr);
}

int
tcp_connect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("tcp_connect error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;	/* ignore this one */

		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		Close(sockfd);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno set from final connect() */
		err_sys("tcp_connect error for %s, %s", host, serv);

	freeaddrinfo(ressave);

	return(sockfd);
}
/* end tcp_connect */

/*
 * We place the wrapper function here, not in wraplib.c, because some
 * XTI programs need to include wraplib.c, and it also defines
 * a Tcp_connect() function.
 */

int
Tcp_connect(const char *host, const char *serv)
{
	return(tcp_connect(host, serv));
}

void
Getpeername(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	if (getpeername(fd, sa, salenptr) < 0)
		err_sys("getpeername error");
}

ssize_t
Recv(int fd, void *ptr, size_t nbytes, int flags)
{
	ssize_t		n;

	if ( (n = recv(fd, ptr, nbytes, flags)) < 0)
		err_sys("recv error");
	return(n);
}

ssize_t
Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t		n;

	if ( (n = read(fd, ptr, nbytes)) == -1)
		err_sys("read error");
	return(n);
}

void
Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		err_sys("fputs error");
}


#ifdef	HAVE_SOCKADDR_DL_STRUCT
# include	<net/if_dl.h>
#endif

char *
sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
			return(NULL);
		return(str);
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		struct sockaddr_un	*unp = (struct sockaddr_un *) sa;

			/* OK to have no pathname bound to the socket: happens on
			   every connect() unless client calls bind() first. */
		if (unp->sun_path[0] == 0)
			strcpy(str, "(no pathname bound)");
		else
			snprintf(str, sizeof(str), "%s", unp->sun_path);
		return(str);
	}
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
	case AF_LINK: {
		struct sockaddr_dl	*sdl = (struct sockaddr_dl *) sa;

		if (sdl->sdl_nlen > 0)
			snprintf(str, sizeof(str), "%*s",
					 sdl->sdl_nlen, &sdl->sdl_data[0]);
		else
			snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
		return(str);
	}
#endif
	default:
		snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
	char	*ptr;

	if ( (ptr = sock_ntop_host(sa, salen)) == NULL)
		err_sys("sock_ntop_host error");	/* inet_ntop() sets errno */
	return(ptr);
}


/*
int main(int argc, char **argv)
{
	int		sockfd, n, npend;
	char	recvline[MAXLINE + 1];
	socklen_t	len;
	struct sockaddr	*sa;

	if (argc != 3) {
		err_quit("usage: a.out <hostname or IPaddress> <service or port#>");
	}

	sockfd = Tcp_connect(argv[1], argv[2]);

	sa = (sockaddr	*)Malloc(MAXSOCKADDR);
	len = MAXSOCKADDR;
	Getpeername(sockfd, sa, &len);
	printf("connected to %s\n", Sock_ntop_host(sa, len));

	for ( ; ; ) {
		if ( (n = Recv(sockfd, recvline, MAXLINE, MSG_PEEK)) == 0)
			break;		// server closed connection

		//Ioctl(sockfd, FIONREAD, &npend);	// check FIONREAD support
		//printf("%d bytes from PEEK, %d bytes pending\n", n, npend);

		n = Read(sockfd, recvline, MAXLINE);
		recvline[n] = 0;	// null terminate 
		Fputs(recvline, stdout);
	}
	exit(0);
}
*/

int
main(int argc, char **argv)
{
	int					sockfd, n;
	char				recvline[MAXLINE + 1];
	struct sockaddr_in	servaddr;

	if (argc != 2)
		err_quit("usage: %s <IPaddress>", argv[0]);

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys("socket error");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	
	servaddr.sin_port   = htons(13);	// daytime server 
	//servaddr.sin_port   = htons(37);	// time server    32 bits of data
    	
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
		err_quit("inet_pton error for %s", argv[1]);

	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
		err_sys("connect error");

	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	// null terminate
		
//		printf("got %d bytes: ", n);
//		for(int i=0;i<n;i++)
//		printf("%02x ", (unsigned char)recvline[i]);
//		printf("\n");
//		for(int i=0;i<n;i++)
//		printf("%c", (unsigned char)recvline[i]);
//		printf("\n");
		
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
	if (n < 0)
		err_sys("read error");

	exit(0);
}
