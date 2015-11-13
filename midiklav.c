#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>		/* XA_STRING */
#include <X11/keysym.h>
#include <X11/XKBlib.h>		/* XkbSetDetectableAutoRepeat */
#include <stdio.h>
#include "seq.h"
#include "notes.h"

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

static char keylabels[NUM_KEYS];
static uchar keycodes[NUM_KEYS];

/* bit arrays indicating which keys are pressed */
static unsigned long keystate[2];

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

static void draw_ivory(enum note note, int pressed, int x, int h, GC gc)
{
	int y = ++h;
	int x2 = x + keyw-1;
	int d, w;
	if (note == C || note == F) {
		d = 0;
		w = (keyw * 6/10) - 1;
	} else {
		d = (keyw * 6/10) + (keyw * 2/3) - (keyw-1);
		if (note == E || note == B) {
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

static void draw_ebony(int pressed, int x, int h, GC gc)
{
	int w = keyw * 2/3;
	XSetForeground(dpy, gc, pressed ? pixels[BLUE] : BLACK);
	XFillRectangle(dpy, keybd_pixmap, gc, x+1, 0, w-1, h);
	XSetForeground(dpy, gc, WHITE);
	XDrawLine(dpy, keybd_pixmap, gc, x-1, 0, x-1, h);
	XDrawLine(dpy, keybd_pixmap, gc, x+w, 0, x+w, h);
	XDrawLine(dpy, keybd_pixmap, gc, x, h, x+w, h);
}

static int draw_key(int i, int pressed, GC gc)
{
	enum note note = i % 14;
	int x = keyw * (i/2);
	int y = keyh * 4/7;

	/* not drawing duplicated keys */
	if (note == B_SHARP || note == E_SHARP)
		return x;

	if (IS_WHITE_KEY(note)) {
		draw_ivory(note, pressed, x, y, gc);
		XSetForeground(dpy, gc, pressed ? WHITE : BLACK);
		XDrawString(dpy, keybd_pixmap, gc, x + keyw/2 - 3, y + 12, keylabels+i, 1);
	} else {
		x += keyw * 6/10;
		draw_ebony(pressed, x, y, gc);
		XSetForeground(dpy, gc, pressed ? WHITE : pixels[RED]);
		XDrawString(dpy, keybd_pixmap, gc, x + keyw/3 - 2, y - 6, keylabels+i, 1);
	}
	return x;
}

/* checks if c is a printable character in ISO 8859-1 */
static int isprint_latin1(unsigned char c) {
	return c > 0x1f && c < 0x7f || c > 0xa0;
}

static void draw_keyboard(int w, Window win, GC gc)
{
	int i = 0;
	while (i < NUM_KEYS) {
		KeySym keysym = XkbKeycodeToKeysym(dpy, keycodes[i], 0, 0);

		/* convert keysym to string with Caps Lock on */
		if (!XkbTranslateKeySym(dpy, &keysym, LockMask, keylabels + i, 1, NULL) ||
		    !isprint_latin1(keylabels[i]))
			keylabels[i] = ' ';

		draw_key(i, (keystate[i & 1] >> i/2) & 1, gc);
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
	/* map keycodes to notes based on "standard" XFree86 codes */
	static const signed char notes[] = {
		NOTE_AE01, NOTE_AE02, NOTE_AE03, NOTE_AE04, NOTE_AE05, NOTE_AE06,
		NOTE_AE07, NOTE_AE08, NOTE_AE09, NOTE_AE10, NOTE_AE11, NOTE_AE12,
		NOTE_BKSP, NOTE_TAB,
		NOTE_AD01, NOTE_AD02, NOTE_AD03, NOTE_AD04, NOTE_AD05, NOTE_AD06,
		NOTE_AD07, NOTE_AD08, NOTE_AD09, NOTE_AD10, NOTE_AD11, NOTE_AD12,
		NOTE_RTRN, -1,
		NOTE_AC01, NOTE_AC02, NOTE_AC03, NOTE_AC04, NOTE_AC05, NOTE_AC06,
		NOTE_AC07, NOTE_AC08, NOTE_AC09, NOTE_AC10, NOTE_AC11,
		-1, -1,
		NOTE_BKSL,
		NOTE_AB01, NOTE_AB02, NOTE_AB03, NOTE_AB04, NOTE_AB05, NOTE_AB06,
		NOTE_AB07, NOTE_AB08, NOTE_AB09, NOTE_AB10, NOTE_RTSH
	};

	if (keycode == 94)
		return NOTE_LSGT;

	if ((keycode -= 10) < sizeof(notes))
		return notes[keycode];

	return -1;
}

static void init_keycodes()
{
	unsigned kc;
	for (kc = 10; kc <= 62; kc++) {
		int i = translate_keycode(kc);
		if (i >= 0)
			keycodes[i] = kc;
	}

	keycodes[NOTE_LSGT] = 94;
}

static int note_value(enum note note) {
	switch (note) {
	case C: return 0;
	case C_SHARP: return 1;
	case D: return 2;
	case D_SHARP: return 3;
	case E: return 4;
	case E_SHARP:
	case F: return 5;
	case F_SHARP: return 6;
	case G: return 7;
	case G_SHARP: return 8;
	case A: return 9;
	case A_SHARP: return 10;
	case B: return 11;
	case B_SHARP: return 12;
	}
	return -1;
}

static int to_midi_note(int i)
{
	enum note note = i % 14;
	i /= 14;
	i += C > 0 && note >= C;
	i *= 12;

	return i + note_value(note);
}


static int play_note(int i, int pressed)
{
	int port;
	const struct notectl *note;
	if (i < NUM_KEYS/2) {
		port = port_lower;
		note = &lower;
	} else {
		port = port_upper;
		note = &upper;
		i -= 28;
	}
	i = to_midi_note(i);
	i += note->base_note;
	if (i < 0 || i > 127)
		return 0;
	send_note(port, pressed, note->channel, i, note->velocity);
	return 1;
}

static void update_keystate(int i, int pressed, Window win, GC gc)
{
	unsigned long bit = 1L << i/2;
	int x;
	if (pressed && ((keystate[i & 1] >> i/2) & 1) || !play_note(i, pressed))
		return;
	if (pressed)
		keystate[i & 1] |= bit;
	else
		keystate[i & 1] &= ~bit;
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
	unsigned mod = e->state & (ControlMask | Mod1Mask);
	KeySym keysym;

	if (e->type == KeyPress && (i < 0 || mod)) {
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
		change_note_control(keysym, i, e->state, e->window, gc);
	} else if (i >= 0)
		update_keystate(i, e->type == KeyPress, e->window, gc);
	return 1;
}

static void keyboard_state_changed(char *key_vector, Window win, GC gc)
{
	uchar i = 0;
	uchar keycode;
	char pressed;
	while (i < NUM_KEYS) {
		keycode = keycodes[i];
		if (keycode) {
			pressed = key_vector[keycode/8] & 1<<(keycode%8);
			if (pressed || ((keystate[i & 1] >> i/2) & 1))
				update_keystate(i, pressed, win, gc);
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
	init_keycodes();

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
