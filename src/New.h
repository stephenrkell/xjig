
// include New.h in all .cc files
// put ALLOC_New; in one file (preferrably the file containing main())
// put INIT_New; at beginning of main();


#ifndef _New_H
#define _New_H

#include <new>

class MemRecord {
public:
  MemRecord();
  void *pointer;
  const char *file;
  int line;
};

#define NUM_OF_MEM_RECORDS 3000

extern MemRecord mem_records[];
extern const char *mem_error_file, *mem_error_pointer, *mem_error_thing;
extern int mem_error_line;

#ifdef MEM_DEBUG

#define New(pointer, thing) \
   mem_error_file = __FILE__; \
   mem_error_line = __LINE__; \
   mem_error_pointer = #pointer; \
   mem_error_thing = #thing; \
   pointer = new thing; \
   AddMemRef((void *)pointer, __FILE__, __LINE__)

#define DELETE(pointer) \
   RemoveMemRef((void *)pointer, __FILE__, __LINE__); \
   delete pointer

#define DELETE_ARRAY(pointer) \
   RemoveMemRef((void *)pointer, __FILE__, __LINE__); \
   delete [] pointer

#define INIT_New \
   std::set_new_handler(OutOfMem)

#else

#define New(pointer, thing) \
   pointer = new thing

#define DELETE(pointer) \
   delete pointer

#define DELETE_ARRAY(pointer) \
   delete [] pointer

#define INIT_New NULL

#endif

void DumpMem();
void OutOfMem();
void AddMemRef(void *pointer, const char *file, int line);
void RemoveMemRef(void *pointer, const char *file, int line);

#endif


