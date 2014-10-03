
#ifndef _EVENT_H
#define _EVENT_H

#include "system.h"
#include "message.h"
#include "Fifo.cc"

class Button {
public:
  Button(UINT32 button);
  operator UINT32() const { return value; };
private:
  UINT32 value;
};

class Key {
public:
  Key(UINT32 key);
  operator UINT32() const { return value; };
private:
  UINT32 value;
};

class Motion {
public:
  Motion(UINT32 time, UINT32 x, UINT32 y) {
    this->time = time;
    this->x = (UINT16)x;
    this->y = (UINT16)y;
  };
  UINT32 Time() const { return time; };
  UINT32 X() const { return (UINT32)x; };
  UINT32 Y() const { return (UINT32)y; };
private:
  UINT32 time;
  UINT16 x;
  UINT16 y;
};

class EventContext {
public:
  EventContext();
  void SetDefault(UINT32 index, UINT8 value);
private:
  EventContext(EventContext const &ec) {
    fprintf(stderr, "EventContext(EventContext) does not exist\n");
    exit(1);
  };
  int MotionNotify();
  UINT8 key_mask[256];
  UINT16 mask;

  Fifo<Motion> *motion_buffer;

  UINT8 event_default[32]; /* This should go away eventually */
  friend class Event;
};

class Event : public Message {
public:
  Event(EventContext &ec, Key const &key);
  Event(EventContext &ec, Button const &button);
  Event(EventContext &ec, Motion const &motion);
  Event(EventContext &ec, UINT32 seq_num, UINT32 start, UINT32 stop);
  Event(EventContext &ec, UINT32 seq_num);
#ifdef DEBUG
  ~Event();
#endif
private:
#ifdef DEBUG
  static UINT32 object_count;
#endif
  void CopyDefault(EventContext &ec);
  Event(Event const &event) {
    fprintf(stderr, "Event(Event) does not exist\n");
    exit(1);
  };
};
  
#endif
