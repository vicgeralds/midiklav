enum port {
	PORT_LOWER,	/* lower keys output */
	PORT_UPPER	/* upper keys output */
};

int open_seq();

/* Send note on/off event. */
int send_note(enum port p, int on, char channel, char note, char velocity);
