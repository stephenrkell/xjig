
/*
Ideas for more options:
   - Create/Change/Delete windows
   - Convert responses generated by X from success to failures
*/

#include "system.h"
#include "socket.h"
#include "message.h"
#include "event.h"
#include "List.cc"
#include "window.h"
#include "xjig.h"
#include "New.h"

// **************** Data Declaration/Initialization *******************

// *** (11 Global variables) ***

static UINT32 PROG_message_count;
static UINT32 X_message_count;

static Socket *X = 0;
static Socket *PROG = 0;

static int connect_request_flag = 0;

enum MessageDetail {No_Detail = 0, Min_Detail = 1, Packet_Detail = 2,
		    Brief_Detail = 3, Full_Detail = 4};
static MessageDetail message_detail;

static EventContext event_context;

static UINT32 last_seq_num;

static UINT32 random_chance;
#define BASE_CHANCE 10000

static int smart_win_on;

// List of windows we know to exist, but we did not see their creation
// request.
List<UINT32> orphan_windows;

// ******************************************

// *** FileExists(filename) returns 1 if <filename> exists, otherwise 0

int FileExists(const char *filename) {
  int f;

  f = open(filename, O_RDONLY);
  if (f == -1) {
    if (errno == ENOENT) {
      return 0;
    };
    fprintf(stderr, "Error %i on open()\n", errno);
    terminate();
  };
  close(f);
  return 1;
};

void Brief(const char *s) {
  if (message_detail >= Brief_Detail) {
    printf("%s", s);
  };
};

void Min(const char *s) {
  if (message_detail >= Min_Detail) {
    printf("%s", s);
  };
};

void terminate() {

  Brief("Closing windows\n");
  Window::DeleteAll();

  if (X) {
    Brief("Closing X session\n");
    DELETE(X);
  };
  
  if (PROG) {
    Brief("Closing PROG\n");
    DELETE(PROG);
  };
  
  exit(1);
};

SIG_HANDLER;

UINT32 Random(UINT32 max_num) {
  
  return UINT32((double(rand()) / (double(0x7FFFFFFF) + 1.0)) * (double(max_num) + 1.0));
};


// *** SendMessage() Mangles, Packets, and Sends off messages ***

void SendMessage(Message &msg, Modifier mod, Socket *dest) {

  if (dest == X) {
    printf("Sending message to X, modifier = %i\n", (int)mod);
  }

  if (msg.Size() > 1) {

    if (mod & RandomBit_Mod && Random(BASE_CHANCE) + 1 <= random_chance) {
      Brief("Modifying message by RandomBit\n");

      UINT32 which_byte = Random(msg.Size() - 1);
      PutUINT8(msg, which_byte, msg[which_byte] ^ (1 << Random(8)));
    };

    if (mod & RandomIncDec_Mod && Random(BASE_CHANCE) + 1 <= random_chance) {
      Brief("Modifying message by RandomIncDec\n");

      UINT32 which_byte = Random(msg.Size() - 1);
      PutUINT8(msg, which_byte, msg[which_byte] - 1 + Random(2));
    };

    if (mod & RandomByte_Mod && Random(BASE_CHANCE) + 1 <= random_chance) {
      Brief("Modifying message by RandomByte\n");

      UINT32 which_byte = Random(msg.Size() - 1);
      PutUINT8(msg, which_byte, Random(255));
    };

    if (mod & RandomStretch_Mod && Random(BASE_CHANCE) + 1 <= random_chance) {
      Brief("Modifying message by RandomStretch\n");
      
      UINT32 start = Random(msg.Size() - 1) + 1;
      UINT32 length = Random(msg.Size() - start) + 1;
      UINT8 *garbage;
      New(garbage, UINT8[length]);
      if (!garbage) {
	fprintf(stderr, "No room for garbage (take it out)\n");
	terminate();
      };
      for (UINT32 i = 0; i < length; i++) {
	garbage[i] = (UINT8)Random(0xFF);
      };
      msg.Modify(start, length, garbage);
      DELETE_ARRAY(garbage);
    };

    if (mod & RandomAppend_Mod && Random(BASE_CHANCE) + 1 <= random_chance) {
      Brief("Modifying message by RandomAppend\n");

      UINT32 append_size = Random(127) + 1;
      msg.SetSizeLock(0);
      for (UINT32 i = 0; i <= append_size; i++) {
	PutUINT8(msg, msg.Size(), Random(255));
      };
      msg.SetSizeLock(1);
    };
  };

  msg.Send(*dest);
};

// ********* Message Snooping Routines **************

void DumpWindow(UINT32 wid) {

  if (!wid) return;

  Window::Dump();

  Window *win = Window::FindWindow(wid);

  if (win) {
    printf("                    event_mask = %X\n",
	   (unsigned int)win->EventMask());
    printf("         do_not_propagate_mask = %X\n",
	   (unsigned int)win->PropagateMask());
  } else {
    printf("                   Window not found.\n");
  };
};

void AddOrphanWindow(UINT32 wid) {

  UINT32 *temp;

  orphan_windows.Rewind();
  while (orphan_windows.Next()) {
    if (*orphan_windows.Current() == wid) {
      return; // don't need to add multiple times
    };
  };

  New(temp, UINT32(wid));
  if (!temp) {
    fprintf(stderr, "No room for orphans\n");
    terminate();
  };
  orphan_windows.Append(temp);
};

void Snoop_X_Message(const Message &msg) {
  UINT32 i;

  X_message_count++;
  if (message_detail >= Brief_Detail) {
    printf("   Message #%u from X:  Code = %u, Length = %u, Sequence number = %u, Name = ",
	   (unsigned int)X_message_count - 1,
	   (unsigned int)msg[0],
	   (unsigned int)msg.Size(),
	   (unsigned int)GetUINT16(msg, 2));
  };
  last_seq_num = GetUINT16(msg, 2);

  if (connect_request_flag && GetUINT16(msg, 2) == 11) {

    connect_request_flag = 0;
    {
      if (msg[28] != 1) {
	fprintf(stderr, "Number of root screens = %u\n", (unsigned int)msg[28]);
	terminate();
      };
      const UINT32 v = GetUINT16(msg, 24);
      const UINT32 n = msg[29];
      const UINT32 m = msg.Size() - 8 * n - v - 40;
      const UINT32 p = (((UINT32)GetUINT16(msg, 6)) - 8 - 2 * n) * 4 - v - m;
      
      UINT32 root_window_id = GetUINT32(msg, 40 + v + p + 8 * n);

      if (smart_win_on) {
	// *** Create root window and "set it free" (i.e. don't delete it) ***
	Window *win;
	New(win, Window(root_window_id));
      };

      if (message_detail >= Full_Detail) {
	printf("root_window_id = %u\n", (unsigned int)root_window_id);
	printf("m = %u\n", (unsigned int)m);
	printf("n = %u\n", (unsigned int)n);
	printf("p = %u\n", (unsigned int)p);
	printf("v = %u\n", (unsigned int)v);
	printf("Message size in words = %u\n", (unsigned int)GetUINT16(msg, 6));
      };
    };

  } else if (msg.Size() > 0) {
    switch (msg[0]) {
      
      // *** Key Press ***
    case 2:
      Brief("Key Press\n");
      goto event_handler;
    case 3:
      Brief("Key Release\n");
      goto event_handler;
    case 4:
      Brief("Button Press\n");
      goto event_handler;
    case 5:
      Brief("Button Release\n");
      goto event_handler;
    case 6:
      Brief("Motion Notify\n");
      goto event_handler;
    case 7:
      Brief("Enter Notify\n");
      goto event_handler;
    case 8:
      Brief("Leave Notify\n");
event_handler:
      if (message_detail >= Full_Detail) {
	printf("            code = %u\n", (unsigned int)msg[0]);
	printf("          detail = %u\n", (unsigned int)msg[1]);
	printf("      sequence # = %u\n", (unsigned int)GetUINT16(msg, 2));
	printf("       TIMESTAMP = %u\n", (unsigned int)GetUINT32(msg, 4));
	printf("     root window = %u\n", (unsigned int)GetUINT32(msg, 8));
	printf("    event window = %u\n", (unsigned int)GetUINT32(msg, 12));

	if (smart_win_on) {
	  DumpWindow(GetUINT32(msg, 12));
	};

	printf("    child window = %u\n", (unsigned int)GetUINT32(msg, 16));

	if (smart_win_on) {
	  DumpWindow(GetUINT32(msg, 16));
	};

	printf("          root-x = %u\n", (unsigned int)GetUINT16(msg, 20));
	printf("          root-y = %u\n", (unsigned int)GetUINT16(msg, 22));
	printf("         event-x = %u\n", (unsigned int)GetUINT16(msg, 24));
	printf("         event-y = %u\n", (unsigned int)GetUINT16(msg, 26));
	printf(" SETofKEYBUTMASK = %u\n", (unsigned int)GetUINT16(msg, 28));
	printf("     same-screen = %u\n", (unsigned int)msg[30]);
	printf("          unused = %u\n", (unsigned int)msg[31]);
	printf("\n");
      };

      for (i = 0; i <= 31; i++) {
	event_context.SetDefault(i, msg[i]);
      };

      if (smart_win_on) {
	printf("%u orphans\n", (unsigned int)orphan_windows.Size());

	UINT32 wid;
	for (UINT32 i = 8; i <= 16; i += 4) { // get root, event, child
	  wid = GetUINT32(msg, i);
	  if (wid && !Window::FindWindow(wid)) {
	    AddOrphanWindow(wid);
	  };
	};
      };

      break;

    default:
      Brief("???\n");
    };
  };
};

void Snoop_PROG_Message(Message &msg) {

  PROG_message_count++;
  if (message_detail >= Brief_Detail) {
    printf("   Message #%u from PROG:  Code = %u, Length = %u, Name = ",
	   (unsigned int)PROG_message_count - 1,
	   (unsigned int)msg[0],
	   (unsigned int)msg.Size());
  };

  int i;

  if (msg.Size() > 0) {
    switch (msg[0]) {
      
      // *** Connection Setup ***
    case 'B':
    case 'l':
      if (PROG_message_count == 1) {
	Brief("Connection Request\n");
	
	connect_request_flag = 1;
	
	// Verify X11
	if (GetUINT16(msg, 2) != 11) {
	  fprintf(stderr, "Test program is using X version %u\n",
		  (unsigned int)GetUINT16(msg, 2));
	  //	getchar();
	  //	fprintf(stderr, "Error: Test program is not using X version 11\n");
	  //	terminate();
	};
      } else {
	Brief("???\n");
      };
      break;
      
      // *** Create Window ***
    case 1:
      Brief("Create Window\n");
      if (message_detail >= Full_Detail) {
	printf("         window id = %u\n", (unsigned int)GetUINT32(msg, 4));
	printf("            parent = %u\n", (unsigned int)GetUINT32(msg, 8));
	printf("                 x = %i\n", (int)(INT32)GetUINT32(msg, 12));
	printf("                 y = %i\n", (int)(INT32)GetUINT32(msg, 16));
	printf("             width = %u\n", (unsigned int)GetUINT16(msg, 18));
	printf("            height = %u\n", (unsigned int)GetUINT16(msg, 20));
	printf("      border-width = %u\n", (unsigned int)GetUINT16(msg, 22));
      };

//      printf("Before:\n");
//      Window::Dump();

      if (smart_win_on) {
	Window *window;
	New(window, Window(msg));
      };

//      printf("After:\n");
//      Window::Dump();
//      getchar();
      break;

    case 2:
      Brief("ChangeWindowAttributes\n");
      if (message_detail >= Full_Detail) {
	printf("            window = %u\n", (unsigned int)GetUINT32(msg, 4));
	printf("        value-mask = %u\n", (unsigned int)GetUINT32(msg, 8));  
      };
      if (smart_win_on) {
	Window *window;
	UINT32 wid = GetUINT32(msg, 4);
	window = Window::FindWindow(wid);
#ifdef WIN_STOP
	if (!window) {
	  fprintf(stderr, "(CWA) PROG specified non-existing window id: %u\n",
		  (unsigned int)wid);
	  terminate();
	};
#endif
	if (window) {
	  window->ChangeWindowAttributes(msg);
	} else {
	  AddOrphanWindow(wid);
	};
      };
      break;

    case 7:
      Brief("ReparentWindow");
      if (message_detail >= Full_Detail) {
	printf("            window = %u\n", (unsigned int)GetUINT32(msg, 4));
      };
      if (smart_win_on) {
	Window *window;
	UINT32 wid = GetUINT32(msg, 4);
	window = Window::FindWindow(wid);
#ifdef WIN_STOP
	if (!window) {
	  fprintf(stderr, "PROG specified non-existing parent window id: %u\n",
		  (unsigned int)wid);
	  terminate();
	};
#endif
	if (window) {
	  window->ReparentWindow(msg);
	} else {
	  AddOrphanWindow(wid);
	};
      };
      break;

    case 12:
      Brief("ConfigureWindow");
      if (message_detail >= Full_Detail) {
	printf("            window = %u\n", (unsigned int)GetUINT32(msg, 4));
      };
      if (smart_win_on) {
	Window *window;
	UINT32 wid = GetUINT32(msg, 4);
	window = Window::FindWindow(wid);
#ifdef WIN_STOP
	if (!window) {
	  fprintf(stderr, "PROG specified non-existing parent window id: %u\n",
		  (unsigned int)wid);
	  terminate();
	};
#endif
	if (window) {
	  window->ConfigureWindow(msg);
	} else {
	  AddOrphanWindow(wid);
	};
      };
      break;

      // *** InternAtom ***
    case 16:
      Brief("InternAtom\n");
      if (message_detail >= Full_Detail) {
	UINT32 n = (UINT32)GetUINT16(msg, 4);
	UINT32 p = (((UINT32)GetUINT16(msg, 2)) - 2) * 4 - n;
	
	printf("      only-if-exists = %u\n", (unsigned int)msg[1]);
	printf("      request length = %u\n", (unsigned int)GetUINT16(msg, 2));
	printf("            (padding = %u\n)", (unsigned int)p);
	printf("         name length = %u\n", (unsigned int)n);
	printf("              unused = %u\n", (unsigned int)GetUINT16(msg, 6));

	printf("                name = '");
	for (UINT32 i = 0; i < n; i++) {
	  printf("%c", (char)msg[8 + i]);
	};
	printf("'\n");
      };
      break;

    case 45:
      Brief("Open Font\n");
      break;

    case 38: {
      Brief("QueryPointer\n");
      // *** Create message containing response to QueryPointer ***
      Event tmpEvent(event_context,
			PROG_message_count - 1   // sequence number
                        );
      SendMessage(tmpEvent,
		  No_Mod,
		  PROG);
      // *** Change PROG message into NoOperation to   ***
      // *** keep X server's sequence number on track. ***
      PutUINT8(msg, 0, 127);
      PutUINT16(msg, 2, 1);
      msg.SetSize(4);
    } break;

    case 39: {
      Brief("GetMotionEvents");
      // *** Create message containing response to GetMotionEvents ***
      Event tmpEvent(event_context,
			 PROG_message_count - 1, // sequence number
			 GetUINT32(msg, 8),      // start time
			 GetUINT32(msg, 12)      // stop time
			 );
      SendMessage(tmpEvent,
		  No_Mod,
		  PROG);
      // *** Change PROG message into NoOperation to   ***
      // *** keep X server's sequence number on track. ***
      PutUINT8(msg, 0, 127);
      PutUINT16(msg, 2, 1);
      msg.SetSize(4);
    } break;

    case 55:
      Brief("CreateGC\n");

      if (message_detail >= Full_Detail) {
	printf("      cid = %u\n", (unsigned int)GetUINT32(msg, 4));
      };
      break;

    case 76:
      Brief("ImageText8\n");

      if (message_detail >= Full_Detail) {

	printf("Length = %i.  Content = '\n", (unsigned int)msg[1]);
	for (i = 0; i < msg[1]; i++) {
	  printf("%c", msg[16 + i]);
	};
	printf("'  Values =\n");
	for (i = 0; i < msg[1]; i++) {
	  printf("%u -> (%u, %u)  ", (unsigned int)msg[16 + i],
		 (unsigned int)GetUINT16(msg, 12),
		 (unsigned int)GetUINT16(msg, 14));
	};
	printf("\n");
      };
      break;

    default:
      Brief("???\n");
    }; /* end switch */
  }; /* end-if size>0 */
};

UINT32 ClientSubLength(Message const &msg, UINT32 index) {
  UINT32 size;
  UINT32 min_size;

  if ((msg[index] == 'B' || msg[index] == 'l') && PROG_message_count == 0) {
    // This is a Connection Setup message
    size = 12 // basic message size
      + ((UINT32)GetUINT16(msg, index + 6)) // protocol-name size
	+ ((UINT32)GetUINT16(msg, index + 8)); // protocol-data size
    min_size = 10;
  } else {
    size = ((UINT32)GetUINT16(msg, index + 2)) * 4;
    min_size = 4;
  };
  if (size < min_size) {
    fprintf(stderr, "Size of message from PROG is definitely too small\n");
    size = min_size;
  };
  return size;
};

UINT32 XSubLength(Message const &msg, UINT32 index) {
  UINT32 size;
  UINT32 min_size;


  if (msg[index] == 0) {
    size = msg.Size() - index;
    min_size = size;
  } else if (X_message_count == 0) {
    size = (UINT32)GetUINT16(msg, index + 6) * 4 + 8;
    min_size = 8;
    printf("Calculated size = %u, Actual size = %u\n",
	   (unsigned int)size,
	   (unsigned int)msg.Size());
  } else if (msg[index] == 1) {
    size = GetUINT32(msg, index + 4) * 4 + 32;
    min_size = 8;
  } else {
    size = 32;
    min_size = 32;
  };
  
  if (size < min_size) {
    fprintf(stderr, "Size of message from X is definitely too small\n");
    size = min_size;
  };
  return size;
};

void syntax() {
  fprintf(stderr, "Syntax:  xjig <port number> [-x MOD] [-p MOD] [-s seed] [-w wait count] [-d DTL]\n");
  fprintf(stderr, "         [-m min_delay] [-t max_run_time] [-h hang_time] [-c mod chance] [-u smart_win_on]\n");

  fprintf(stderr, "   MOD    Type of modification\n");
  fprintf(stderr, "  -----   --------------------\n");
  fprintf(stderr, "    1     Flip single bits\n");
  fprintf(stderr, "    2     Inc/dec single bytes\n");
  fprintf(stderr, "    4     Change single bytes\n");
  fprintf(stderr, "    8     Change stretch of bytes\n");
  fprintf(stderr, "   16     Append bytes\n");
  fprintf(stderr, "   32     Insert well-formed events\n");
  fprintf(stderr, "   64     Insert legal events\n");
  fprintf(stderr, "  128     Insert bogus events\n");
  fprintf(stderr, "  256     Force packet cracking\n");

  fprintf(stderr, "\n");

  fprintf(stderr, "   DTL    Level of detail\n");
  fprintf(stderr, "  -----   --------------------\n");
  fprintf(stderr, "    0     None (only fatal errors)\n");
  fprintf(stderr, "    1     Minimum\n");
  fprintf(stderr, "    2     Each Packet\n");
  fprintf(stderr, "    3     Brief\n");
  fprintf(stderr, "    4     Full\n");

  terminate();
};

// ************ Main ***************

int main(int argc, char *argv[]) {

  INIT_New;

  int arg;
  Message msg;

  Message *last_msg[128] __attribute__((unused));
  int i;
  for (i = 0; i <= 127; i++) {
    last_msg[i] = 0;
  };
  
  if (argc < 2) {
    syntax();
  };
  
  char *host_name;
  char *x_host_name;
  char *x_display_var;
  int x_display_number;
  int x_display_port = 6000;

  New(host_name, char[100]);

  if (!host_name) {
    fprintf(stderr, "no room for host_name\n");
    terminate();
  };
  if (gethostname(host_name, 100)) {
    fprintf(stderr, "error %i on gethostname()\n", errno);
    terminate();
  };
  x_host_name = host_name;
  if (NULL != (x_display_var = getenv("DISPLAY"))) {
    char *display_val = strdup(x_display_var);
    assert(display_val);
    /* Null out the colon onwards. */
    char *colon = strchr(display_val, ':');
    if (!colon) {
      fprintf(stderr, "bad DISPLAY variable: %s\n", display_val);
      free(display_val);
      terminate();
    }
    char *dot = strchr(colon + 1, '.');
    if (!dot) {
      /* It's okay to lack the dot. */
      dot = display_val + strlen(display_val);
      assert(*dot == '\0');
    } else {
      *dot = '\0';
    }
    x_display_number = atoi(colon + 1);
    x_display_port = 6000 + x_display_number;
    free(display_val);
  }

  int program_port = atoi(argv[1]);
  if (program_port == x_display_port || program_port < 6000 || program_port > 6999) {
    fprintf(stderr, "Bad port number\n");
    terminate();
  };
  
  // *** Defaults ***
  UINT32 X_mod = 0;
  UINT32 PROG_mod = 0;
  srand(12345678);
  UINT32 wait_count = 0;
  message_detail = Brief_Detail;
  UINT32 min_delay = 0;         // in 1/1000th of seconds
  time_t max_time = 0x70000000; // in seconds
  time_t hang_time = 5*60;      // in seconds
  random_chance = 50;           // in percent
  smart_win_on = 0;

  for (arg = 2; arg < argc; arg++) {
    if (argv[arg][0] != '-') {
      syntax();
    } else switch (argv[arg][1]) {
    case 'x':
      X_mod = atoi(&(argv[arg][2]));
      if ((X_mod & RandomWellFormed_Mod) || (X_mod & RandomLegal_Mod)) {
	fprintf(stderr, "Can not send events to X, only to PROG\n\n");
	syntax();
      };
      if (X_mod > MAX_MODIFIER) {
	syntax();
      };
      break;
    case 'p':
      if ((PROG_mod = atoi(&(argv[arg][2]))) > MAX_MODIFIER) {
	syntax();
      };
      break;
    case 's':
      srand(atoi(&(argv[arg][2])));
      break;
    case 'w':
      wait_count = atoi(&(argv[arg][2]));
      break;
    case 'd':
      message_detail = (MessageDetail)atoi(&(argv[arg][2]));
      printf("   Setting detail to level %i\n", (int)message_detail);
      break;
    case 'm':
      min_delay = atoi(&(argv[arg][2]));
      break;
    case 't':
      max_time = atoi(&(argv[arg][2]));
      break;
    case 'h':
      hang_time = atoi(&(argv[arg][2]));
      break;
    case 'c':
      random_chance = atoi(&(argv[arg][2]));
      break;
    case 'u':
      smart_win_on = atoi(&(argv[arg][2]));
      break;
    default:
      syntax();
    };
  };
  
  // *** Set up Signals ***
  
/*
  {
    // Route all signals to handler (which terminates program)
    for (int i = 1; i <= MAX_SIGNAL; i++) {
      SET_SIGNAL(i, handler);
    };
  };
*/

  // *** Make initial connects ***
    
  Brief("Connecting X\n");
  
  New(X, Socket(x_host_name, x_display_port));

  if (!X) {
    fprintf(stderr, "Socket X not allocated\n");
    terminate();
  };
  
  X->Connect();
  
  Brief("Connecting remote port\n");
  
  Socket program_port_socket(host_name, program_port);
  
  Brief("Connecting client\n");
  
  New(PROG, Socket(program_port_socket));

  if (!PROG) {
    fprintf(stderr, "Socket PROG not allocated\n");
    terminate();
  };
  
  struct rlimit max_file_num;  
  SET_RLIM_MAX;
  
  fd_set fdset, nullset;  
  FD_ZERO(&nullset);
  
// ******************* MAIN LOOP *****************************************
  
  int temp;
  UINT32 packet_count = 0;
  UINT32 X_packet_count = 0;
  UINT32 PROG_packet_count = 0;
  UINT32 size;
  //int index;
  UINT32 temp_count;
  UINT32 skip_break = 0;
  timeval zero_time;
    zero_time.tv_sec= 0;
    zero_time.tv_usec= 0;
  
  timeb *time_pointer = 0;
  New(time_pointer, timeb);
  if (!time_pointer) {
    fprintf(stderr, "*time_pointer not allocated\n");
    terminate();
  };
  if (ftime(time_pointer) == -1) {
    fprintf(stderr, "Error %i from ftime()\n", errno);
    terminate();
  };
  time_t start_seconds = time_pointer->time;
  UINT32 last_time = time_pointer->millitm;
  UINT32 current_time;
  time_t last_PROG_message_time = start_seconds;
  PROG_message_count = 0;
  X_message_count = 0;

  while (1) {
    FD_ZERO(&fdset);
    FD_SET(PROG->Handle(), &fdset);
    FD_SET(X->Handle(), &fdset);
    temp = select(max_file_num.rlim_max,
		  (FD_SET_CAST)&fdset,
		  (FD_SET_CAST)&nullset,
		  (FD_SET_CAST)&nullset, 
		  &zero_time);
    if (temp == -1) {
      fprintf(stderr, "Error %i on select()\n", errno);
      terminate();
    };
    
    if (ftime(time_pointer) == -1) {
      fprintf(stderr, "Error %i from ftime()\n", errno);
      terminate();
    };
    if (time_pointer->time - start_seconds > max_time) {
      fprintf(stderr, "Application ran out of time\n");
      terminate();
    };
    if (time_pointer->time - last_PROG_message_time > hang_time) {
      if (FileExists("core")) {
	fprintf(stderr, "Application dumped core\n");
	terminate();
      };
      fprintf(stderr, "Application hung\n");
      terminate();
    };
    current_time = (time_pointer->time - start_seconds) * 1000
      + time_pointer->millitm;

    if (temp) {
      if (packet_count <= wait_count) packet_count++;
    };

    if (temp == 0) {

    // ********** No Messages Yet, Insert Event? **********

      {
	if (packet_count > wait_count
	    && ((PROG_mod & RandomLegal_Mod)
	     || (PROG_mod & RandomWellFormed_Mod)
	     || (PROG_mod & RandomBogus_Mod))
	    && PROG_message_count > wait_count
	    && Random(BASE_CHANCE) + 1 <= random_chance
	    && current_time >= last_time + min_delay)
	{
	  last_time = current_time;

	  if (PROG_mod & RandomLegal_Mod) {
	    Brief("Inserting synthetic legal event message\n");
	    Event *event_pointer;

	    UINT32 win_id, child_id = 0, event_x, event_y, root_x, root_y;
	    UINT32 event_mask = 0;
	    Window *win = 0;
	    int looking = 1;

	    printf("1\n");

	    if (smart_win_on) {
	      // Find a child window (most of the time)
	      while(looking) {

		printf("2\n");

		win = Window::Number(Random(Window::Count() - 1));

		printf("3, win = %p\n", win);

		if (win->IsChild() || Random(100) < 5) {
		  looking = 0;
		};
	      };

	      printf("4\n");

	      win_id = win->ID();
	      event_x = Random(win->Max_x());
	      event_y = Random(win->Max_y());
	      root_x = win->Root_x(event_x);
	      root_y = win->Root_y(event_y);

	      printf("5\n");

	      if (win->IsRoot()
		  && orphan_windows.Size() > 0
		  && Random(100) < 95)
	      {
		printf("6\n");

		win = 0;
		if (!orphan_windows.Next()) {
		  printf("7\n");
		  orphan_windows.Rewind();
		};		  
		printf("8, current = %p\n", orphan_windows.Current());
		win_id = *(orphan_windows.Current());
		printf("9\n");
	      };
	    } else {
	      win_id = 0;
	      event_x = Random(1000);
	      event_y = Random(1000);
	      root_x = Random(1000);
	      root_y = Random(1000);
	    };

	    switch (Random(2)) {
//	    switch (Random(0)) {
	    case 0:
	      New(event_pointer, Event(event_context, Key(Random(247) + 8)));
	      break;
	    case 1:
	      New(event_pointer, Event(event_context, Button(Random(4) + 1)));
	      break;
	    default:
	      New(event_pointer,
		  Event(event_context, Motion(0,
					      root_x,
					      root_y
		                             )
		       )
		 );
	      break;
	    };

		printf("10\n");

	    switch ((*event_pointer)[0]) {
	    case 2:
	      event_mask = 1;
	      break;
	    case 3:
	      event_mask = 2;
	      break;
	    case 4:
	      event_mask = 4;
	      break;
	    case 5:
	      event_mask = 8;
	      break;
	    case 7:
	      event_mask = 16;
	      break;
	    case 8:
	      event_mask = 32;
	      break;
	    case 6:
	      event_mask = 64;
	      break;
	    };

		printf("11\n");

	    // Propagate if event is masked from current window
	    if (win) {
	      while (!win->IsRoot() && !(win->EventMask() & event_mask)) {

		printf("12\n");

		child_id = win_id;

		printf("13\n");
		win = win->Parent();
		printf("14\n");
		win_id = win->ID();
	      };
		printf("15\n");
	      PutUINT32(*event_pointer, 8, Window::RootID());
	      PutUINT32(*event_pointer, 12, win_id);
	      PutUINT32(*event_pointer, 16, child_id);
	    };
		printf("16\n");

	    PutUINT16(*event_pointer, 20, root_x);
	    PutUINT16(*event_pointer, 22, root_y);
	    PutUINT16(*event_pointer, 24, event_x);
	    PutUINT16(*event_pointer, 26, event_y);

	    PutUINT16(*event_pointer, 2, last_seq_num);
		printf("17\n");
	    Snoop_X_Message(*event_pointer);
		printf("18\n");

	    event_pointer->Send(*PROG);
		printf("19\n");
	    
	    DELETE(event_pointer);
		printf("20\n");
	  } else if (PROG_mod & RandomWellFormed_Mod) {
	    Brief("Inserting synthetic well formed event message\n");
	    Message event(32);
	    event.SetSizeLock(0);
	    PutUINT8(event, 0, Random(32) + 2);  // Event Type
	    for (UINT32 i = 1; i <= 31; i++) {
	      PutUINT8(event, i, Random(255));
	    };
	    event.SetSizeLock(1);
	    Snoop_X_Message(event);
	    //PutUINT16(event, 2, 1);  // sequence number
	    //PutUINT32(event, 8, 42); // RootID
	    event.Send(*PROG);
	  } else {
	    Brief("Inserting synthetic bogus event message\n");
	    UINT32 size = Random(1023) + 1;
	    Message event(size);
	    event.SetSizeLock(0);
	    for (UINT32 i = 0; i < size; i++) {
	      PutUINT8(event, i, Random(255));
	    };
	    event.SetSizeLock(1);
	    //Snoop_X_Message(event);
	    //PutUINT16(event, 2, 1);  // sequence number
	    //PutUINT32(event, 8, 42); // RootID
	    event.Send(*PROG);

	  };
	};
      };      


    } else if (FD_ISSET(PROG->Handle(), &fdset)) {

      // ********** Got Message From PROG **********

      PROG_packet_count++;

      // *** Record time last message received from PROG to watch for hang ***
      if (ftime(time_pointer) == -1) {
	fprintf(stderr, "Error %i from ftime()\n", errno);
	terminate();
      };
      last_PROG_message_time = time_pointer->time;

      if(msg.Receive(*PROG) == -1) {
	fprintf(stderr, "Connection reset by client\n");
	terminate();
      };

      if (message_detail >= Packet_Detail) {
	printf("Packet %u from PROG of size %u\n",
	       (unsigned int)PROG_packet_count - 1,
	       (unsigned int)msg.Size());
      };

      if (msg.Size() > 0) {
	if (!(PROG_mod & RandomLegal_Mod) && !(X_mod & ForceCracking_Mod)) {
	  skip_break = 1;
	} else {
	  int reset_flag = 0;
	  // *** Prescan packet and see if it parses correctly ***
	  // *** If it can't be cracked, just send whole packet ***
	  //if (PROG_message_count > 0) {
	    temp_count = PROG_message_count;
	    for (unsigned index = 0u; index + 1u <= msg.Size() && skip_break == 0;
			    index += size) {
	      if ((msg[index] == 'B' || msg[index] == 'l')
		  && PROG_message_count == 0 && index == 0) {
		// This is a Connection Setup message
	      printf("PROG_message_count = %u\n",
		     (unsigned int)PROG_message_count);
		SetByteOrder(msg[index]);
	      reset_flag = 1;
	      };
	      size = ClientSubLength(msg, index);
	      if (size == 0) {
		if (message_detail >= Min_Detail) {
		  printf("About to send message #%u which is of size 0!\n",
			 (unsigned int)temp_count - 1);
		  skip_break = 1;
		};
	      } else {
		if (message_detail >= Full_Detail) {
		  printf("  (pre-scan) Message code: %u, size %u\n",
			 (unsigned int)msg[index],
			 (unsigned int)size);
		};
		temp_count++;
		if (index + size > msg.Size()) {
		  if (message_detail >= Min_Detail) {
		    printf("About to send message #%u which is too big\n",
			   (unsigned int)temp_count - 1);
		    skip_break = 1;
		  };
		};
	      //};
	    };
	  };
	  if (reset_flag) {
	    ResetByteOrder(); // So that double-connection error doesn't occur
	  };
	  //Brief("Finished pre-scan\n");
	};


	if (skip_break) {
	  if (PROG_message_count == 0) {
	      printf("PROG_message_count = %u\n",
		     (unsigned int)PROG_message_count);
	      SetByteOrder(msg[0]);
	      connect_request_flag = 1;
	  };
	  skip_break = 0;  // Send whole packet without cracking it
	  msg.Send(*X);
	  PROG_message_count++;
	} else {
	  for (unsigned index = 0u; index + 1u <= msg.Size(); index += size) {
	    
	    if ((msg[index] == 'B' || msg[index] == 'l')
		&& PROG_message_count == 0 && index == 0) {
	      // This is a Connection Setup message
	      printf("PROG_message_count = %u\n",
		     (unsigned int)PROG_message_count);
	      SetByteOrder(msg[index]);
	    };

	    size = ClientSubLength(msg, index);
	    
	    if (message_detail >= Brief_Detail) {
	      printf("  Message code: %u, size %u\n",
		     (unsigned int)msg[index],
		     (unsigned int)size);
	    };
	    
	    if (index + size > msg.Size()) {
	      fprintf(stderr, "Uncrackable packet received from PROG\n");
	      fprintf(stderr, "Byte order: %s\n", ByteOrder());
	      msg.Dump();
	      
	      {
		FILE *f = fopen("bad.PROG.message", "w");
		if (!f) {
		  fprintf(stderr, "Unable to dump bad message\n");
		} else {
		  fprintf(f, "Byte order: %s\n", ByteOrder());
		  msg.Dump(f, 16);
		  for (index = 0; index + 1 <= msg.Size(); index += size) {
		    size = ClientSubLength(msg, index);
		    if (msg[index] == 'B' || msg[index] == 'l') {
		      fprintf(f, "%u: code: %u, name size: %u, data size: %u, bytes: %u\n",
			      (unsigned int)index,
			      (unsigned int)msg[index],
			      (unsigned int)GetUINT16(msg, index + 6),
			      (unsigned int)GetUINT16(msg, index + 8),
			      (unsigned int)size);
		    } else {
		      fprintf(f, "%u: code: %u, words: %u, bytes: %u\n",
			      (unsigned int)index,
			      (unsigned int)msg[index],
			      (unsigned int)size / 4,
			      (unsigned int)size);
		    }; //* endif *
		  };  //* endfor *
		}; //* endif *
	      }; //* end scope *
	      terminate();
	    }; //* end error-if *
	    
	    {
	      Message extracted_msg(msg, index, size);
	      Snoop_PROG_Message(extracted_msg);
	      SendMessage(extracted_msg,
			  (packet_count > wait_count) ? (Modifier)X_mod : No_Mod,
			  X
		         );
	    };
	  };
	};

      } else {

	// ********** PROG Disconnected, Set Up for Reconnect **********
	
	Min("Closing windows\n");
	Window::DeleteAll();

	Min("Closing X session\n");
	DELETE(X);
	X = 0;
	
	Min("Reconnecting to X\n");
	New(X, Socket(x_host_name, x_display_port));
	if (!X) {
	  fprintf(stderr, "Socket X not allocated (in loop)\n");
	  terminate();
	};
	X->Connect();
	
	Min("Closing PROG\n");
	DELETE(PROG);
	PROG = 0;
	
	Min("Reconnecting to test program\n");
	New(PROG, Socket(program_port_socket));
	if (!PROG) {
	  fprintf(stderr, "Socket PROG not allocated (in loop)\n");
	  terminate();
	};

	ResetByteOrder();
	packet_count = 0;
	X_packet_count = 0;
	PROG_packet_count = 0;
	X_message_count = 0;
	PROG_message_count = 0;

	orphan_windows.Rewind();
	while (orphan_windows.Next()) {
	  orphan_windows.Delete();
	};
      };
      
    } else if (FD_ISSET(X->Handle(), &fdset)) {

      // ********** Got Message From X **********

      X_packet_count++;

      if(msg.Receive(*X) == -1) {
	fprintf(stderr, "Connection reset by X server\n");
	terminate();
      };

      if (message_detail >= Packet_Detail) {
	printf("Packet %u from X of size %u\n",
	       (unsigned int)X_packet_count - 1,
	       (unsigned int)msg.Size());
      };

      if (X_message_count > 0) {
	if (!(PROG_mod & RandomLegal_Mod) && !(PROG_mod & ForceCracking_Mod)) {
	  skip_break = 1;
	} else {
	  // *** Prescan packet and see if it parses correctly ***
	  // *** If it can't be cracked, just send whole packet ***
	  temp_count = X_message_count;
	  for (unsigned index = 0u; index + 1u <= msg.Size() && skip_break == 0;
			  index += size) {
	    size = XSubLength(msg, index);
	    if (size == 0) {
	      if (message_detail >= Min_Detail) {
		printf("About to send message #%u which is of size 0!\n",
		       (unsigned int)temp_count - 1);
		skip_break = 1;
	      };
	    } else {
	      if (message_detail >= Full_Detail) {
		printf("  (pre-scan) Message code: %u, size %u\n",
		       (unsigned int)msg[index],
		       (unsigned int)size);
	      };
	      temp_count++;
	      if (index + size > msg.Size()) {
		if (message_detail >= Min_Detail) {
		  printf("About to send message #%u from X which is too big\n",
			 (unsigned int)temp_count - 1);
		  skip_break = 1;
		};
	      };
	    };
	  };
	};
      };

      if (skip_break) {
	skip_break = 0;  // Send whole packet without cracking it
	msg.Send(*PROG);
	X_message_count++;
      } else {
	for (unsigned index = 0u; index + 1u <= msg.Size(); index += size) {
	  size = XSubLength(msg, index);
	  
	  if (message_detail >= Brief_Detail) {
	    printf("  Message code (index %u): %u, size %u\n",
	      	   (unsigned int)index,
		   (unsigned int)msg[index],
		   (unsigned int)size);
	  };

	  if (index + size > msg.Size()) {
	    fprintf(stderr, "Uncrackable packet received from X\n");
	    fprintf(stderr, "Byte order: %s\n", ByteOrder());
	    msg.Dump();
	    {
	      FILE *f = fopen("bad.X.message", "w");
	      if (!f) {
		fprintf(stderr, "Unable to dump bad message\n");
	      } else {
		fprintf(f, "Byte order: %s\n", ByteOrder());
		msg.Dump(f);
		for (index = 0; index + 1 <= msg.Size(); index += size) {
		  size = XSubLength(msg, index);
		  fprintf(f, "%u: code: %u, words: %u, bytes: %u\n",
			  (unsigned int)index,
			  (unsigned int)msg[index],
			  (unsigned int)size / 4,
			  (unsigned int)size);
		};  //* endfor *
	      }; //* endif *
	    }; //* end scope *
	    terminate();
	  }; //* end error-if *

	  {
	    Message extracted_msg(msg, index, size);
	    Snoop_X_Message(extracted_msg);
	    SendMessage(extracted_msg,
		        (packet_count > wait_count) ? (Modifier)PROG_mod : No_Mod,
			PROG
	               );
	  };
	};
      };
    } else {
      fprintf(stderr, "Message received from unknown connection\n");
      terminate();
    };
  };
  return 0;
};
