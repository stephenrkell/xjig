
#ifndef _SOCKET_H
#define _SOCKET_H

class Socket {
public:
  Socket(char *host_name, int port_number);
  Socket(Socket const &s);
  ~Socket() { close(handle); };
  void Connect();
  sockaddr_in Address() const { return address; };
  int Handle() const { return handle; };
private:
  int handle;
  struct sockaddr_in address;
};

#endif
