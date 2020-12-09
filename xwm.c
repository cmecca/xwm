#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <signal.h>
#include "bitmaps/focus_frame_bi"
#include <stdlib.h>
#include "eventNames.h"
#define MAX_CHOICE 10
#define DRAW 1
#define ERASE 0
#define RAISE 1
#define LOWER 0
#define MOVE 1
#define RESIZE 0
#define NONE 100
#define NOTDEFINED 0
#define BLACK  1
#define WHITE  0
Window focus_window;
Window inverted_pane = NONE;
static char *menu_label[] =
  {
    "Raise",
    "Lower",
    "Move",
    "Resize",
    "CirculateDn",
    "CirculateUp",
    "(De)Iconify",
    "Kybrd Focus",
    "New Xterm",
    "Exit",
  };
Display *display;
int screen_num;
XFontStruct *font_info;
char icon_name[50];

paint_pane(window, panes, ngc, rgc, mode)
Window window;
Window panes[];
GC ngc, rgc;
int mode;
{
  int win;
  int x = 2, y;
  GC gc;
  if (mode == BLACK) {
    XSetWindowBackground(display, window, BlackPixel(display,
						     screen_num));
    gc = rgc;
  }
  else {
    XSetWindowBackground(display, window, WhitePixel(display,
						     screen_num));
    gc = ngc;
  }
  XClearWindow(display, window);
  for (win = 0; window != panes[win]; win++)
    ;
  y = font_info->max_bounds.ascent;
  XDrawString(display, window, gc, x, y, menu_label[win],
	      strlen( menu_label[win]));
}

circup(menuwin)
Window menuwin;
{
  XCirculateSubwindowsUp(display, RootWindow(display,screen_num));
  XRaiseWindow(display, menuwin);
}
circdn(menuwin)
Window menuwin;
{
  XCirculateSubwindowsDown(display, RootWindow(display,screen_num));
  XRaiseWindow(display, menuwin);
}

raise_lower( menuwin, raise_or_lower )
Window menuwin;
Bool raise_or_lower;
{
  XEvent report;
  int root_x,root_y;
  Window child, root;
  int win_x, win_y;
  unsigned int mask;
  unsigned int button;
  /* Wait for ButtonPress, find out which subwindow of root */
  XMaskEvent(display, ButtonPressMask, &report);
  button = report.xbutton.button;
  XQueryPointer(display, RootWindow(display,screen_num), &root,
		&child, &root_x, &root_y, &win_x, &win_y,
		&mask);
  /* If not RootWindow, raise */
  if (child != NULL) {
    if (raise_or_lower == RAISE)
      XRaiseWindow(display, child);
    else
      XLowerWindow(display, child);
    XRaiseWindow(display, menuwin);
  }
  while (1)  {
    XMaskEvent(display, ButtonReleaseMask, &report);
    if (report.xbutton.button == button) break;
  }
  while (XCheckMaskEvent(display, ButtonReleaseMask |
			 ButtonPressMask, &report))
    ;
}

move_resize(menuwin, hand_cursor, move_or_resize)
Window menuwin;
Cursor hand_cursor;
Bool move_or_resize;
{
  XEvent report;
  XWindowAttributes win_attr;
  int press_x, press_y, release_x, release_y, move_x, move_y;
  static int box_drawn = False;
  int left, right, top, bottom;
  Window root, child;
  Window win_to_configure;
  int win_x, win_y;
  unsigned int mask;
  unsigned int pressed_button;
  XSizeHints size_hints;
  Bool min_size, increment;
  unsigned int width, height;
  int temp_size;
  static GC gc;
  static int first_time = True;
  long user_supplied_mask;
  if (first_time) {
    gc = XCreateGC(display, RootWindow(display,screen_num), 0,
		   NULL);
    XSetSubwindowMode(display, gc, IncludeInferiors);
    XSetForeground(display, gc, BlackPixel(display, screen_num));
    XSetFunction(display, gc, GXxor);
    first_time = False;
  }
  XMaskEvent(display, ButtonPressMask, &report);
  pressed_button = report.xbutton.button;
  XQueryPointer(display, RootWindow(display,screen_num), &root,
		&child, &press_x, &press_y, &win_x,
		&win_y, &mask);
  win_to_configure = child;
  if ((win_to_configure == NULL)  ||
      ((win_to_configure == menuwin)
       && (move_or_resize == RESIZE)))  {
    while (XCheckMaskEvent(display, ButtonReleaseMask |
			   ButtonPressMask, &report))
      ;
    return;
  }

  XGetWindowAttributes(display, win_to_configure,
		       &win_attr);
  XGetWMNormalHints(display, win_to_configure, &size_hints,
		    &user_supplied_mask);
  if (size_hints.flags && PMinSize)
    min_size = True;
  if (size_hints.flags && PResizeInc)
    increment = True;
  XChangeActivePointerGrab(display, PointerMotionHintMask |
         ButtonMotionMask | ButtonReleaseMask |
			   OwnerGrabButtonMask, hand_cursor, CurrentTime);

  XGrabServer(display);
  while  (1) {
    XNextEvent(display, &report);
    switch (report.type) {
    case ButtonRelease:
      if (report.xbutton.button == pressed_button) {
	if (box_drawn)
	  draw_box(gc, left, top, right, bottom);

	XUngrabServer(display);
	XQueryPointer(display, RootWindow(display,
					  screen_num), &root, &child,
		      &release_x, &release_y, &win_x,
		      &win_y, &mask);
	if (move_or_resize == MOVE)
	  XMoveWindow(display, win_to_configure,
		      win_attr.x + (release_x -
				    press_x), win_attr.y +
		      (release_y - press_y));
	else
	  XResizeWindow(display, win_to_configure,
			win_attr.width + (release_x - press_x),
			win_attr.height + (release_y - press_y));
	XRaiseWindow(display, win_to_configure);
	XFlush(display);
	box_drawn = False;
	while (XCheckMaskEvent(display,
                     ButtonReleaseMask
			       | ButtonPressMask,
			       &report))
	  ;
	return;
      }
      break;
    case MotionNotify:
      if (box_drawn == True)
	draw_box(gc, left, top, right, bottom);

      while (XCheckTypedEvent(display, MotionNotify,
			      &report));

      XQueryPointer(display, RootWindow(display,
					screen_num), &root, &child, &move_x,
		    &move_y, &win_x, &win_y, &mask);

      if (move_or_resize == MOVE) {
	left = move_x - press_x + win_attr.x;
	top = move_y - press_y + win_attr.y;
	right = left + win_attr.width;
	bottom = top + win_attr.height;
      }
      else
	{
	  if (move_x < win_attr.x)
	    move_x = 0;
	  if (move_y < win_attr.y )
	    move_y = 0;
	  left = win_attr.x;
	  top = win_attr.y;
                  right = left + win_attr.width + move_x
		    - press_x;
                  bottom = top + win_attr.height + move_y
		    - press_y;

		  width = right - left;
		  height = bottom - top;

		  for (temp_size = size_hints.min_width;
		       temp_size < width;
		       temp_size += size_hints.width_inc)
		    ;

		  for (temp_size = size_hints.min_height;
		       temp_size < height;
		       temp_size += size_hints.height_inc)
		    ;

		  bottom = top + temp_size + 2;
		  right = left + temp_size + 2;
	}

      draw_box(gc, left, top, right, bottom);
      box_drawn = True;
      break;
    default:

      ;
    } 
  } 
} 

Display *display;
int screen;
draw_box(gc, x, y, width, height)
GC gc;
int x, y;
unsigned int width, height;
{
  XSetForeground(display, gc, BlackPixel(display,screen));
  XSetSubwindowMode(display, gc, IncludeInferiors);

  XSetFunction(display, gc, GXxor);
  XDrawRectangle(display, RootWindow(display,screen), gc, x, y,
		 width, height);
}

iconify(menuwin)
Window menuwin;
{
  XEvent report;
  extern Window focus_window;
  Window *assoc_win;
  int press_x,press_y;
  Window child;
  Window root;
  int win_x, win_y;
  unsigned int mask;
  unsigned int button;
  XMaskEvent(display, ButtonPressMask, &report);
  button = report.xbutton.button;
  XQueryPointer(display, RootWindow(display,screen_num), &root,
		&child, &press_x, &press_y, &win_x, &win_y, &mask);
  if ((child == NULL) || (child == menuwin))
    {
      while (1)  {
	XMaskEvent(display, ButtonReleaseMask, &report);
	if (report.xbutton.button == button) break;
      }
      return;
    }

  isIcon(child, press_x, press_y, &assoc_win, icon_name, True);

  XUnmapWindow(display, child);
  XMapWindow(display, *assoc_win);

  while (1)  {
    XMaskEvent(display, ButtonReleaseMask, &report);
    if (report.xbutton.button == button) break;
  }

  while (XCheckMaskEvent(display, ButtonReleaseMask |
			 ButtonPressMask, &report))
    ;
}

Window makeIcon(window, x, y, icon_name_return)
     Window window;                      
     int x, y;                             
     char *icon_name_return;
{
  int icon_x, icon_y;                 
  int icon_w, icon_h;              
  int icon_bdr;                    
  int depth;                      
  Window root;                       
  XSetWindowAttributes icon_attrib;  
  unsigned long icon_attrib_mask;
  XWMHints *wmhints;                
  XWMHints *XGetWMHints();
  Window FinishIcon();
  char *icon_name;

  if (wmhints = XGetWMHints(display, window)) {
    if (wmhints->flags&IconWindowHint)
      return(finishIcon(window, wmhints->icon_window,
			False, icon_name));
    else if (wmhints->flags&IconPixmapHint)
      {

	if (!XGetGeometry(display, wmhints->icon_pixmap,
			  &root, &icon_x, &icon_y,
			  &icon_w, &icon_h, &icon_bdr, &depth)) {
	  fprintf(stderr, "winman: client passed invalid \
                        icon pixmap." );
	  return( NULL );
	}
	else {
	  icon_attrib.background_pixmap = wmhints->icon_pixmap;
	  icon_attrib_mask = CWBorderPixel|CWBackPixmap;
	}
      }
    else {
      icon_name = getDefaultIconSize(window, &icon_w, &icon_h);
      icon_attrib_mask = CWBorderPixel | CWBackPixel;
      icon_attrib.background_pixel = (unsigned long)
	WhitePixel(display,screen_num);
    }
  }
  else {
    icon_name = getDefaultIconSize(window, &icon_w, &icon_h);
    icon_attrib_mask = CWBorderPixel | CWBackPixel;
  }
  icon_w += 2;
  icon_h += 2;
  strcpy(icon_name_return, icon_name);
  icon_bdr = 2;
  icon_attrib.border_pixel = (unsigned long)
    BlackPixel(display,screen_num);

  if (wmhints && (wmhints->flags&IconPositionHint))
    {
      icon_x = wmhints->icon_x;
      icon_y = wmhints->icon_y;
    }
  else
    {
      icon_x = x;
      icon_y = y;
    }
  return(finishIcon(window, XCreateWindow(display,
					  RootWindow(display, screen_num),
					  icon_x, icon_y, icon_w, icon_h,
					  icon_bdr, 0, CopyFromParent, CopyFromParent,
					  icon_attrib_mask, &icon_attrib),
		    True, icon_name));
}

char *
getDefaultIconSize(window, icon_w, icon_h)
     Window window;
     int *icon_w, *icon_h;
{
  char *icon_name;
  icon_name = getIconName(window);
  *icon_h = font_info->ascent + font_info->descent + 4;
  *icon_w = XTextWidth(font_info, icon_name, strlen(icon_name));
  return(icon_name);
}
char *
getIconName(window)
     Window window;
{
  char *name;
  if (XGetIconName( display, window, &name )) return( name );
  if (XFetchName( display, window, &name )) return( name );
  return( "Icon" );
}

Window finishIcon(window, icon, own, icon_name)
     Window window, icon;
     Bool own;  
     char *icon_name;
{
  WindowList win_list;
  Cursor manCursor;
  if (icon == NULL) return(NULL);

  manCursor = XCreateFontCursor(display, XC_man);
  XDefineCursor(display, icon, manCursor);

  XSelectInput(display, icon, ExposureMask);


  win_list = (WindowList) malloc(sizeof(WindowListRec));
  win_list->window = window;
  win_list->icon = icon;
  win_list->own = own;
  win_list->icon_name = icon_name;
  win_list->next = Icons;
  Icons = win_list;
  return(icon);
}

removeIcon(window)
Window window;
{
  WindowList win_list, win_list1;
  for (win_list = Icons; win_list; win_list = win_list->next)
    if (win_list->window == window) {
      if (win_list->own)
	XDestroyWindow(display, win_list->icon);
      break;
    }
  if (win_list) {
    if (win_list==Icons) Icons = Icons->next;
    else
      for (win_list1 = Icons; win_list1->next;
	   win_list1 = win_list1->next)
	if (win_list1->next == win_list) {
	  win_list1->next = win_list->next;
	  break;
	};
  }
}

focus(menuwin)
Window menuwin;
{
  XEvent report;
  int x,y;
  Window child;
  Window root;
  Window assoc_win;
  extern Window focus_window;
  int win_x, win_y;
  unsigned int mask;
  char *icon_name;
  unsigned int button;
  XWindowAttributes win_attr;
  static int old_width;
  static Window old_focus;
  int status;
  XMaskEvent(display, ButtonPressMask, &report);
  button = report.xbutton.button;
  XQueryPointer(display, RootWindow(display,screen_num), &root,
		&child, &x, &y, &win_x, &win_y, &mask);
  if ((child == NULL) || (isIcon(child, x, y, &assoc_win,
				 &icon_name)))
    focus_window = RootWindow(display, screen_num);
  else
    focus_window = child;
  if (focus_window != old_focus)  { 
    if  (old_focus != NULL)
      XSetWindowBorderWidth(display, old_focus, old_width);
    XSetInputFocus(display, focus_window, RevertToPointerRoot,
		   CurrentTime);
    if (focus_window != RootWindow(display, screen_num)) {
      if (!(status = XGetWindowAttributes(display,
					  focus_window, &win_attr)))
	fprintf(stderr, "winman: can't get attributes for \
                  focus window\n");
      XSetWindowBorderWidth(display, focus_window,
			    win_attr.border_width + 1);
      old_width = win_attr.border_width;
    }
  }
  while (1)  {
    XMaskEvent(display, ButtonReleaseMask, &report);
    if (report.xbutton.button == button) break;
  }
  old_focus = focus_window;
  return(focus_window);
}

draw_focus_frame()
{
  XWindowAttributes win_attr;
  int frame_width = 4;
  Pixmap focus_tile;
  GC gc;
  int foreground = BlackPixel(display, screen_num);
  int background = WhitePixel(display, screen_num);
  extern Window focus_window;
  Bool first_time = True;
  if (first_time) {
    focus_tile = XCreatePixmapFromBitmapData(display,
					     RootWindow(display,screen_num),
					     focus_frame_bi_bits, focus_frame_bi_width,
					     focus_frame_bi_height, foreground,
					     background, DefaultDepth(display, screen_num));

    gc = XCreateGC(display, RootWindow(display,screen_num), 0,
		   NULL);
    XSetFillStyle(display, gc, FillTiled);
    XSetTile(display, gc, focus_tile);
    first_time = False;
  }
  XClearWindow(display, RootWindow(display,screen_num));
  if (focus_window == RootWindow(display,screen_num)) return;
  XGetWindowAttributes(display, focus_window, &win_attr);
  XFillRectangle(display, RootWindow(display,screen_num), gc,
		 win_attr.x - frame_width, win_attr.y - frame_width,
		 win_attr.width + 2 * (win_attr.border_width + frame_width),
		 win_attr.height + 2 * (win_attr.border_width + frame_width));
}

main()
{
  Window menuwin;
  Window panes[MAX_CHOICE];
  int menu_width, menu_height, x = 0, y = 0, border_width = 4;
  int winindex;
  int cursor_shape;
  Cursor cursor, hand_cursor;
  char *font_name = "9x15";
  int direction, ascent, descent;
  int char_count;
  char *string;
  XCharStruct overall;
  Bool owner_events;
  int pointer_mode;
  int keyboard_mode;
  Window confine_to;
  GC gc, rgc;
  int pane_height;
  Window assoc_win;
  XEvent event;
  unsigned int button;
  if ( (display=XOpenDisplay(NULL)) == NULL ) {
    (void) fprintf( stderr, "winman: cannot connect to \
                X server %s\n", XDisplayName(NULL));
    exit( -1 );
  }
  screen_num = DefaultScreen(display);
  font_info = XLoadQueryFont(display,font_name);
  if (font_info == NULL) {
    (void) fprintf( stderr, "winman: Cannot open font %s\n",
		    font_name);
    exit( -1 );
  }
  string = menu_label[6];
  char_count = strlen(string);
  XTextExtents(font_info, string, char_count, &direction, &ascent,
	       &descent, &overall);
  menu_width = overall.width + 4;
  pane_height = overall.ascent + overall.descent + 4;
  menu_height = pane_height * MAX_CHOICE;
  x = DisplayWidth(display,screen_num) - menu_width -
    (2*border_width);
  y = 0;  

  menuwin = XCreateSimpleWindow(display, RootWindow(display,
						    screen_num), x, y, menu_width, menu_height,
				border_width, BlackPixel(display,screen_num),
				WhitePixel(display,screen_num));
  for (winindex = 0; winindex < MAX_CHOICE; winindex++) {
    panes[winindex] = XCreateSimpleWindow(display, menuwin, 0,
					  menu_height/MAX_CHOICE*winindex, menu_width,
					  pane_height, border_width = 1,
					  BlackPixel(display,screen_num),
					  WhitePixel(display,screen_num));
    XSelectInput(display, panes[winindex], ButtonPressMask
		 | ButtonReleaseMask |  ExposureMask);
  }
  XSelectInput(display, RootWindow(display, screen_num),
	       SubstructureNotifyMask);
  XMapSubwindows(display,menuwin);
  cursor = XCreateFontCursor(display, XC_left_ptr);
  hand_cursor = XCreateFontCursor(display, XC_hand2);
  XDefineCursor(display, menuwin, cursor);
  focus_window = RootWindow(display, screen_num);
  
  gc = XCreateGC(display, RootWindow(display, screen_num), 0,
		 NULL);
  XSetForeground(display, gc, BlackPixel(display, screen_num));
  rgc = XCreateGC(display, RootWindow(display, screen_num), 0,
		  NULL);
  XSetForeground(display, rgc, WhitePixel(display, screen_num));
  XMapWindow(display, menuwin);

  if ((fcntl(ConnectionNumber(display), F_SETFD, 1)) == -1)
    fprintf(stderr, "winman: child cannot disinherit TCP fd");
  while (1) {
    XNextEvent(display, &event);
    switch (event.type) {
    case Expose:
      if  (isIcon(event.xexpose.window, event.xexpose.x,
		  event.xexpose.y, &assoc_win, icon_name, False))
	XDrawString(display, event.xexpose.window, gc, 2,
		    ascent + 2, icon_name, strlen(icon_name));
      else {
	if (inverted_pane == event.xexpose.window)
	  paint_pane(event.xexpose.window, panes, gc, rgc,
		     BLACK);
	else
	  paint_pane(event.xexpose.window, panes, gc, rgc,
		     WHITE);
      }
      break;
    case ButtonPress:
      paint_pane(event.xbutton.window, panes, gc, rgc, BLACK);
      button = event.xbutton.button;
      inverted_pane = event.xbutton.window;
      while (1) {
	while (XCheckTypedEvent(display, ButtonPress,
				&event));
	XMaskEvent(display, ButtonReleaseMask, &event);
	if (event.xbutton.button == button)
	  break;
      }
      
      owner_events = True;
      
      pointer_mode = GrabModeAsync;
      keyboard_mode = GrabModeAsync;
      confine_to = None;
      XGrabPointer(display, menuwin, owner_events,
		   ButtonPressMask | ButtonReleaseMask,
		   pointer_mode, keyboard_mode,
		   confine_to, hand_cursor, CurrentTime);
      
      if (inverted_pane == event.xbutton.window) {
	for (winindex = 0; inverted_pane !=
	       panes[winindex]; winindex++)
	  ;
	switch (winindex) {
	case 0:
	  raise_lower(menuwin, RAISE);
	  break;
	case 1:
	  raise_lower(menuwin, LOWER);
	  break;
	case 2:
	  move_resize(menuwin, hand_cursor, MOVE);
	  break;
	case 3:
	  move_resize(menuwin, hand_cursor, RESIZE);
	  break;
	case 4:
	  circup(menuwin);
	  break;
	case 5:
	  circdn(menuwin);
	  break;
	case 6:
	  iconify(menuwin);
	  break;
	case 7:
	  focus_window = focus(menuwin);
	  break;
	case 8:
	  execute("xterm&");
	  break;
	case 9: 
	  XSetInputFocus(display,
			 RootWindow(display,screen_num),
			 RevertToPointerRoot,
			 CurrentTime);
	  
	  XClearWindow(display, RootWindow(display,
					   screen_num));
	  XFlush(display);
	  XCloseDisplay(display);
	  exit(1);
	default:
	  (void) fprintf(stderr,
			 "Something went wrong\n");
	  break;
	} 
      } 
      
      paint_pane(event.xexpose.window, panes, gc, rgc, WHITE);
      inverted_pane = NONE;
      draw_focus_frame();
      XUngrabPointer(display, CurrentTime);
      XFlush(display);
      break;
    case DestroyNotify:
      
      removeIcon(event.xdestroywindow.window);
      break;
    case CirculateNotify:
    case ConfigureNotify:
    case UnmapNotify:
      
      draw_focus_frame();
      break;
    case CreateNotify:
    case GravityNotify:
    case MapNotify:
    case ReparentNotify:
      
      break;
    case ButtonRelease:
      break;
    case MotionNotify:
      break;
    default:
      fprintf(stderr, "winman: got unexpected %s event.\n",
	      event_names[event.type]);
    } 
  } 
} 