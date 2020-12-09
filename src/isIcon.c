#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
extern Display *display;
extern int screen_num;

typedef struct _windowList {
  struct _windowList *next;
  Window window;
  Window icon;
  Bool own;
  char *icon_name;
} WindowListRec, *WindowList;

WindowList Icons = NULL;
Bool isIcon(win, x, y, assoc, icon_name, makeicon)
     Window win;
     int x, y;
     Window *assoc;
     char *icon_name;
     Bool makeicon;
{
  WindowList win_list;
  Window makeIcon();
  for (win_list = Icons; win_list; win_list = win_list->next) {
    if (win == win_list->icon) {
      *assoc = win_list->window;
      strcpy(icon_name, win_list->icon_name);
      return(True);
    }
    if (win == win_list->window) {
      *assoc = win_list->icon;
      strcpy(icon_name, win_list->icon_name);
      return(False);
    }
  }

  if (makeicon) {
    *assoc = makeIcon(win, x, y, icon_name);
    XAddToSaveSet(display, win);
  }
  return(False);
}