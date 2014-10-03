
#ifndef _List_H
#define _List_H

#include "system.h"

template <class T> class List;

template <class T>
class ListElement {
private:
  T *object;
  ListElement *prev;
  ListElement *next;
  friend class List<T>;
};

template <class T>
class List {
public:
  List();

  ~List();
  //* ~List() deletes the list, and all objects still in the list.

  void Append(T *object);
  //* Append(object) adds the object to the end of the list.

  T *Current();
  //* Current() returns a pointer to the current object.  If the current
  //* pointer is off the list (because of Rewind()), Current() fist
  //* moves to the first element of the list as defined by the
  //* current direction.

  void Delete();
  //* Delete() deletes the current object, and makes the next object
  //* (in the current direction) the current object.  If the current
  //* pointer is off the list (because of Rewind()), Delete() fist
  //* moves to the first element of the list as defined by the
  //* current direction.

  int Seek(T *object);
  //* Seek(object) scans through the list (from the next position
  //* and in the current direction) until a match is found (in which
  //* case the return value is 1) or the end of the list is past (in
  //* which case the return value is 0).  A match means that the
  //* object address is identical.

  void Forward();
  //* Forward() sets the direction of Next() and Delete() to be from the
  //* front of the list to the end.  Forward() does NOT change which
  //* object is the current object.

  T *Next();
  //* Next() moves to the next object in the list (as defined by the current
  //* direction) the current object and then returns a pointer to that
  //* object.

  void Prepend(T *object);
  //* Prepend(object) adds the object to the beginning of the list.

  void Reverse();
  //* Reverse() sets the direction of Next() and Delete() to be from the
  //* end of the list to the front.  Reverse() does NOT change which
  //* object is the current object.

  void Rewind();
  //* Rewind() moves the current pointer off of the list so that the next
  //* call to Next() get either the first or last object in the list
  //* depending on the current direction.

  void Remove();
  //* Remove() removes the current object, and makes the next object
  //* (in the current direction) the current object.  If the current
  //* pointer is off the list (because of Rewind()), Remove() fist
  //* moves to the first element of the list as defined by the
  //* current direction.

  int Empty();
  //* Empty() returns 0 if the list is empty.

  UINT32 Size();
  //* Size() returns the number of object currently in the list.

  void SetPosition(UINT32 index);
  //* SetPosition(index) finds the index'th object in the list where
  //* the first object is numbered zero, and the first object is
  //* defined by the current direction.

private:
  ListElement<T> *head;
  ListElement<T> *tail;
  ListElement<T> *current;
  enum Direction {Forward_Dir, Backward_Dir};
  Direction dir;
  enum State {Iterating, EndOfList};
  State state;
  UINT32 size;
};

#endif
