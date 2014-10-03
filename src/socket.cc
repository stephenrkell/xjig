
#include "system.h"
#include "socket.h"
#include "New.h"

Socket::Socket(char *host_name, int port_number) {

  hostent *hp = gethostbyname(host_name);

  if (!hp) {
    fprintf(stderr, "Error getting host name for port %i\n", port_number);
    terminate();
  };

  if (hp->h_length > sizeof(in_addr)) {
    printf("Host address is too large\n");
    terminate();
  };

  bzero((char *) &address, sizeof(address));
// used to be:
//  address.sin_addr.s_addr = inet_addr(hp->h_addr_list[0]);
// which is wrong since hp->h_addr_list[0] is not in dotted
// form, but is instead just a sequence of control characters
// it should be:
  memcpy(&address.sin_addr.s_addr, hp->h_addr_list[0],
	 sizeof(address.sin_addr.s_addr));

  address.sin_family = hp->h_addrtype;
  address.sin_port = htons(port_number);
  
  handle = socket(PF_INET, SOCK_STREAM, 0);
  if (handle == -1) {
    fprintf(stderr, "Error creating socket for host %s, port %i\n",
	    host_name, port_number);
    terminate();
  };

  unsigned int temp = 8192;

  if (setsockopt(handle,       // what socket
		 SOL_SOCKET,    // option level
		 SO_SNDBUF,      // change send buffer size
		 (char *)(&temp),   // new size ??
		 sizeof(unsigned int) // size of argument ??
    )) {
    fprintf(stderr, "Error %i on setsockopt():1\n", errno);
    terminate();
  };

  if (setsockopt(handle,       // what socket
		 SOL_SOCKET,    // option level
		 SO_RCVBUF,      // change send buffer size
		 (char *)(&temp),   // new size ??
		 sizeof(unsigned int) // size of argument ??
    )) {
    fprintf(stderr, "Error %i on setsockopt():2\n", errno);
    terminate();
  };

};


static int hack_flag = 0;

Socket::Socket(Socket const &s) {


  if (!hack_flag) {
    
    hack_flag = 1;
    
    // bind should probably be unlink'ed! (See man page)
    
    if (bind(s.handle, (struct sockaddr *)&s.address, sizeof(s.address))) {
      fprintf(stderr, "Error %i on bind()\n", errno);
    terminate();
    };
    
    
    // *** Listen for a connection ***
    
    if (listen(s.handle, 5) == -1) {
      fprintf(stderr, "Error %i on listen()\n", errno);
      terminate();
    };
  };  

  // *** Accept a connect ***
  
  bcopy((char *)&s.address, (char *)&address, sizeof(address));
  int addr_size = sizeof(address);
  
  handle = accept(s.handle, (sockaddr *)&address, &addr_size);
  if (handle == -1) {
    fprintf(stderr, "Error %i on accept()\n", errno);
    terminate();
  };

  unsigned int temp = 8192;

  if (setsockopt(handle,       // what socket
		 SOL_SOCKET,    // option level
		 SO_SNDBUF,      // change send buffer size
		 (char *)(&temp),   // new size ??
		 sizeof(unsigned int) // size of argument ??
    )) {
    fprintf(stderr, "Error %i on setsockopt():3\n", errno);
    terminate();
  };

  if (setsockopt(handle,       // what socket
		 SOL_SOCKET,    // option level
		 SO_RCVBUF,      // change send buffer size
		 (char *)(&temp),   // new size ??
		 sizeof(unsigned int) // size of argument ??
    )) {
    fprintf(stderr, "Error %i on setsockopt():4\n", errno);
    terminate();
  };
};

void Socket::Connect() {
  
  if (connect(handle, (sockaddr *)&address, sizeof(address)) == -1) {
    fprintf(stderr, "Error %i on connect()\n", errno);
    terminate();
  };
};
