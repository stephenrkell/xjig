
#include "New.h"
#include <stdlib.h>
#include <stdio.h>

MemRecord mem_records[NUM_OF_MEM_RECORDS];
char *mem_error_file, *mem_error_pointer, *mem_error_thing = "";
int mem_error_line = 0;

MemRecord::MemRecord() {

  pointer = 0;
};

void DumpMem() {
  int i;
  int count = 0;

  fprintf(stderr, "\n\nMemory Dump:\n\n");

  for (i = 0; i < NUM_OF_MEM_RECORDS; i++) {
    if (mem_records[i].pointer) {
      count++;
      fprintf(stderr, "   File: %s, line: %i, address: %p\n",
	      mem_records[i].file,
	      mem_records[i].line,
	      mem_records[i].pointer);
    };
    if (count == 20) {
      count = 0;
      fprintf(stderr, "press enter to continue...\n");
      getchar();
    };
  };
  exit(1);
};

void OutOfMem() {
  fprintf(stderr, "\nOut of memory...\n");
  fprintf(stderr, "In file \'%s\', line %i:  %s = new %s;\n\n",
	  mem_error_file, mem_error_line, mem_error_pointer, mem_error_thing);
  DumpMem();
};

void AddMemRef(void *pointer, char *file, int line) {
  int i;

  if (!mem_records) {
    fprintf(stderr, "mem_records is NULL\n");
    exit(1);
  };

  for (i = 0; i < NUM_OF_MEM_RECORDS; i++) {
    if (!mem_records[i].pointer) {
      mem_records[i].pointer = pointer;
      mem_records[i].file = file;
      mem_records[i].line = line;
      return;
    };
  };
  fprintf(stderr, "Out of memory records\n");
  DumpMem();
};

void RemoveMemRef(void *pointer, char *file, int line) {
  int i;

  for (i = 0; i < NUM_OF_MEM_RECORDS; i++) {
    if (mem_records[i].pointer == pointer) {
      mem_records[i].pointer = 0;
      return;
    };
  };
  fprintf(stderr, "Tried to delete non-allocated object:\n");
  fprintf(stderr, "   File: %s, line: %i\n", file, line);
  DumpMem();
};
