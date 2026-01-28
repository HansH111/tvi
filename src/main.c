#include "tvi.h"
#include <string.h>

/*** main ***/

struct editorConfig E;

int main(int argc, char *argv[]) {
  int argn=1;
  if (argc >= 2 && strcmp(argv[argn], "-i") == 0) {
      initEditor(0);
      initRedirectInput(argv[argn+1]);
      argn+=2;
  } else {
      enableRawMode();      
      initEditor(1);      
  }
  if (argn < argc) {
     editorOpen(argv[argn]);
//     if (E.filename) editorSetStatusMessage("\"%s\" %dL, %dB", E.filename, E.numrows, E.size);
  }
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
