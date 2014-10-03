
#include "system.h"
#include "socket.h"
#include "message.h"
#include "New.h"

// *** Initialize Message static variables ***

int Message::byte_order = -1;  // -1 = undefined
#ifdef DEBUG
UINT32 Message::object_count = 0;
#endif

#ifdef DEBUG
#define BUMP_COUNT \
   object_count++; \
   if (object_count > 5) { \
      fprintf(stderr, "More than 5 Messages have been created\n"); \
      DumpMem(); \
   }
#define DEC_COUNT object_count--
#else
#define BUMP_COUNT NULL
#define DEC_COUNT NULL
#endif

Message::Message(UINT32 size) {

  BUMP_COUNT;

  if (size > MAX_MESSAGE_SIZE) {
    fprintf(stderr, "Requested message size too large\n");
    terminate();
  };
  New(data, UINT8[size]);
  if (!data) {
    fprintf(stderr, "Message buffer not allocated\n");
    terminate();
  };
  max_size = size;
  this->size = 0;
  size_lock = 1;
};

Message::Message(Message const &msg, UINT32 index, UINT32 length,
	UINT32 size) {

  BUMP_COUNT;

  if (size > MAX_MESSAGE_SIZE) {
    fprintf(stderr, "Requested message size too large(2)\n");
    terminate();
  };
  if (length > size) {
    fprintf(stderr, "Can not contstruct message with length = %u and size = %u\n", (unsigned int)length, (unsigned int)size);
    terminate();
  };
  New(data, UINT8[size]);
  if (!data) {
    fprintf(stderr, "Message buffer not allocated\n");
    terminate();
  };
  max_size = size;
  this->size = length;
  size_lock = 1;

//  printf("&msg.data[index] = %p, data = %p, length = %u\n", &msg.data[index],
//	 data, (unsigned int)length);

  bcopy( (char *)&msg.data[index], (char *)data, length);

//  fprintf(stderr, "M:M #7\n");

};

Message::~Message() {

  DEC_COUNT;

//  delete [] data;
  DELETE_ARRAY(data);
};

#undef BUMP_COUNT
#undef DEC_COUNT

Message const &Message::operator=(Message const &msg) {

/*
  delete this;
  this = new Message(msg, 0, msg.size);
*/
  if (max_size < msg.size) {
    fprintf(stderr, "Can not assign message - buffer too small.\n");
    terminate();
  };
  bcopy( (char *)msg.data, (char *)data, msg.size);
  size = msg.size;
  return *this;
};


void Message::SetSizeLock(int flag) {

  size_lock = flag;
};

void Message::SetSize(UINT32 size) {

  if (size > max_size) {
    fprintf(stderr, "Can not make message size larger than max_size\n");
    terminate();
  };
  this->size = size;
};

void Message::Dump(FILE *f, int columns) const {
UINT32 offset, i;

   for (offset = 0; offset < size; offset += columns) {
      fprintf(f, "%lu: ", offset);
      for (i = offset; i < size && i < offset + columns; i++) {
         fprintf(f, "%u ", (unsigned int)(data[i]));
      };
      fprintf(f, "\n");
   };
};

int Message::Receive(Socket const &from) {

//  printf("from.Handle() = %u, &from = %p, data = %p, max_size = %u\n",
//	 (unsigned int)from.Handle(),
//	 &from,
//	 data,
//	 (unsigned int)max_size);

  size = recv(from.Handle(), (char *)data, max_size, 0);
  if (size == -1) {
    if (errno == ECONNRESET) {
      return -1;
    };
    fprintf(stderr, "Error %i on recv()\n", errno);
    terminate();
  };
  if (size >= max_size - 2) {
    fprintf(stderr, "Message buffer overflow, size = %u, max_size = %u\n",
	    (unsigned int)size,
	    (unsigned int)max_size);
    terminate();
  };
  return 0;
};
  
void Message::Send(Socket const &to) const {

  if (send(to.Handle(), data, size, 0) == -1) {
    fprintf(stderr, "Error %i on send(X)\n", errno);
    terminate();
  };
};

UINT8 Message::operator[](UINT32 index) const {

  return data[index];
};

void Message::Modify(UINT32 position, UINT32 length, UINT8 const *source) {

  if (position + 1 > max_size || (position + length) > max_size) {
    fprintf(stderr, "Bad call to Message::Modify()\n");
    terminate();
  };
  if (size_lock) {
    if (position + 1 > size || (position + length) > size) {
      fprintf(stderr, "Bad call to Message::Modify()\n");
      terminate();
    };
  } else {
    if (position + length > size) {
      size = position + length;
    };
  };

  bcopy((char *)source, (char *)&data[position], length);
};

/********* Non-Member Routines ************/

void SetByteOrder(UINT8 code) {
  int flag = Message::byte_order;
  
  switch (code) {
  case 'B':   // Big Endian (MSB first)
    Message::byte_order = 0;
    flag += 1;
    break;
    
  case 'l':   // little endian (LSB first)
    Message::byte_order = 1;
    flag += 1;
    break;
  };
  
  if (flag > 0) {
    fprintf(stderr, "Warning: Client sent connection setup message more than once\n");
    if (flag - 1 != Message::byte_order) {
      fprintf(stderr, "Warning: Client changed byte order\n");
    };
  };
};

void Check_Byte_Order() {
  
  if (Message::byte_order == -1) {
    fprintf(stderr, "Error: Client did not send connection setup message as first message\n");
    terminate();
  };
};

char *ByteOrder() {

  switch (Message::byte_order) {
  case 0:
    return "MSB First";
    break;
  case 1:
    return "LSB First";
    break;
  default:
    return "Undefined";
  };
};

UINT16 GetUINT16(Message const &msg, UINT32 index) {
  
  Check_Byte_Order();
  return (UINT16)(msg[index + Message::byte_order]) * 0x100 +
    (UINT16)(msg[index + 1 - Message::byte_order]);
};

UINT32 GetUINT32(Message const &msg, UINT32 index) {
  
  Check_Byte_Order();
  return (UINT32)(msg[index + 3 * Message::byte_order]) * 0x1000000 +
    (UINT32)(msg[index + 1 + Message::byte_order]) * 0x10000 +
      (UINT32)(msg[index + 2 - Message::byte_order]) * 0x100 +
	(UINT32)(msg[index + 3 - 3 * Message::byte_order]);
};

void PutUINT8(Message &msg, UINT32 index, UINT8 value) {

  Check_Byte_Order();
  msg.Modify(index, 1, &value);
};


void PutUINT16(Message &msg, UINT32 index, UINT16 value) {

  Check_Byte_Order();
  PutUINT8(msg, index + Message::byte_order, value >> 8);
  PutUINT8(msg, index + 1 - Message::byte_order, value & 0xFF);
};

void PutUINT32(Message &msg, UINT32 index, UINT32 value) {

  Check_Byte_Order();
  PutUINT8(msg, index + 3 * Message::byte_order, value >> 24);
  PutUINT8(msg, index + 1 + Message::byte_order, (value >> 16) & 0xFF);
  PutUINT8(msg, index + 2 - Message::byte_order, (value >> 8) & 0xFF);
  PutUINT8(msg, index + 3 - 3 * Message::byte_order, value & 0xFF);
};


