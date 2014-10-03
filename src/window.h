
#ifndef WINDOW_H
#define WINDOW_H
#include "system.h"
#include "message.h"
#include "List.h"

class Window {
private: 
  UINT32 id;
  UINT32 parent_id;
  Window *parent;
  INT32 x;
  INT32 y;
  UINT32 width;
  UINT32 height;
  UINT32 event_mask;
  UINT32 do_not_propagate_mask;


  List<Window> children;
  
  static UINT32 root_window_id;
  static Window *root_window;
  static List<Window> global_list;
  

  void SetMasks(Message const &create_window_msg, UINT32 msg_offset);
  
public:

  static UINT32 RootID() { return root_window_id; };
  Window *Parent() { return parent; };

  UINT32 EventMask() { return event_mask; };
  UINT32 PropagateMask() { return do_not_propagate_mask; };

  UINT32 Max_x() { return width - 1; };
  UINT32 Max_y() { return height - 1; };
  UINT32 Root_x(UINT32 rel_x);
  UINT32 Root_y(UINT32 rel_y);

  static void DumpGlobal();
  Window(UINT32 root_id);
  Window(Message const &create_window_msg);
  ~Window();
  void ChangeWindowAttributes(Message const &change_window_attributes_msg);
  void ConfigureWindow(Message const &configure_window_msg);
  void ReparentWindow(Message const &reparent_window_msg);
  static Window *FindWindow(UINT32 wid, Window *win = root_window);
  static void Dump(Window *win = root_window);
  static void DeleteAll();

  // *** These routines let you step through all active windows ***
  // (The current window is undefined after any call to ~Window)
  static UINT32 Count();
  static Window *Number(UINT32 index);

  // *** These routines let you look at window attributes ***
  int IsRoot();
  int IsChild();
  UINT32 ID();
  UINT32 Width();
  UINT32 Height();

/*
  // Grab/Ungrab Button/Key;
  void addChild(Window *child) { children.Append(child); };
  void removeChild(Window *child) { children.Delete(child); };
  void Initialize() { children.Rewind(); };
  Window *getNextChild() { return children.Next(); };
  Window *getPreviousChild() { return children.Previous(); };
  void PutFocus();
*/
};

#endif
