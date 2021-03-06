
#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "socket.h"

const UINT32 MAX_MESSAGE_SIZE = 3000000;
#ifdef __linux__
const int RECV_FLAGS = 0;
#else
const int RECV_FLAGS = 0;
#endif
class Message;

inline void ResetByteOrder();
void SetByteOrder(UINT8 code);
void Check_Byte_Order();
const char *ByteOrder();  // Returns "MSB First", "LSB First" or "Undefined"
UINT16 GetUINT16(Message const &msg, UINT32 index);
UINT32 GetUINT32(Message const &msg, UINT32 index);
void PutUINT8(Message &msg, UINT32 index, UINT8 value);
void PutUINT16(Message &msg, UINT32 index, UINT16 value);
void PutUINT32(Message &msg, UINT32 index, UINT32 value);

class Message {
public:
  Message(UINT32 size = MAX_MESSAGE_SIZE);
  Message(Message const &msg, UINT32 index, UINT32 length,
	  UINT32 size = MAX_MESSAGE_SIZE);
  ~Message();
  Message const &operator=(Message const &msg);
  void SetSizeLock(int flag);
  void SetSize(UINT32 size);
  void Dump(FILE *f = stdout, int columns = 40) const;

  int Receive(Socket const &s);
  int ExtendTo(Socket const &s, size_t requested_len);
  void Send(Socket const &s) const;
  UINT32 Size() const { return size; };
  UINT8 operator[](UINT32 index) const;
  void Modify(UINT32 position, UINT32 length, UINT8 const *source);
protected:
#ifdef DEBUG
  static UINT32 object_count;
#endif
  UINT8 *data;
  UINT8 *pos;
  UINT32 size;
  UINT32 max_size;
  int size_lock;
  friend void ResetByteOrder();
  friend void SetByteOrder(UINT8 code);
  friend void Check_Byte_Order();
  friend const char *ByteOrder();  // Returns "MSB First", "LSB First" or "Undefined"
  friend UINT16 GetUINT16(Message const &msg, UINT32 index);
  friend UINT32 GetUINT32(Message const &msg, UINT32 index);
  friend void PutUINT8(Message &msg, UINT32 index, UINT8 value);
  friend void PutUINT16(Message &msg, UINT32 index, UINT16 value);
  friend void PutUINT32(Message &msg, UINT32 index, UINT32 value);

  static int byte_order;  // -1 = undefined, 0 = MSB first, 1 = LSB first

  Message(Message const&msg) {fprintf(stderr, "Message(Message)\n"); exit(1);};
};

inline void ResetByteOrder() { Message::byte_order = -1; }

#endif
