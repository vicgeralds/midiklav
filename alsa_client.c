#include <alsa/asoundlib.h>
#include <stdio.h>
#include "seq.h"

static snd_seq_t *handle;
static int port_lower;
static int port_upper;

int open_seq()
{
	unsigned caps, type;
	if (snd_seq_open(&handle, "default", SND_SEQ_OPEN_OUTPUT, 0) < 0) {
		fprintf(stderr, "Error opening ALSA sequencer.\n");
		return 0;
	}
	snd_seq_set_client_name(handle, "midiklav");
	caps = SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
	type = SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION;
	port_lower = snd_seq_create_simple_port(handle, "lower keys",
						caps, type);
	if (port_lower < 0 || (port_upper = snd_seq_create_simple_port(
				handle, "upper keys", caps, type)) < 0) {
		fprintf(stderr, "Error creating sequencer port.\n");
		return 0;
	}
	return 1;
}

int send_note(enum port p, int on, char ch, char note, char vel)
{
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	ev.dest.client = SND_SEQ_ADDRESS_SUBSCRIBERS;
	ev.source.client = snd_seq_client_id(handle);
	ev.source.port = p==PORT_LOWER ? port_lower : port_upper;
	snd_seq_ev_set_direct(&ev);

	ev.type = on ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
	snd_seq_ev_set_fixed(&ev);
	ev.data.note.channel  = ch;
	ev.data.note.note     = note;
	ev.data.note.velocity = vel;

	snd_seq_event_output(handle, &ev);
	snd_seq_drain_output(handle);
	return 1;
}
