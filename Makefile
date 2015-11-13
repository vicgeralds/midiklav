midiklav: midiklav.c alsa_client.c seq.h notes.h
	$(CC) -O2 -o midiklav midiklav.c alsa_client.c -lX11 -lasound
