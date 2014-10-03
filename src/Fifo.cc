
#include "system.h"
#include "New.h"
#include "Fifo.h"

template <class T>
inline Fifo<T>::Fifo(UINT32 max_size) {

  head = 0;
  tail = 0;
  current = 0;
  size = 0;
  this->max_size = max_size;
};

template <class T>
inline Fifo<T>::~Fifo() {

  Rewind();
  while (head) {
    DeleteTail();
  };
};

template <class T>
inline void Fifo<T>::Add(T *object) {

  if (size == max_size) {
    DeleteTail();
  };
  FifoElement<T> *temp = head;
  New(head, FifoElement<T>);
  head->object = object;
  head->next = 0;
  if (temp) {
    head->prev = temp;
    temp->next = head;
  } else {
    head->prev = 0;
    tail = head;
  };
  size++;
};

template <class T>
inline void Fifo<T>::DeleteTail() {

  if (tail == current) {
    current = 0;
  };
  FifoElement<T> *temp = tail;
  tail = tail->next;
  if (tail) {
    tail->prev = 0;
  } else {
    head = 0;
  };
  DELETE(temp->object);
  DELETE(temp);
  size--;
};

template <class T>
inline void Fifo<T>::Rewind() {

  current = tail;
  dir = 0;
};

template <class T>
inline void Fifo<T>::Reverse() {

  current = head;
  dir = 1;
};

template <class T>
inline T *Fifo<T>::Next() {

  if (current) {
    T *temp = current->object;
    if (dir) {
      current = current->prev;
    } else {
      current = current->next;
    };
    return temp;
  } else {
    return 0;
  };
};
