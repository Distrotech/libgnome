/* ObLICENSE:
   You are free to do anything you want with this program.

   It is requested that you keep the format of data sent
   between client and server the same, and
   that you install this program as dns-helper
   (and under no other name).

   Just an attempt to make life easy for people.
   This is called just plain "dns-helper" so other programs
   will feel welcome to use it too...
*/

/* Note:
   I have no clue how to make this integrate well with
   a mixed IPv4/IPv6 environment. May have to create a more
   complex helper <-> main program communication protocol

   Hmm, brainstorm, use fd's 3 & 4 for that perhaps.
   (don't use 2 - we'll use that in the future to see
   if someone is running this from the cmdline or something
   (is there a way to check if an FD is valid?)
*/

#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#if defined(HAVE_GETDTABLESIZE)
#define GET_MAX_FDS() getdtablesize()
#else
/* Fallthrough case - please add other #elif cases above
   for different OS's as necessary */
#define GET_MAX_FDS() 256
#endif

int main(int argc, char *argv[])
{
  char hostname[4096];
  int nread;
  struct hostent *host;
  int fd, max_fd;
  struct in_addr ip_addr;
	
  /* close all file descriptors except ones used by the pipes (0 and 1). */
  max_fd = GET_MAX_FDS();
  for(fd = 2 /* The first fd after the pipes */; fd < max_fd; fd++)
    close(fd);
	
  while(1) {
    /* block on read from client */
    nread = read(0, hostname, 4095);
		
    /* will return 0 if parent exits. */

    if (nread == 0)
      _exit(0);
		
    if (nread < 0)
      _exit(1);
		
    hostname[nread] = '\0';
    host = gethostbyname(hostname);
		
    if (host == NULL)
      ip_addr.s_addr = 0;
    else
      {
	memcpy(&ip_addr, host->h_addr_list[0], sizeof(ip_addr));
	ip_addr.s_addr = ntohl(ip_addr.s_addr);
      }
		
    /* write hostname to client */
    if(write(1, &ip_addr, sizeof(ip_addr)) < 0)
      _exit(2);
  }
  _exit(0);
}
