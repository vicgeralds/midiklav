#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>		/* XA_STRING */
#include <X11/keysym.h>
#include <X11/XKBlib.h>		/* XkbSetDetectableAutoRepeat */
#include <stdio.h>
#include "keymap_se.h"
#include "seq.h"

#define PANEL_HEIGHT 32

#define BLACK BlackPixel(dpy, DefaultScreen(dpy))
#define WHITE WhitePixel(dpy, DefaultScreen(dpy))

/* allocated colors */
enum {
	GREY,
	BLUE,
	RED,
	PANEL1,
	PANEL2,
	PANEL3,
	PANEL4,
	NUM_PIXELS
};

static unsigned long pixels[NUM_PIXELS] = {
	0x555555,	/* GREY */
	0x0000ff,	/* BLUE */
	0xff0000,	/* RED */
	0x433900,	/* PANEL1 */
	0xb8c76f,	/* PANEL2 */
	0x9a6759,	/* PANEL3 */
	0x9ad284	/* PANEL4 */
};

typedef unsigned char uchar;

static Display *dpy;
static Pixmap keybd_pixmap;

/* width and height of each white key */
static int keyw = 18;
static int keyh = 72;

/* one extra element for alt. last note */
static char keylabels[46];
static const uchar keycodes[46] = KEYCODES;

static uchar keystate[6];

struct notectl {
	int channel;
	int base_note;
	int velocity;
};

static struct notectl lower = {0, 48, 127};
static struct notectl upper = {0, 72, 127};

/* get autorepeat as press--press--release
 * instead of press--release--press--release */
static void set_detectable_autorepeat()
{
	Bool supported;
	XkbSetDetectableAutoRepeat(dpy, True, &supported);
}

static Colormap init_colors() {
	Colormap colormap = DefaultColormap(dpy, DefaultScreen(dpy));
	int i;
	for (i = 0; i < NUM_PIXELS; i++) {
		XColor c;
		c.red   = 257 * ((pixels[i] >> 16) & 0xff);
		c.green = 257 * ((pixels[i] >> 8) & 0xff);
		c.blue  = 257 * ((pixels[i]) & 0xff);
		XAllocColor(dpy, colormap, &c);
		pixels[i] = c.pixel;
	}
	return colormap;
}

static void set_wm_stuff(Window win)
{
	XTextProperty title;
	XSizeHints *hints;
	Atom wm_delete_window;

	title.value = (uchar *) "midiklaviatur";
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = 13;	/* string length */
	XSetWMName(dpy, win, &title);

	hints = XAllocSizeHints();
	hints->flags = PMinSize | PResizeInc | PWinGravity;
	hints->min_width   = 312;
	hints->min_height  = 41 + PANEL_HEIGHT;
	hints->width_inc   = 26;
	hints->height_inc  = 16;
	hints->win_gravity = 8;	/* south */
	XSetWMNormalHints(dpy, win, hints);
	XFree(hints);

	wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);
}

static Window createwin()
{
	unsigned long grey = pixels[GREY];
	Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
					 468, 73+PANEL_HEIGHT, 0, grey, grey);
	set_wm_stuff(win);
	XSelectInput(dpy, win, StructureNotifyMask | ExposureMask |
			       KeymapStateMask | KeyPressMask|KeyReleaseMask);
	XMapWindow(dpy, win);
	return win;
}

static Pixmap createpixmap(unsigned w, unsigned h, unsigned long bg)
{
	int depth = DefaultDepth(dpy, DefaultScreen(dpy));
	Pixmap pixm = XCreatePixmap(dpy, DefaultRootWindow(dpy), w, h, depth);
	GC gc = XCreateGC(dpy, pixm, 0, NULL);
	XSetForeground(dpy, gc, bg);
	XFillRectangle(dpy, pixm, gc, 0, 0, w, h);
	XFreeGC(dpy, gc);
	return pixm;
}

static void create_keybd_pixmap()
{
	keybd_pixmap = createpixmap(keyw*26, keyh+1, pixels[GREY]);
}

static void draw_ivory(int i, int pressed, int x, int h, GC gc)
{
	int y = ++h;
	int x2 = x + keyw-1;
	int d, w;
	if (i==0 || i==6) {
		d = 0;
		w = (keyw * 6/10) - 1;
	} else {
		d = (keyw * 6/10) + (keyw * 2/3) - (keyw-1);
		if (i==4 || i==12) {
			y = 0;
			w = keyw-1-d;
		} else
			w = (keyw * 6/10)-1-d;
	}
	XSetForeground(dpy, gc, BLACK);
	XDrawLine(dpy, keybd_pixmap, gc, x2, y, x2, keyh-1);
	XSetForeground(dpy, gc, pressed ? pixels[BLUE] : WHITE);
	XFillRectangle(dpy, keybd_pixmap, gc, x+d, 0, w, h);
	XFillRectangle(dpy, keybd_pixmap, gc, x, h, keyw-1, keyh-h);
}

static void draw_ebony(int i, int pressed, int x, int h, GC gc)
{
	int w = keyw * 2/3;
	XSetForeground(dpy, gc, pressed ? pixels[BLUE] : BLACK);
	XFillRectangle(dpy, keybd_pixmap, gc, x+1, 0, w-1, h);
	XSetForeground(dpy, gc, WHITE);
	XDrawLine(dpy, keybd_pixmap, gc, x-1, 0, x-1, h);
	XDrawLine(dpy, keybd_pixmap, gc, x+w, 0, x+w, h);
	XDrawLine(dpy, keybd_pixmap, gc, x, h, x+w, h);
}

/* first key has index 7 (note G, "<" key next to SHIFT) */
static int draw_key(int i, int pressed, GC gc)
{
	int j = i % 12;
	int x, y;
	int eb_h = keyh * 4/7;
	int labelx;
	unsigned long color;

	if (j >= 5)
		j++;
	x = ((i/12) * 7 - 4 + j/2) * keyw;
	if (j % 2 == 0) {
		draw_ivory(j, pressed, x, eb_h, gc);
		labelx = x + keyw/2 - 3;
		y = eb_h + 12;
		color = BLACK;
	} else {
		x += keyw * 6/10;
		if (x > keyw * 25)
			return 0;
		draw_ebony(j, pressed, x, eb_h, gc);
		labelx = x + keyw/3 - 2;
		y = eb_h - 6;
		color = pixels[RED];
	}
	if (pressed)
		color = WHITE;
	XSetForeground(dpy, gc, color);
	i -= 7;
	XDrawString(dpy, keybd_pixmap, gc, labelx, y, keylabels+i, 1);
	return x;
}

/* checks if c is a printable character in ISO 8859-1 */
static int isprint_latin1(unsigned char c) {
	return c > 0x1f && c < 0x7f || c > 0xa0;
}

static void draw_keyboard(int w, Window win, GC gc)
{
	int i = 0;
	while (i < 44) {
		KeySym keysym = XkbKeycodeToKeysym(dpy, keycodes[i], 0, 0);

		/* convert keysym to string with Caps Lock on */
		if (!XkbTranslateKeySym(dpy, &keysym, LockMask, keylabels + i, 1, NULL) ||
		    !isprint_latin1(keylabels[i]))
			keylabels[i] = ' ';

		draw_key(i+7, keystate[i/8] & 1<<(i%8), gc);
		i++;
	}
	XSetForeground(dpy, gc, BLACK);
	XDrawLine(dpy, keybd_pixmap, gc, 0, keyh, w, keyh);
	XCopyArea(dpy, keybd_pixmap, win, gc, 0,0,w, keyh+1, 0,PANEL_HEIGHT);
}

static void draw_panel_keys(int x1, int x2, int w, Window win, GC gc)
{
	unsigned long bg;
	char s[2] = "F";
	int x, i;
	bg = pixels[PANEL3];
	for (i=0; i<8; i++) {
		x = 4;
		if (i >= 4)
			x += w/2;
		if (i/2 % 2)
			x += x1;
		if (i % 2)
			x += 15;
		XSetForeground(dpy, gc, bg);
		XFillRectangle(dpy, win, gc, x, 17, 13, 10);
		s[1] = i+'1';
		XSetForeground(dpy, gc, BLACK);
		XDrawString(dpy, win, gc, x+1, 25, s, 2);
	}
	XSetForeground(dpy, gc, bg);
	x = x2+4;
	XFillRectangle(dpy, win, gc, x, 17, 24, 10);
	XFillRectangle(dpy, win, gc, x+w/2, 17, 20, 10);
	XSetForeground(dpy, gc, BLACK);
	XDrawString(dpy, win, gc, x+1, 25, "Ctrl", 4);
	XDrawString(dpy, win, gc, x+1+w/2, 25, "Alt", 3);
}

static void display_base_octave(const struct notectl *note,
				int xpos, Window win, GC gc)
{
	static int xx;
	char s[2] = "0";
	int x;
	s[1] = note->base_note / 12 + '0';
	if (s[1] == '0'-1) {
		s[0] = '-';
		s[1] = '1';
	}
	if (xpos)
		xx = xpos;
	x = xx;
	if (note == &upper)
		x += keyw * 13;
	XSetForeground(dpy, gc, BLACK);
	XFillRectangle(dpy, win, gc, x, 16, 17, 12);
	XSetForeground(dpy, gc, pixels[PANEL4]);
	XDrawString(dpy, win, gc, x+3, 25, s, 2);
}

static void display_velocity(const struct notectl *note,
			     int xpos, Window win, GC gc)
{
	static int xx;
	char s[4];
	int x;
	sprintf(s, "%03d", note->velocity);
	if (xpos)
		xx = xpos;
	x = xx;
	if (note == &upper)
		x += keyw * 13;
	XSetForeground(dpy, gc, BLACK);
	XFillRectangle(dpy, win, gc, x, 16, 23, 12);
	XSetForeground(dpy, gc, pixels[PANEL4]);
	XDrawString(dpy, win, gc, x+3, 25, s, 3);
}

static void display_channel(const struct notectl *note,
			    int xpos, Window win, GC gc)
{
	static int xx;
	char s[3];
	int x;
	sprintf(s, "%02d", note->channel);
	if (xpos)
		xx = xpos;
	x = xx;
	if (note == &upper)
		x += keyw * 13;
	XSetForeground(dpy, gc, BLACK);
	XFillRectangle(dpy, win, gc, x, 16, 17, 12);
	XSetForeground(dpy, gc, pixels[PANEL4]);
	XDrawString(dpy, win, gc, x+3, 25, s, 2);
}

static void draw_panel(int w, Window win, GC gc)
{
	int x;
	int x1 = w/6;
	int x2 = w/3;
	int n1 = 11;
	int n2 = 8;
	int n3 = 7;

	XSetForeground(dpy, gc, pixels[PANEL2]);
	XDrawLine(dpy, win, gc, 2, PANEL_HEIGHT-1, w-2, PANEL_HEIGHT-1);

	if (keyw < 17)
		n1 = 8;
	for (x = 6; x < w; x += w/2) {
		XDrawString(dpy, win, gc, x, 13, "Base octave", n1);
		XDrawString(dpy, win, gc, x+x1, 13, "Velocity", n2);
		XDrawString(dpy, win, gc, x+x2, 13, "Channel", n3);
	}
	XSetForeground(dpy, gc, BLACK);
	x = w/2 - 1;
	XDrawLine(dpy, win, gc, x, 2, x, PANEL_HEIGHT-4);

	if (keyw >= 14) {
		draw_panel_keys(x1, x2, w, win, gc);
		x = 37;
	} else
		x = 11;
	display_base_octave(&lower, x, win, gc);
	display_velocity(&lower, x1+x, win, gc);
	display_channel(&lower, x2+x, win, gc);

	display_base_octave(&upper, 0, win, gc);
	display_velocity(&upper, 0, win, gc);
	display_channel(&upper, 0, win, gc);
}

static int resizewin(XEvent *e, GC gc)
{
	unsigned w = e->xconfigure.width;
	unsigned h = e->xconfigure.height;
	int kw = w/26;
	int kh = h-1 - PANEL_HEIGHT;
	if (kw == keyw && kh == keyh)
		return 0;
	while (XPending(dpy)) {
		XNextEvent(dpy, e);
		if (e->type == ConfigureNotify)
			return resizewin(e, gc);
		if (e->type != Expose ) {
			XPutBackEvent(dpy, e);
			break;
		}
	}
	XFreePixmap(dpy, keybd_pixmap);
	keyw = kw;
	keyh = kh;
	create_keybd_pixmap();
	return 1;
}

static void redraw_rect(const XExposeEvent *e, GC gc)
{
	int x = e->x;
	int y = e->y;
	unsigned w = e->width;
	unsigned h = e->height;
	if (y < PANEL_HEIGHT) {
		XSetForeground(dpy, gc, pixels[PANEL1]);
		XFillRectangle(dpy, e->window, gc, x, y, w, PANEL_HEIGHT-y);
		draw_panel(keyw*26, e->window, gc);
	}
	if (y + h > PANEL_HEIGHT) {
		y -= PANEL_HEIGHT;
		if (y < 0) {
			h += y;
			y = 0;
		}
		XCopyArea(dpy, keybd_pixmap, e->window, gc,
			  x, y, w, h, x, y+PANEL_HEIGHT);
	}
}

static int translate_keycode(unsigned keycode)
{
	int i = 0;
	while (i < 46) {
		if (keycode == keycodes[i])
			return i+7;
		i++;
	}
	return 0;
}

static int sendnot(int i, int pressed)
{
	int port;
	const struct notectl *note;
	if (i <= 28) {
		port = port_lower;
		note = &lower;
	} else {
		if (i > 7+45)
			return 0;
		if (i == 7+45)
			i = 7+44;
		port = port_upper;
		note = &upper;
		i -= 24;
	}
	i += note->base_note;
	if (i < 0 || i > 127)
		return 0;
	send_note(port, pressed, note->channel, i, note->velocity);
	return 1;
}

static void update_keystate(int i, int pressed, Window win, GC gc)
{
	uchar *byte;
	uchar bit;
	int x;
	byte = keystate + (i-7)/8;
	bit = 1 << (i-7) % 8;
	if (pressed && *byte & bit || !sendnot(i, pressed))
		return;
	if (pressed)
		*byte |= bit;
	else
		*byte &= ~bit;
	x = draw_key(i, pressed, gc);
	XCopyArea(dpy, keybd_pixmap, win, gc, x,0,keyw,keyh+1, x,PANEL_HEIGHT);
}

static void change_note_control(KeySym keysym, int i, unsigned mod,
				Window win, GC gc)
{
	struct notectl *note = i==0 ? &lower : &upper;
	i = 0;
	switch (keysym) {
	case XK_F1:
	case XK_F5:
		if (note->base_note >= 0) {
			note->base_note -= 12;
			i = 1;
		}
		break;
	case XK_F2:
	case XK_F6:
		if (note->base_note <= 104) {
			note->base_note += 12;
			i = 1;
		}
		break;
	case XK_F3:
	case XK_F7:
		if (note->velocity > 0) {
			if (mod & ShiftMask)
				note->velocity--;
			else {
				int vel = note->velocity - 10;
				note->velocity = vel < 0 ? 0 : vel;
			}
			i = 2;
		}
		break;
	case XK_F4:
	case XK_F8:
		if (note->velocity < 127) {
			if (mod & ShiftMask)
				note->velocity++;
			else {
				int vel = note->velocity + 10;
				note->velocity = vel > 127 ? 127 : vel;
			}
			i = 2;
		}
		break;
	/* Ctrl or Alt pressed */
	case XK_plus:
	case XK_KP_Add:
		note->channel++;
		if (note->channel > 15)
			note->channel = 0;
		i = 3;
		break;
	case XK_minus:
	case XK_KP_Subtract:
		note->channel--;
		if (note->channel < 0)
			note->channel = 15;
		i = 3;
		break;
	default:
		if (keysym >= XK_0 && keysym <= XK_9) {
			note->channel = keysym % 0x10;
			i = 3;
		}
	}
	switch (i) {
	case 1:
		display_base_octave(note, 0, win, gc);
		break;
	case 2:
		display_velocity(note, 0, win, gc);
		break;
	case 3:
		display_channel(note, 0, win, gc);
	}
}

static int keypressrelease(XKeyEvent *e, GC gc)
{
	int i = translate_keycode(e->keycode);
	unsigned mod = e->state & (ShiftMask | ControlMask | Mod1Mask);
	KeySym keysym;
	if (e->type == KeyPress && (!i || mod)) {
		keysym = XLookupKeysym(e, 0);
		i = 1;
		if (mod & ControlMask)
			i = 0;
		else if (!(mod & Mod1Mask))
			switch (keysym) {
			case XK_Escape:
				return 0;
			case XK_F1:
			case XK_F2:
			case XK_F3:
			case XK_F4:
				i = 0;
			case XK_F5:
			case XK_F6:
			case XK_F7:
			case XK_F8:
				break;
			default:
				return 1;
			}
		change_note_control(keysym, i, mod, e->window, gc);
	} else if (i)
		update_keystate(i, e->type == KeyPress, e->window, gc);
	return 1;
}

static void keyboard_state_changed(char *key_vector, Window win, GC gc)
{
	uchar i = 0;
	uchar keycode;
	char pressed;
	while (i < 46) {
		keycode = keycodes[i];
		if (keycode) {
			pressed = key_vector[keycode/8] & 1<<(keycode%8);
			if (pressed != (keystate[i/8] & 1<<(i%8)))
				update_keystate(i+7, pressed, win, gc);
		}
		i++;
	}
}

static int wm_close(const XClientMessageEvent *e)
{
	Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	return e->data.l[0] == wm_delete_window;
}

static int process_event(Window win, GC gc)
{
	XEvent e;
	int width;
	XNextEvent(dpy, &e);
	switch (e.type) {
	case ConfigureNotify:
		if (!resizewin(&e, gc))
			break;
	case MapNotify:
		width = keyw * 26;
		XSetForeground(dpy, gc, pixels[PANEL1]);
		XFillRectangle(dpy, win, gc, 0, 0, width, PANEL_HEIGHT);
		draw_panel(width, win, gc);
		draw_keyboard(width, win, gc);
		while (XPending(dpy)) {
			XPeekEvent(dpy, &e);
			if (e.type != Expose)
				break;
			XNextEvent(dpy, &e);
		}
		break;
	case Expose:
		redraw_rect(&e.xexpose, gc);
		break;
	case KeyPress:
	case KeyRelease:
		if (!keypressrelease(&e.xkey, gc))
			return 0;
		break;
	case KeymapNotify:
		keyboard_state_changed(e.xkeymap.key_vector, win, gc);
		break;
	case ClientMessage:
		if (wm_close(&e.xclient))
			return 0;

	}
	return 1;
}

int main(int argc, char **argv)
{
	Window win;
	GC gc;
	XFontStruct *font;
	Colormap colormap;

	dpy = XOpenDisplay(NULL);
	if (!dpy || !open_seq())
		return 1;
	set_detectable_autorepeat();
	/* make sure latin1 is used when converting keysym to string */
	XkbSetXlibControls(dpy, XkbLC_ForceLatin1Lookup, XkbLC_ForceLatin1Lookup);

	colormap = init_colors();
	win = createwin();
	create_keybd_pixmap();
	gc = XCreateGC(dpy, win, 0, NULL);
	font = XLoadQueryFont(dpy, "*-fixed-*-10-*-iso8859-1");
	XSetFont(dpy, gc, font->fid);

	while (process_event(win, gc))
		;
	XFreeGC(dpy, gc);
	XFreeFont(dpy, font);
	XFreePixmap(dpy, keybd_pixmap);
	XFreeColors(dpy, colormap, pixels, NUM_PIXELS, 0);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	return 0;
}
