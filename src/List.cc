
#include "system.h"
#include "New.h"
#include "List.h"

#define TEST_LIST \
  if (size == 0) { \
    assert(head == 0); \
    assert(tail == 0); \
    assert(current == 0); \
    assert(state == EndOfList); \
  } else { \
    assert(head != 0); \
    assert(tail != 0); \
    if (size == 1) { \
      assert(head == tail); \
      assert(head->prev == 0); \
      assert(head->next == 0); \
    }; \
  }

template <class T>
inline List<T>::List() {

  head = 0;
  tail = 0;
  current = 0;
  dir = Forward_Dir;
  state = EndOfList;
  size = 0;

  TEST_LIST;
};

template <class T>
inline List<T>::~List() {

  TEST_LIST;

  Rewind();
  while (!Empty()) {
    Delete();
  };
};

template <class T>
inline void List<T>::Append(T *object) {

  TEST_LIST;

  ListElement<T> *temp = tail;
  New(tail, ListElement<T>);
  tail->object = object;
  tail->next = 0;
  if (temp) {
    tail->prev = temp;
    temp->next = tail;
  } else {
    tail->prev = 0;
    head = tail;
  };
  size++;

  TEST_LIST;
};

template <class T>
inline void List<T>::Prepend(T *object) {

  TEST_LIST;

  ListElement<T> *temp = head;
  New(head, ListElement<T>);
  head->object = object;
  head->prev = 0;
  if (temp) {
    head->next = temp;
    temp->prev = head;
  } else {
    head->next = 0;
    tail = head;
  };
  size++;

  TEST_LIST;
};

template <class T>
inline void List<T>::Delete() {

  TEST_LIST;

  if (!current) {
    (void)Next();
  };
  ListElement<T> *temp = current;
  (void)Next();

  if (temp->prev) {
    temp->prev->next = temp->next;
  } else {
    head = temp->next;
    if (head) {
      head->prev = 0;
    };
  };
  if (temp->next) {
    temp->next->prev = temp->prev;
  } else {
    tail = tail->prev;
    if (tail) {
      tail->next = 0;
    };
  };
  DELETE(temp->object);
  DELETE(temp);
  size--;

  TEST_LIST;
};

template <class T>
inline void List<T>::Remove() {

  TEST_LIST;

  if (!current) {
    (void)Next();
  };
  ListElement<T> *temp = current;
  (void)Next();

  if (temp->prev) {
    temp->prev->next = temp->next;
  } else {
    head = temp->next;
    if (head) {
      head->prev = 0;
    };
  };
  if (temp->next) {
    temp->next->prev = temp->prev;
  } else {
    tail = tail->prev;
    if (tail) {
      tail->next = 0;
    };
  };
  DELETE(temp);
  size--;

  TEST_LIST;
};

template <class T>
inline void List<T>::Forward() {

  TEST_LIST;

  dir = Forward_Dir;

  TEST_LIST;
};

template <class T>
inline int List<T>::Empty() {

  TEST_LIST;

  return !head;
};

template <class T>
inline void List<T>::Rewind() {

  TEST_LIST;

  current = 0;
  if (head) {
    state = Iterating;
  };

  TEST_LIST;
};

template <class T>
inline void List<T>::Reverse() {

  TEST_LIST;

  dir = Backward_Dir;

  TEST_LIST;
};

template <class T>
inline T *List<T>::Next() {

  TEST_LIST;

  // Move <current> to the next element
  if (state == Iterating) {
    switch(dir) {
    case Forward_Dir:
      if (current) {
	current = current->next;
      } else {
	current = head;
      };
      break;
    case Backward_Dir:
      if (current) {
	current = current->prev;
      } else {
	current = tail;
      };
    };
  };

  // If just past the end, or previously past the end...
  if (!current || state == EndOfList) {
    current = 0;
    state = EndOfList;
    return 0;
  };

  TEST_LIST;

  return current->object;
};

template <class T>
inline T *List<T>::Current() {

  TEST_LIST;

  if (!current) {
    (void)Next();
  };
  if (current) {
    return current->object;
  } else {
    return 0;
  };
};

template <class T>
inline int List<T>::Seek(T *object) {

  TEST_LIST;

  while (Next()) {
    if (current->object == object) {
      return 1;
    };
  };
  return 0;
};

template <class T>
inline UINT32 List<T>::Size() {

  TEST_LIST;

  return size;
};

template <class T>
inline void List<T>::SetPosition(UINT32 index) {

  TEST_LIST;

  Rewind();
  while (index-- > 0 && Next());

  TEST_LIST;
};



