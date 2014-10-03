
#include "system.h"
#include "message.h"
#include "event.h"
#include "New.h"

// *** Initialize Event static variables ***

#ifdef DEBUG
UINT32 Event::object_count = 0;
#endif


Button::Button(UINT32 button) {

  if (button < 1 || button > 5) {
    fprintf(stderr, "Bad button detected in ButtonToggle()\n");
    exit(1);
  };
  value = button;
};

Key::Key(UINT32 key) {

  if (key < 8 || key > 255) {
    fprintf(stderr, "Invalid Key() argument\n");
    exit(1);
  };
  value = key;
};

EventContext::EventContext() {
  UINT32 i;

  for (i = 8; i <= 255; i++) {
    key_mask[i] = 0;
  };
  mask = 0;

  New(motion_buffer, Fifo<Motion>(1000));
};

void EventContext::SetDefault(UINT32 index, UINT8 value) {

  if (index > 31) {
    fprintf(stderr, "Bad index sent to EventContext::SetDefault()\n");
    exit(1);
  };
  event_default[index] = value;
};

void Event::CopyDefault(EventContext &ec) {
  UINT32 i;

  SetSize(32);

  for (i = 0; i <= 31; i++) {
    PutUINT8(*this, i, ec.event_default[i]);
  };

  PutUINT32(*this, 4, GetUINT32(*this, 4) + 1); /* Increment TIMESTAMP */

  for (i = 4; i <= 7; i++) {    /* Save new TIMESTAMP */
    ec.event_default[i] = (*this)[i];
  };
};

#ifdef DEBUG
#define BUMP_COUNT \
   object_count++; \
   if (object_count > 1005) { \
      fprintf(stderr, "More than 1005 Events have been created\n"); \
      DumpMem(); \
   }
#define DEC_COUNT object_count--
#else
#define BUMP_COUNT NULL
#define DEC_COUNT NULL
#endif

Event::Event(EventContext &ec, Key const &key_in) {
  const UINT32 key = key_in;
  UINT32 code;
  UINT32 bit;

  BUMP_COUNT;

  if (ec.key_mask[key]) {
    code = 3;
  } else {
    code = 2;
  };
  switch (key) {
  case 106: /* left-shift */
  case 117: /* right-shift */
    bit = 1;
    break;
  case 126: /* caps lock */
    if (code == 2) { /* only press of caps lock interesting */
      bit = 2;
    } else {
      bit = 0;
    };
    break;
  case 83: /* cntrl */
    bit = 4;
    break;
  case 127: /* left-mod1 (meta) */
  case 129: /* right-mod1 */
    bit = 8;
    break;
  case 20: /* mod2 (Alt Graph) */
    bit = 16;
    break;
  case 26: /* mod3 (Alt) */
    bit = 32;
    break;
    /* no known mod4 or mod5 */
  default:
    bit = 0;
  };
  CopyDefault(ec);
//  ec.mask = 0; // ***************
  PutUINT8(*this, 0, code);
  PutUINT8(*this, 1, key);
  ec.key_mask[key] = !ec.key_mask[key];
  ec.mask = ec.mask ^ bit;
};

Event::Event(EventContext &ec, Button const &button_in) {
  const UINT32 button = button_in;
  UINT32 code;

  BUMP_COUNT;

  const UINT32 bit = 0x80 << button;
  if (bit & ec.mask) {
    code = 5;
  } else {
    code = 4;
  };
  CopyDefault(ec);
  PutUINT8(*this, 0, code);
  PutUINT8(*this, 1, button);
  ec.mask = ec.mask ^ bit;
};

Event::Event(EventContext &ec, Motion const &motion) {

  BUMP_COUNT;

  // *** Notify program that motions have occurred ***
  CopyDefault(ec);
  PutUINT8(*this, 0, 6); // 6 = MotionNotify

  // 0 = Normal, 1 = Hint; Use 1 some time in the future
  PutUINT8(*this, 1, 0);

  // *** Add motion to event context's motion buffer ***
  Motion *temp;
  New(temp, Motion(GetUINT32(*this, 4),
		   motion.X(),
		   motion.Y()
		   )
      );
  ec.motion_buffer->Add(temp);
};

Event::Event(EventContext &ec, UINT32 seq_num, UINT32 start, UINT32 stop) {

  BUMP_COUNT;

  Motion *motion;
  UINT32 count = 0;

  if (!start) start = MAX_UINT32;
  if (!stop) stop = MAX_UINT32;

  ec.motion_buffer->Rewind();
  while (motion = ec.motion_buffer->Next()) {
    if (motion->Time() >= start && motion->Time() <= stop) {
      count++;
    };
  };

  DELETE_ARRAY(data);
  max_size = size = count * 8 + 32;
  New(data, UINT8[size]);

  PutUINT8(*this, 0, 1); // Reply Code
  PutUINT16(*this, 2, seq_num);
  PutUINT32(*this, 4, count * 2);
  PutUINT32(*this, 8, count);

  count = 0;
  ec.motion_buffer->Rewind();
  while (motion = ec.motion_buffer->Next()) {
    if (motion->Time() >= start && motion->Time() <= stop) {
      PutUINT32(*this, 32 + count * 8, motion->Time());
      PutUINT16(*this, 32 + count * 8 + 4, motion->X());
      PutUINT16(*this, 32 + count * 8 + 6, motion->Y());
      count++;
    };
  };
};

Event::Event(EventContext &ec, UINT32 seq_num) {

  BUMP_COUNT;

  CopyDefault(ec);
  PutUINT8(*this, 0, 1);
  PutUINT8(*this, 1, 1); // same-screen
  PutUINT16(*this, 2, seq_num);
  PutUINT32(*this, 4, 0);
  for (int i = 12; i <= 25; i++) {
    PutUINT8(*this, i, (*this)[i + 4]);
  };
  if (ec.motion_buffer->IsEmpty()) {
    PutUINT16(*this, 20, 0);
    PutUINT16(*this, 22, 0);
  } else {
    ec.motion_buffer->Reverse();
    Motion *m = ec.motion_buffer->Next();
    PutUINT16(*this, 20, m->X());
    PutUINT16(*this, 22, m->Y());
  };
};

#ifdef DEBUG
Event::~Event() {

  DEC_COUNT;
};
#endif

#undef BUMP_COUNT
#undef DEC_COUNT
