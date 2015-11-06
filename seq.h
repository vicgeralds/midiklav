extern int port_lower;	/* lower keys output */
extern int port_upper;	/* upper keys output */

int open_seq();

/* Send note on/off event. */
int send_note(int port, int on, int channel, int note, int velocity);
