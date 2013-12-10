midiklav
========

midiklav is a simple program that let you play MIDI instruments with the computer keyboard, by generating MIDI notes.
It only uses ALSA and Xlib, and reportedly still "just works(TM)" [as of 2013-04-30].

You get a neat resizable GUI window and full control from the keyboard. The keys are split into two parts that can
play on different MIDI channels, which makes it possible to play two different instruments simultaneously.
You need to connect the output ports of midiklav somewhere, for example, using QjackCtl as shown below:

![midiklav with QjackCtl](midiklav_with_qjackctl.png?raw=true)
