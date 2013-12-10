midiklav: midiklav.c alsa_client.c seq.h keymap_se.h
	$(CC) -o midiklav *.c -lX11 -lasound
