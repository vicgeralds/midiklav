enum note {
	G, G_SHARP,
	A, A_SHARP,
	B, B_SHARP,	/* extra black key */
	C, C_SHARP,
	D, D_SHARP,
	E, E_SHARP,	/* extra black key */
	F, F_SHARP
};

#define IS_WHITE_KEY(note) ((note) % 2 == 0)

/* using xkb keycode names to enumerate the keys by note */
enum {
	/* lower keys */
	NOTE_LSGT, NOTE_AC01,
	NOTE_AB01, NOTE_AC02,
	NOTE_AB02, NOTE_AC03,
	NOTE_AB03, NOTE_AC04,
	NOTE_AB04, NOTE_AC05,
	NOTE_AB05, NOTE_AC06,
	NOTE_AB06, NOTE_AC07,
	NOTE_AB07, NOTE_AC08,
	NOTE_AB08, NOTE_AC09,
	NOTE_AB09, NOTE_AC10,
	NOTE_AB10, NOTE_AC11,
	NOTE_RTSH, NOTE_BKSL,
	NOTE_RTRN,

	/* upper keys */
	NOTE_TAB = 26, NOTE_AE01,
	NOTE_AD01, NOTE_AE02,
	NOTE_AD02, NOTE_AE03,
	NOTE_AD03, NOTE_AE04,
	NOTE_AD04, NOTE_AE05,
	NOTE_AD05, NOTE_AE06,
	NOTE_AD06, NOTE_AE07,
	NOTE_AD07, NOTE_AE08,
	NOTE_AD08, NOTE_AE09,
	NOTE_AD09, NOTE_AE10,
	NOTE_AD10, NOTE_AE11,
	NOTE_AD11, NOTE_AE12,
	NOTE_AD12, NOTE_BKSP,

	NUM_KEYS
};
