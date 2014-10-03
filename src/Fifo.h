
#ifndef _Fifo_H
#define _Fifo_H

#include "system.h"

template <class T> class Fifo;

template <class T>
class FifoElement {
private:
  T *object;
  FifoElement *prev;
  FifoElement *next;
  friend class Fifo<T>;
};

template <class T>
class Fifo {
public:
  Fifo(UINT32 max_size);
  ~Fifo();
  void Add(T *object);
  void Rewind();
  void Reverse();
  T *Next();
  int IsEmpty() { return size == 0; };
private:
  void DeleteTail();

  FifoElement<T> *head;
  FifoElement<T> *tail;
  FifoElement<T> *current;
  UINT32 max_size;
  UINT32 size;
  int dir;
};

#endif
