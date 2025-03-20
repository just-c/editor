#include "action.h"

#include <stdlib.h>

#include "editor.h"
#include "terminal.h"

bool editorUndo(void) {
  if (current_file->action_current == current_file->action_head) return false;

  switch (current_file->action_current->action->type) {
    case ACTION_EDIT: {
      EditAction* edit = &current_file->action_current->action->edit;
      editorDeleteText(edit->added_range);
      editorPasteText(&edit->deleted_text, edit->deleted_range.start_x,
                      edit->deleted_range.start_y);
      current_file->cursor = edit->old_cursor;
    } break;

    case ACTION_ATTRI: {
      AttributeAction* attri = &current_file->action_current->action->attri;
      current_file->newline = attri->old_newline;
    } break;
  }

  current_file->action_current = current_file->action_current->prev;
  current_file->dirty--;
  return true;
}

bool editorRedo(void) {
  if (!current_file->action_current->next) return false;

  current_file->action_current = current_file->action_current->next;

  switch (current_file->action_current->action->type) {
    case ACTION_EDIT: {
      EditAction* edit = &current_file->action_current->action->edit;
      editorDeleteText(edit->deleted_range);
      editorPasteText(&edit->added_text, edit->added_range.start_x,
                      edit->added_range.start_y);
      current_file->cursor = edit->new_cursor;
    } break;

    case ACTION_ATTRI: {
      AttributeAction* attri = &current_file->action_current->action->attri;
      current_file->newline = attri->new_newline;
    } break;
  }

  current_file->dirty++;
  return true;
}

void editorAppendAction(EditorAction* action) {
  if (!action) return;

  EditorActionList* node = malloc_s(sizeof(EditorActionList));
  node->action = action;
  node->next = NULL;

  current_file->dirty++;

  editorFreeActionList(current_file->action_current->next);

  if (current_file->action_current == current_file->action_head) {
    current_file->action_head->next = node;
    node->prev = current_file->action_head;
    current_file->action_current = node;
    return;
  }

  node->prev = current_file->action_current;
  current_file->action_current->next = node;
  current_file->action_current = current_file->action_current->next;
}

void editorFreeAction(EditorAction* action) {
  if (!action) return;

  if (action->type == ACTION_EDIT) {
    editorFreeClipboardContent(&action->edit.deleted_text);
    editorFreeClipboardContent(&action->edit.added_text);
  }

  free(action);
}

void editorFreeActionList(EditorActionList* thisptr) {
  EditorActionList* temp;
  while (thisptr) {
    temp = thisptr;
    thisptr = thisptr->next;
    editorFreeAction(temp->action);
    free(temp);
  }
}
