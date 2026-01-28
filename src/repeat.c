#include "tvi.h"

/*** repeat last action ***/

void editorRecordAction(int type, int count, int motion, char *text, size_t text_len, int line_count) {
  E.last_action_type = type;
  E.last_action_count = count;
  E.last_action_motion = motion;
  if (text && text_len > 0 && text_len < sizeof(E.last_action_text)) {
    memcpy(E.last_action_text, text, text_len);
    E.last_action_text[text_len] = '\0';
    E.last_action_text_len = text_len;
  } else {
    E.last_action_text[0] = '\0';
    E.last_action_text_len = 0;
  }
  E.last_action_line_count = line_count;
}

void editorRepeatLastAction() {
  int i;
  int count = E.last_action_count > 0 ? E.last_action_count : 1;

  switch (E.last_action_type) {
    case UNDO_INSERT_LINE:
      for (i = 0; i < count; i++) {
        editorInsertRow(E.cy + 1, "", 0);
        E.cy++;
      }
      E.cx = 0;
      break;

    case UNDO_DELETE_LINE:
      for (i = 0; i < count && E.cy < E.numrows; i++) {
        editorDeleteLines(1);
      }
      break;

    case UNDO_INSERT_CHAR:
      for (i = 0; i < count && E.last_action_text_len > 0; i++) {
        int j;
        for (j = 0; j < (int)E.last_action_text_len; j++) {
          editorInsertChar(E.last_action_text[j]);
        }
      }
      break;

    case UNDO_DELETE_CHAR:
      for (i = 0; i < count && E.cy < E.numrows; i++) {
        if (E.cx > 0) {
          editorDelChar();
        }
      }
      break;

    case UNDO_MODIFY_LINE:
      for (i = 0; i < count && E.cy < E.numrows; i++) {
        //editorSetStatusMessage("-- INSERT --");
        E.mode = INSERT;
        E.bold = 1;
      }
      break;

    default:
      editorSetStatusMessage("Nothing to repeat");
      return;
  }

  editorSetStatusMessage("Repeated %d time(s)", count);
}
