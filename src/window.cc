
#include "system.h"
#include "message.h"
#include "List.cc"
#include "window.h"

// *** Initialize Window static variables ***

UINT32 Window::root_window_id; // = 42;
Window *Window::root_window = 0;
List<Window> Window::global_list;

void Window::DumpGlobal() {
  printf("Global list (%u):", (unsigned int)global_list.Size());
  global_list.Rewind();
  while (global_list.Next()) {
    printf(" %p", global_list.Current());
  };
  printf("\n");
};

Window::Window(UINT32 root_id) {

  if (root_window) {
    fprintf(stderr, "Root window redefined\n");
    terminate();
  };
  id = root_id;
  root_window_id = root_id;
  root_window = this;
  parent = 0;
  global_list.Append(this);
};

Window::Window(Message const &create_window_msg) {

  if (!root_window) {
    fprintf(stderr, "Root window not yet defined\n");
    terminate();
  };

  id = GetUINT32(create_window_msg, 4);
  parent_id = GetUINT32(create_window_msg, 8);
  parent = FindWindow(parent_id);
#ifdef WIN_STOP
  if (!parent) {
    fprintf(stderr, "xPROG specified non-existing parent window id: %u\n",
	    (unsigned int)parent_id);
    terminate();
  };
#else
  parent = root_window;
#endif
  parent->children.Append(this);
  global_list.Append(this);

  x = GetUINT16(create_window_msg, 12);
  y = GetUINT16(create_window_msg, 14);
  width = GetUINT16(create_window_msg, 16);
  height = GetUINT16(create_window_msg, 18);
  SetMasks(create_window_msg, 28);
};

Window::~Window() {

  global_list.Rewind();
  if (global_list.Seek(this)) {
    global_list.Remove();
  } else {
    fprintf(stderr, "Bad window pointer in ~Window()\n");
    terminate();
  };
};

void
Window::ChangeWindowAttributes(Message const &change_window_attributes_msg) {

  SetMasks(change_window_attributes_msg, 8);
};

void Window::ConfigureWindow(Message const &configure_window_msg) {

  UINT32 i;
  UINT32 value_mask;
  UINT32 msg_offset = 12;

  value_mask = GetUINT16(configure_window_msg, 8);

  for (i = 0; i <= 3; i++) {
    if (value_mask & 1) {
      switch (i) {
      case 0:
	x = GetUINT32(configure_window_msg, msg_offset);
	break;
      case 1:
	y = GetUINT32(configure_window_msg, msg_offset);
	break;
      case 2:
	width = GetUINT32(configure_window_msg, msg_offset);
	break;
      case 3:
	height = GetUINT32(configure_window_msg, msg_offset);
	break;
      };
      msg_offset += 4;
    };
    value_mask >>= 1;
  };
};

void Window::ReparentWindow(Message const &reparent_window_msg) {

#ifdef WIN_STOP
  if (!parent) {
    fprintf(stderr, "Can't reparent window without parent\n");
    terminate();
  };
#endif
  Window *new_parent = FindWindow(GetUINT16(reparent_window_msg, 8));
  x = GetUINT16(reparent_window_msg, 12);
  y = GetUINT16(reparent_window_msg, 14);
  if (parent) {
    parent->children.Rewind();
    parent->children.Seek(this);
    parent->children.Remove();
  };
  new_parent->children.Append(this);
};

Window *Window::FindWindow(UINT32 wid, Window *win) {
  Window *temp;

  if (win->id == wid) {
    return win;
  };

  win->children.Rewind();
  while (win->children.Next()) {
    if (temp = FindWindow(wid, win->children.Current())) {
      return temp;
    };
  };

  return 0; // No match found
};

void Window::Dump(Window *win) {

  printf("Window %u, Chilren: ", (unsigned int)win->id);
  win->children.Rewind();
  while (win->children.Next()) {
    printf("%u ", (unsigned int)win->children.Current()->id);
  };
  printf("\n");

  win->children.Rewind();
  while (win->children.Next()) {
    win->Dump(win->children.Current());
  };
};

void Window::DeleteAll() {

  if (root_window) {
    DELETE(root_window);
    root_window = 0;
  };
};

void Window::SetMasks(Message const &create_window_msg, UINT32 msg_offset) {

  UINT32 i;
  UINT32 value_mask;

  value_mask = GetUINT32(create_window_msg, msg_offset);
  msg_offset += 4;

  for (i = 0; i <= 12; i++) {
    if (value_mask & 1) {
      if (i == 11) {
	event_mask = GetUINT32(create_window_msg, msg_offset);
      } else if (i == 12) {
	do_not_propagate_mask = GetUINT32(create_window_msg, msg_offset);
      };
      msg_offset += 4;
    };
    value_mask >>= 1;
  };
};

// *** These routines let you step through all active windows ***
UINT32 Window::Count() {

  return global_list.Size();
};

Window *Window::Number(UINT32 index) {

  global_list.SetPosition(index);
  return global_list.Current();
};

// *** These routines let you look at window attributes ***
int Window::IsRoot() {

  return this == root_window && root_window != 0;
};

int Window::IsChild() {

  return children.Empty();
};

UINT32 Window::ID() {

  return id;
};

UINT32 Window::Width(){

  return width;
};

UINT32 Window::Height() {

  return height;
};

UINT32 Window::Root_x(UINT32 rel_x) {

  if (this == root_window) {
    return rel_x;
  };

  return rel_x + parent->Root_x(x);
};

UINT32 Window::Root_y(UINT32 rel_y) {

  if (this == root_window) {
    return rel_y;
  };

  return rel_y + parent->Root_y(y);
};
