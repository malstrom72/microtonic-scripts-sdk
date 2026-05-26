![Image](Microtonic%20User%20Guide_artifacts/image_000000_2e1f9abc4d4f8fbd1a929a952bf4645b95085164e3989d5966cfdf021fa71ac2.png)

![Image](Microtonic%20User%20Guide_artifacts/image_000001_c2c015aa3f4cacf2513d49f3e79956b7edaf7b29d8943839fc8da247d55735a1.png)

version 3.3.4

## Table of Contents

- Introduction, p. 3
- Architecture, p. 4
- User Interface, p. 6
- Toolbar, p. 7
- Preset Section, p. 10
- Drum Patch Section, p. 13
- Pattern Section, p. 21
- Global Section, p. 24
- Matrix Editor, p. 26
- Open Preset Dialog, p. 26
- Open Drum Patch Dialog, p. 27
- Preferences Dialog, p. 27
- MIDI Config Dialog, p. 29
- MIDI Controllers And Keys, p. 32
- MIDI Keyboard, p. 32
- Computer Keyboard, p. 33
- Requirements, p. 34
- Change History, p. 34
- Credits and Contacts, p. 43
- Copyrights And Trademarks, p. 44

## I N T R O D U C T I O N

Welcome to Sonic Charge Microtonic , the synthetic rhythm generator.

Microtonic is a VST® and AudioUnits compatible plug-in featuring a powerful drum and percussion synthesizer combined with a pattern-based drummachine engine. You can use it as a conventional sound module to play drum sounds from your MIDI keyboard or sequencer, or you can use the built-in pattern engine to play drum patterns in sync with your sequencer. 1

In Microtonic , a single universal sound architecture is used to simulate a wide variety of sounds. The parameters and the configuration of this architecture have been carefully chosen to be as versatile as possible while still keeping Microtonic simple and straightforward.

The sound of Microtonic is 100% synthetic and rendered in real-time. No samples or pre-rendered waveforms are used. This means that you can modify the sound in real-time with an instantaneous response by turning and dragging the knobs and faders of Microtonic . If your host supports parameter automation, you can record all parameter changes in your host sequencer for later playback with high accuracy.

Much work has been put into achieving optimal sound quality for Microtonic . There are virtually no compromises to the sound it produces. The oscillators are 'over-sampled' and produce a clean sound without distortion or 'aliasing' even at extreme frequency settings. The filters and equalizers have a full frequency response range, and parameter changes are smooth and responsive. The sound synthesis in Microtonic is exact without low-resolution approximations and quantization. All parameters have virtually unlimited resolution.

Furthermore, the triggering of drums is sample-accurate , and envelope 2 generators and modulators work at the highest possible rate to give the sound a distinct sharpness. Microtonic works in any sample rate of your choice. In short, Microtonic delivers a quality that sets the standard for what synthetic drums and percussions should sound like.

Thank you for choosing Microtonic , I trust that you will find it a valuable addition to your palette of sounds.

/ Magnus Lidström

1  Some hosts do not support synchronization of plug-ins, and in this case, the built-in pattern engine will be disabled.

2  Timing accuracy may be affected by the precision of the host.

## Sound Synthesizer

The sound engine of Microtonic offers eight channels of stereophonic sound (called 'drum channels' or simply 'channels') . Each channel has its own set of parameters that define its sound. Collectively these parameter settings constitute a drum patch . You work with one channel at a time, and you can load and save drum patches individually. (The file extension for a drum patch file is ' .mtdrum ')

The parameters of a drum patch are organized into four main sections: the Mixing Section ,  the Oscillator Section ,  the Noise Section ,  and  the Velocity Sensitivity section (from left to right on the screen) .

The Mixing Section is the final stage in the audio processing chain. It mixes the oscillator  and the noise generator and optionally distorts and equalizes the channel. After this stage, the audio from the various channels is sent to the host. Microtonic plug-in comes installed in two different versions, the 'standard version' ('Microtonic') , which offers two assignable stereo outputs and the 'multiple output version' ('MicrotonicMulti') which  offers  eight  individual  stereo  outputs,  one  for each drum channel.

The Oscillator produces a steady or pitch-modulated tone. The oscillator features three different waveforms and typically provides the tonal quality and the pitch of a drum sound. For example, a low-frequency tone with a quick drop in pitch is often used to synthesize bass drums. A higher frequency setting and a slower drop in frequency can simulate the sound of a tuned drum like a tom. Other types of modulations can be used to create a wide variety of sounds.

The Noise Generator is used to add noisy elements to the sound. For example, it can be a noisy punch sound, such as a hand or a drumstick hitting a drum, or the sound of the vibrating snares on a snare drum. The noise generator also features a stereo mode where two uncorrelated noise sources are used for the left and right output channels . This mode creates a dispersed stereo effect similar to the sound of  a  reverb.  Furthermore,  the  noise  section  features  a  multi-mode  filter  with  lowpass, band-pass, and high-pass modes and an amplitude envelope with different shapes allowing you to sculpt the noise just the way that fits your sound.

Lastly, we have a Velocity Sensitivity Section that controls how MIDI velocity and pattern accents affect the sound (see below for more info on pattern accents) . You have three different settings, two of which control the oscillator and noise volumes and one that controls the amount of pitch modulation on the oscillator.

## Pattern Engine

The Pattern Engine plays rhythmic patterns in sync with your host preset (that is, if your host supports tempo and song position synchronization of plug-ins) . You have 12 different patterns to choose from, labeled A to L . Each pattern consists of up to 16 steps , and you may chain them in series so that when one pattern ends, the next automatically follows. Each step, in turn, has switches for triggering drum channels with either accented or normal velocity. (Accented velocity corresponds to a MIDI velocity of 127, the hardest you can hit a MIDI key, and normal velocity is 64.)

The Rate at which the patterns run is set globally for all patterns in a preset and is relative to the tempo of your host sequencer. The setting goes from 1/8, meaning that each step is a one-eighth note in length, up to 1/32.

Each step of a pattern also has a Fill option used to create rapid drum rolls at a rate of your choice. Finally, a Swing parameter gives your rhythm a looser, more human feel  by  delaying  the  sixteenth  notes  that  fall  between  the  eights (this  feature  also goes by the name of 'shuffle' in some products) . The fill rate and the swing parameter are also set globally for the entire preset.

Pattern Changes usually do not occur in the middle of a pattern . The patterns play 1 to their full length before switching. You can automate and arrange pattern changes in  several  different  ways.  First,  you  have  the  chaining  option  where  you  create Chains of patterns that play after each other in series. As you only have 12 patterns to chain, this technique is somewhat inadequate for longer arrangements. One way to solve this is to automate pattern changes as parameter changes, provided your host supports this. (The current pattern selection is a parameter just as any other.) Another technique is to use MIDI notes to trigger patterns and record these notes in a MIDI sequencer track (see MIDI Keyboard for details) .

Notice that if you have odd pattern lengths , such as 7 or 13 steps, the pattern engine will synchronize its play position to the song position of your host sequencer in a manner that gives consistent and predictable results regardless of where you start the sequencer. If you switch between patterns with different lengths, this can result in the current step indicator jumping in peculiar ways.

Similar to the synchronization of pattern changes, starting and stopping patterns is synchronized to the length of the currently selected pattern. However, unlike pattern changes, you can force the pattern engine to start and stop immediately by clicking twice on the Play and Stop Buttons . (Starting and stopping patterns may be automated as parameter changes, just like pattern changes.)

The pattern engine of Microtonic can also transmit MIDI notes. Thus, to the extent that your host preset supports it, you can use Microtonic to trigger other MIDI devices. The MIDI keys / note-numbers are the same as for reception (normally C1 to G1) . The MIDI velocities depend on the accent settings (64 or 128) .

## Presets

The drum patch settings for all eight channels, all the 12 patterns, and the global parameters together constitute a Preset .  In Microtonic ,  you will find several functions that operate on the entire preset, including functions to load and save presets to ' .Mtpreset ' files.

## Morph

Morph lets  you  interpolate  all  drum  patch  parameters (for  all  eight  channels) between  two  end-points  using  a  single  slider.  Patterns  and  global  parameters (like Swing ) are  not  affected.  While  you  morph,  the  drum  patch  knobs  and  faders  will turn and move along. You can leave the morph slider at any position and edit the preset there (including copying, pasting, loading, etc.) , and it will affect the two endpoints according to where the morph slider is positioned. If the morph slider is to the far left or right, only the left or right end-points are affected. If the morph is cen- tered,  both  end-points will  change by an equal amount. Morph can be controlled with MIDI and automated, allowing you to use it both for performance (e.g., for dramatic build-ups) and for editing (e.g., exploring semi-random parameter settings) .

MIDI notes can be used to switch patterns immediately depending on the chosen Pattern Launch Mode in 1 the MIDI Config Dialog.

## Programs

The morph data, all patterns, and global parameters together constitute a Program . There are 16 programs in memory that you can instantly activate from MIDI or the user-interface at any given time without interrupting audio. These 16 programs are collectively known as the Program Bank , and it corresponds to the program bank accessible in VST hosts. By default, when you open a new instance of Microtonic , the  last  used  16  programs  will  be  reloaded  automatically. (This  behavior  can  be changed with the Preferences Dialog.)

## User Interface

The main window of Microtonic is  divided into four sections with a Toolbar at the top containing a Program Selector , Sound Morph , and more. The top area is the Preset  Section with  the Preset  Browser and  channel  buttons.  Below  this,  you have the Drum Patch Section where the currently selected channel's drum patch parameters are visible. At the bottom, you have the Pattern Section and the Global Section (including the master volume control of Microtonic ) .

![Image](Microtonic%20User%20Guide_artifacts/image_000002_5dd06e3567f35dd4eb366ba871c7de3084184400dda7b753ca037eb7ae638f99.png)

## Menus

The buttons marked with down-pointing arrows are menu buttons; menus concerning the relevant sections will pull down when you click on these. If you right-click anywhere  in  the  main  window (or  control-click  on  Mac), there  are  also  context menus available, with items relevant to the section or controller that you clicked on.

## Repeat Last Menu

You can Shift-Click any menu button to repeat the last executed menu. This feature is  especially useful for repeating functions such as altering and randomizing drum patches and patterns, shifting patterns left and right, etc.

## Knobs and Faders

Parameters are edited with the numerous buttons, knobs, and faders found on the screen. Whenever you drag a fader or a knob, a hint window displays the name and setting of the parameter in question. The setting is shown in the natural unit for the parameter, e.g., frequency settings are expressed in hertz (Hz) and note name and volume settings in decibels (dB) .

Some keys on your computer keyboard modify the behavior of knobs and faders. If the Shift Key is held down, the rate of change is adjusted so that you can make finer adjustments. If the Control Key is held down (command key on Mac) , the fader or  knob  will  reset  to  its  default  value (i.e.,  the  value  of  a  newly  initialized  preset  / drum patch) . Finally, you can press the Alt Key when clicking a knob to temporarily change its mode from circular to linear or vice versa.

You can right-click any knob, fader, or button in Microtonic to  open a small menu with a few different choices that work directly on the controller. For example, you can use this menu to enter an exact textual value or assign a MIDI controller to the parameter.

## Toolbar

The Toolbar contains the Program Selector , Sound Morph ,  various buttons, and the Midi Drag icon.

![Image](Microtonic%20User%20Guide_artifacts/image_000003_354ddfa4a77a964e14fb192ab00062df232becc1bc19763d8f1f1794f20f00f5.png)

## MIDI Drag

This icon offers an alternative to exporting the current pattern to a MIDI file from the pattern menu. By dragging this icon to your desktop or a folder, it automatically creates a MIDI file (file extension '.mid') named after the current preset name. You can also drag and drop this icon directly onto a track in a host that supports 'drag and drop' of MIDI files.

## Program Selector

There are 16 program slots available in Microtonic . Clicking the program display will open up a popup menu to select which of the 16 slots you want to activate. There are  also  options  for  enabling Midi  Program  Change messages  as  well  as Write Protecting all programs. By write protecting the programs, any edits you make will be lost as soon as you switch to another program. (This can be handy in live performance situations.)

## Sound Morph

This  slider  allows  you  to  morph  all  drum  patches  of  all  channels  simultaneously. Read the section above called Morph for information on how this slider works.

## Undo / Redo

These  buttons  undo  or  redo  recent  changes  you  have  made.  The  'undo  history' remembers up to 40 changes, but the history will be cleared every time you close the editor window.

## Select Channel with MIDI

When  the Select  Channel  With  Midi option  is  turned  on, Microtonic will  select drum channels as you trigger them from your MIDI keyboard. This feature is especially useful if you plan to use the MIDI controller mapping feature to edit your drum patches (see the section on MIDI Controllers And Keys for more info) .

Notice that there is no way to distinguish MIDI notes from your keyboard from those that  come  from  MIDI  tracks  in  your  host  sequencer.  Therefore,  it  may  be  a  good idea to turn this option off before starting sequencer playback.

(This option is also available from the MIDI Config Dialog.)

## Pitched MIDI Mode

Microtonic features a Pitched Midi Mode , which allows you to play melodies with the drum patches on your keyboard. Clicking this button will switch this mode on and off. In pitched mode, the eight drum channels are addressed with MIDI channels 1 to 8, and you have the entire keyboard for each channel. C3 (note number 60) will play the 'original pitch' of the drum patch.

MIDI channel 10 still responds as it does in standard mode, which allows you to control Microtonic fully from your MIDI keyboard (including changing programs, patterns, and muting channels) .

(This option is also available from the MIDI Config Dialog.)

## Stop Voices with MIDI Note Off

Turning  this  option  on  will  make Microtonic mute  voices  on Midi  Note  Off messages, allowing you to change the drum sounds' length by changing the note length in your sequencer. It is also useful when using Pitched MIDI Mode to sustain held midi keys.

(This option is also available from the MIDI Config Dialog.)

## PO-32 Transfer

Click the button with the Teenage Engineering logotype to open the PO-32 Tonic transfer  window.  Here  you  can  choose  to  transfer  a  single  drum  patch (including morph settings) or the full pattern (including all unmuted drum sounds) to the PO-32 Tonic hardware.

Transfers are done via an audible modem signal either over the speaker (to the microphone on PO-32 ) or through an audio cable (to the line input on PO-32 ) . The audio from Microtonic must be played loud and clear without any effects for the transfer to succeed. If you transfer over the speaker, hold the microphone on the PO-32 very close to the speaker.

If the transfer fails, the PO-32 will beep and flash 'Err'. If it succeeds, you will hear a cheerful little melody, and the PO-32 will flash 'End'.

On PO-32 , knob A is always the pitch (by changing all three frequency sliders simultaneously), and Knob B is the morph slider. Notice that unlike on Microtonic , morph can be set individually for any of the 16 sounds in PO-32 .

Some morph settings are 'out of bounds' for the PO-32 .  When  transferring,  you may see a warning that the morph settings had to be adjusted for the PO-32 . When this happens, the drum will still sound identical to Microtonic at the current morph position, but at the left and right 'morph edges', the sound will be different (both in the PO-32 and in Microtonic ) .

Bear in  mind  that  the PO-32 has  four  monophonic  channels,  whereas Microtonic has eight stereophonic channels. When transferring a full pattern, channel 1 to 4 will interrupt any sounds playing on channel 5 to 8. While the sounds+pattern tab  is open, you can hear a preview of how this sounds. (If  the transfer window is in the way, you can 'minimize' it and remain in preview mode.)

The Optimize  For  PO-32 button attempts  to  minimize  'voice  stealing'  by rearranging  the  drum  channels.  The  algorithm estimates the importance of each drum played and tries to avoid cutting  off  important  sounds.  This  problem is  challenging  for  a  machine  to  solve, and sometimes it fails. One way to help the  algorithm  is  to  manually  raise  the volume  of important sounds  before clicking  the  button  and  lowering  them afterward.  Another  way  is  to  mute  the sounds  that  are  unimportant  to  you.  A final method is to rearrange the channels manually, e.g., by dragging the channels in the Matrix Editor.

Finally,  some  features  are  not  available or work differently in PO-32 compared to Microtonic ,  e.g.,  the  choke  button,  patterns  shorter  than  16  steps,  the  exact sound of fills under high swing settings, etc. While individual  drum  sounds should  always  sound  identical,  full  patterns  can  sometimes  sound  very  different.

![Image](Microtonic%20User%20Guide_artifacts/image_000004_e27dbeb9076ffbb70df84fa73619618111f68e6db1c79968ed9332160bb04a38.png)

## Preset Section

The Preset Section of Microtonic contains the preset selector and channel buttons.

![Image](Microtonic%20User%20Guide_artifacts/image_000005_9d731084dc78b3c55996ea19180c4375ef89d75fec7c9d177375f34a1f414459.png)

## Main Menu Button

Click the main menu button to display the main menu. Here you will find functions that operate on the entire preset and some general utility and installation functions.

- Undo / Redo

These choices undo or redo recent changes you have made. The 'undo history' remembers up to 40 changes, but the history will be cleared every time you close the editor window. You will also find buttons for Undo and Redo in the Toolbar .

- Open Preset

Choose Open Preset to bring up an 'open dialog' that lets you load a Microtonic preset file (file extension '.mtpreset' or '.mtpg') . The open dialog features direct previewing of presets when you browse them and options to load only parts of preset (see Open Preset Dialog).

- Save Preset As

Save Preset brings up a 'save dialog' that lets you save the current preset settings into a Microtonic preset file (file extension '.mtpreset') .

- Revert to Saved

Reloads the last saved version of the preset from disk. (Only available if the current preset has been loaded from or saved to disk.)

- Cut Preset

This menu copies the current preset settings onto the clipboard and resets the settings in Microtonic .

- Copy Preset

This menu copies the current preset settings onto the clipboard.

- Paste Preset

This menu pastes preset settings from the clipboard, replacing the current preset. (Only available if a Microtonic preset is available on the clipboard.)

- Initialize Preset

This  menu choice will reset all  preset  settings,  including  all  current  drum  patch parameters and all patterns.

- Exchange Preset / Clipboard

This menu item exchanges the preset settings on the clipboard with the settings in Microtonic . This feature is useful if you would like to toggle between two different versions of a preset. Copy the first version, change the parameters (or load a new  preset) and  use  this  function  to  swap  between  the  two  different  presets. (Only available if a Microtonic preset is available on the clipboard.)

## ‣ Transpose Preset

This function will let you transpose the entire preset (all drum patches) by an arbitrary number of semitones up (positive) or  down (negative) .  Y ou  can  enter  decimals for a finer precision than whole semitones. To repeat the transposition, use the Repeat Last Menu feature.

## ‣ Alter Drum Patches

Use this function to make minor random adjustments to all drum patches. To test various alterations, use the Repeat Last Menu feature.

## ‣ Randomize All

Use this function to randomize all preset settings, including all current drum patch parameters and all patterns. A lot of effort has been put into making the random function create musically interesting results (with an emphasis on the word 'interesting',  as  in  experimental,  not  necessarily  musically  pleasing) .  T o  test  various random results, use the Repeat Last Menu feature.

## ‣ Edit MIDI Controllers / Keys

This menu toggles the on-screen editing of MIDI keys and controllers mapping. See MIDI Controllers And Keys for a description of how this works.

## ‣ MIDI Config

This menu item opens the MIDI config dialog described below in MIDI Config Dialog.

## ‣ Preferences

Choose this item to open the preferences dialog described below in Preferences Dialog.

## ‣ Register

Brings up a registration dialog where you register your copy of Microtonic . More information  on  how  to  purchase  a  registration  key  can  be  found  on  the Sonic Charge web site.

## ‣ Read User Guide

Opens the Microtonic User Guide in your PDF reader.

## ‣ Zoom

Opens the zoom overlay, which allows you to scale the Microtonic user interface.

## ‣ About

Choosing this item will display a window with information on the version of Microtonic you are running.

## Channel Buttons

There are eight channel buttons, one for each drum channel. Press a channel button to display and edit the drum patch parameters for that channel. The pattern editor will also display the triggers, accents, and fills of the channel you select. A blue light indicates the selected channel.

You may trigger a channel to preview its drum patch by clicking a selected channel button again. If the Control Key is held down (Alt Key on Mac) , the channel will be triggered  accentuated (MIDI  velocity  127) ;  otherwise,  it  will  be  triggered  normally (MIDI velocity 64) .

The channel buttons also  indicate  when  channels  are  being  triggered  by  flashing quickly with a green light.

If you right-click a channel button, a menu will pop up, offering a few different drum channel choices. For example, from this menu, you can cut, copy, and paste entire channels,  including  the  channel's  pattern  data  for  all  patterns.  These  functions make it easy to swap the places of two channels. Just copy the first channel to the clipboard, choose Exchange Drum Channel / Clipboard on the second channel, and paste back on the first channel.

## Mute Buttons

Pressing a mute button will toggle the muting of a channel. A muted channel will not respond to triggers from the pattern engine or MIDI notes. If a sound has already been triggered, it will not be shut off by muting the channel. Thus, it differs from the mute on a mixer-console where the sound turns off and on instantaneously. If you are using Microtonic to control other MIDI devices, the pattern engine will not generate MIDI notes for muted channels. Muted channels are indicated by a red light.

Holding down the Control Key (Alt or Command key on Mac) when clicking a mute button will solo that channel by muting all other channels, unless it is already soloed in which case it will be 'un-soloed' by 'un-muting' all channels.

The mutes can also be controlled from the MIDI keyboard, normally with the keys C2 to G2 (MIDI note-number 48 to 55) , but this may be customized to your preference (see MIDI Controllers And Keys) .  When using keys to mute, the channel will stay muted until you release the key. (Muting from the MIDI keyboard cannot be automated by the parameter automation features in your host as opposed to muting with mouse-clicks or from the computer keyboard. Instead, you can record the MIDI mutes in a MIDI track.)

## Drum Patch Section

In the middle of the main window, you have the drum patch section. This section contains all the controls for altering the drum patch of the currently selected channel.

![Image](Microtonic%20User%20Guide_artifacts/image_000006_eaebe4237417ba8fff1c0dc4bedb838b86c5d0d739159b8eb9f9b2ba5035de29.png)

## Drum Patch Selector

If the current drum patch has been loaded from or saved to disk, the Drum Patch Selector will display its name. Use the buttons on the left and right-hand side of the name display to flip through patches on disk and click inside the name display to bring up a list with all the patches in the current directory. If the drum patch is modified, a star (*) is  appended to the name. Note that you cannot explicitly rename a patch without saving it with a new name.

## Drum Patch Menu Button

Click  the  drum  patch  menu  button  to  open  up  the  drum  patch  menu.  The  drum patch menu contains functions that operate on the drum patch of the currently selected channel.

## ‣ Open Drum Patch

Brings up an 'open dialog' that lets you load a Microtonic drum patch file (file extension  '.mtdrum'  or  '.mtdp') .  The  open  dialog  features  direct  previewing  of drum patches when you browse them (see Open Drum Patch Dialog).

## ‣ Save Drum Patch

Brings up a 'save dialog' that lets you save the current drum patch parameters (of  the  selected  channel) into  a Microtonic drum  patch  file (file  extension  '.mtdrum') .

## ‣ Cut Drum Patch

This  menu  copies  the  current  drum  patch  parameters (of  the  selected  channel) onto the clipboard and resets the parameters in Microtonic .

## ‣ Copy Drum Patch

This  menu  copies  the  current  drum  patch  parameters (of  the  selected  channel) onto the clipboard.

## ‣ Paste Drum Patch

This menu pastes drum patch parameters from the clipboard. The patch on the clipboard will replace the parameters of the selected channel. (Only available if a Microtonic drum patch is available on the clipboard.)

## ‣ Initialize Drum Patch

This menu choice will reset all drum patch parameters of the selected channel to their default settings.

## ‣ Exchange Drum Patch / Clipboard

This menu choice exchanges the drum patch parameters on the clipboard with the  selected  channel's  parameters  in Microtonic .  This  feature  is  useful  if  you would like to toggle between two different versions of a patch. Copy the first version, change the parameters (or load a new patch) and use this function to swap between the two different patches. It is also useful if you want to swap the drum patches of two different channels. (Only  available  if  a Microtonic drum  patch  is available on the clipboard.)

## ‣ Transpose Drum Patch

This will let you transpose the drum patch by an arbitrary number of semitones up (positive) or  down (negative) .  You  can  enter  decimals  for  a  finer  precision  than whole semitones.

## ‣ Alter Drum Patches

Use this function to make minor random adjustments to the drum patch.

## ‣ Randomize Drum Patch

Use this function to randomize the drum patch entirely.

## Open Drum Patch Button

Press this button to bring up an 'open dialog' that lets you load a Microtonic drum patch file (file extension '.mtdrum or '.mtdp'). The loaded patch will replace the current patch parameters of the selected channel.

(This is a shortcut for 'Open Drum Patch' in the Drum Patch Menu.)

## Save Drum Patch Button

Press this button to up a 'save dialog' that lets you save the current drum patch parameters (of the selected channel) into a Microtonic drum patch file (file extension '.mtdrum') .

(This is a shortcut for 'Save Drum Patch' in the Drum Patch Menu.)

## Edit All

When Edit All is enabled, turning any knob and dragging any fader will adjust all unmuted  drum  channels  simultaneously.  For  example,  if  you  turn Distort up  10%, then the Distort amount of all (un-muted) channels will be increased by 10%. If you are recording automation in your host, Microtonic will  write  parameter automation for all affected channels.

## Mixing Section

The mixing section is the leftmost group and the final stage in the audio processing chain.  It  mixes  the  oscillator  and  the  noise  generator  and  optionally  distorts  and equalizes the channel. After this stage, the audio from the various channels is mixed and sent to the host.

## ‣ Oscillator/Noise Mix

Range: 0/100 to 100/0

Default: 50/50

Oscillator/Noise Mix controls the mix of oscillator versus noise. Only the oscillator  is  heard  at  the  far  left  setting (100/0), and  at  the  far  right (0/100), only  the noise. In the middle (50/50) , the volumes are balanced so that the oscillator and noise sources are mixed with equal power.

## ‣ Equalizer Frequency

Range: 20Hz to 20 000Hz (20kHz)

Default: 632.46Hz

Equalizing is applied after the distortion. Proper use of the equalizer plays an integral part in achieving a professional sound. You can use the equalizer to boost the bass frequencies of bass drum sounds or make the high-end frequencies of hi-hats sound crispy and sharp. Negative equalizer gain can create notch effects such  as  the  typical  loudness  curve (where  the  middle  range  is  attenuated  and bass  and  treble  are  boosted) .  Sweeping  the  frequency  using  parameter  automation usually creates a cool effect.

## ‣ Distortion Amount

Range: 0 to 100

Default: 0

This parameter controls the amount of distortion. Zero = no distortion (100% linear) ,  100 = complete and utter destruction. The distortion unit is specifically designed to be useful for creating powerful drum sounds. It softly shapes the signal at very low settings, adding both odd and even harmonics, which creates a warm and thick sound. At higher settings, the distortion turns more into an overdrive effect, creating a harder sound. The distortion unit is the only part of Microtonic that has not been specifically designed to prevent 'aliasing' artifacts on high-frequency material. There are two reasons for this: first, to be perfectly 'alias-free', the distortion would require a lot of CPU-power. Secondly, 'aliasing' can be cool, and since everything else in Microtonic is virtually 'alias-free', this is a good place to give the user a choice. (A word of caution here: the amount and the character of 'aliasing' will be different when running in different sample rates.)

## ‣ Equalizer Gain

Range: -40db to +40db

Default: 0db

The  equalizer  gain  setting.  A  negative  gain  will  cut  frequencies  from  the  signal spectrum,  whereas  a  positive  gain  will  boost  them.  When  the  gain  is  high,  the channel's  output  level  is  automatically  attenuated  to  achieve  a  more  consistent volume. The peak or dip width (i.e., the q value) is fixed and cannot be changed.

## ‣ Level

Range: -infinite dB to +10dB

Default: 0dB

This is the output level of the drum patch. When turned down to its minimum, the channel  is  shut  off  completely.  Zero  decibels (the  default  setting) is  considered normal volume, and the maximum of 10 decibels is around three times as loud.

- Pan

Range: -100 to +100 Default: 0

The Pan parameter controls the drum patch stereo position within its output pair. At the minimum and maximum settings, the patch is panned fully left and right, respectively. Microtonic uses a 'sine/cosine pan law', which means that the experienced volume remains more or less the same when panning.

- Choke

Choices: Off, On Default: Off

Enable Choke on  a  drum channel to have it 'cut off' other drum channels that also  have  choke  enabled.  Useful  for  making  the  closed  hi-hat  sound  mute  the open hi-hat sound, for example. Lesser channel numbers will have priority over higher channel numbers, meaning that if they both try to play at the same time, only the lesser channel number will be heard. If you need to change the priority order, it is easy to rearrange channels in the Matrix Editor.

- Output

Choices: A, B Default: A

This is where you select the output pair for the channel. Microtonic has two individual stereo outputs, A and B, and each drum patch can be routed to either one. This  feature  is  useful  if  you  want  to  add  external  effects  to  some  of  the  drum channels,  but  not  all  of  them.  If  you  feel  that  two  separate  outputs  are  not enough, there is an alternative version of Microtonic , which offers eight individual outputs, one for each drum channel ('MicrotonicVSTMulti'/'MicrotonicAUMulti') . And remember, you always have the choice of creating more instances of Microtonic to attain additional outputs.

## Oscillator Section

The oscillator  produces  a  steady  or  pitch-modulated  tone.  The  oscillator  features three different waveforms and typically provides the tonal quality and the pitch of a drum sound. For example, a low-frequency tone with a quick drop in pitch is often used to synthesize bass drums. A higher frequency setting and a slower drop in frequency can simulate the sound of a tuned drum like a tom. Other types of modulations can be used to create a wide variety of sounds.

## ‣ Oscillator Waveform

Choices: Sine, Triangle, Sawtooth Default: Sine

The Oscillator Waveform setting  defines  the  basic  shape  and  character  of  the tone produced by the oscillator. The sine waveform produces an ideal sine tone with no overtones. It is useful for all types of drums, percussions, and effects. The triangle  waveform  produces  a  soft  tone  with  all  odd  harmonics (like  a  'square wave'  but  with  less  high-frequency  content) .  The  triangular  waveform's  hollow character gives the sound a bit more 'body' than the sine waveform and is thus very  useful  for  toms,  snares,  and  bass  drums.  Finally,  the  sawtooth  waveform produces a distinct tone containing all harmonics. This sound is not very common in  natural percussion sounds. It is most useful for special effects and electronic sounds,  but  also  for  sounds  that  need  a  lot  of  high-frequency  energy  such  as cymbals.

## ‣ Oscillator Frequency

Range: 20Hz to 20 000Hz (20kHz)

Default: 632.46Hz

Oscillator frequency setting (or pitch) , ranging from subsonic (20Hz) to supersonic (20kHz) .  This  is  the  steady  frequency  of  the  oscillator  or  the  base  frequency around which the pitch is modulated if pitch modulation is used.

## ‣ Pitch Modulation Mode

Choices: Decaying, Sine, Random

Default: Decaying

The pitch modulator is one of the most powerful features of Microtonic . The combination  of  mode,  amount,  and  rate  lets  you  modify  the  sound  of  the  oscillator dramatically.  The Decaying  Pitch  Modulation mode  is  perhaps  the  most straightforward.  It  'bends'  the  pitch  towards  the  oscillator  base  frequency  and creates the typical drops in frequency you hear in most drums. With a negative modulation amount, the pitch will go upwards instead of downwards, useful for sound effects and percussions like tablas and clay drums. The Sine Pitch Modulation mode is quite versatile. At low modulation rates, it acts as an 'LFO effect' that oscillates the pitch around its base frequency. At fast modulation rates, the sine modulator turns into an FM-like effect. FM generates inharmonic overtones giving the sound a metallic character that is useful for hi-hats and cowbell patches. Finally, the Random Pitch Modulation mode applies a random modulation of the pitch. At zero rate, it simply randomizes the pitch by the chosen amount every time the drum patch is triggered. At slightly higher rates, it gives the sound a sort of bubbly character. At the highest rates, it adds a band of filtered noise to the oscillator,  where  the  modulation  amount  controls  the  band's  width.  With  proper settings, the random modulator can simulate the sound of rattles, shakers, and tambourines.

## ‣ Pitch Modulation Amount

Range:  depends  on  modulation  mode,  decaying:  -96  semi-tones  to  +96  semitones (-8 octaves to +8 octaves) , sine and random: -48 semi-tones to +48 semitones (-4 octaves to +4 octaves)

Default: 0 semi-tones.

Pitch Modulation Amount controls the amount of pitch modulation. You can use a negative amount to invert the modulator effect so that it starts from a low pitch and goes upwards instead of going downwards from a high pitch. (Naturally,  a negative amount with the random modulation mode is no different from a positive amount since the random modulator randomizes the pitch totally both upwards and downwards.)

## ‣ Pitch Modulation Rate

Range: depends on modulation mode, decaying: infinite ms down to 10ms, sine: 0Hz to 2 000Hz (2kHz) , random: 0Hz to 20 000Hz (20kHz) Default: 353.33ms / 17.78Hz / 100.0Hz

The Rate parameter  depends  on  the  modulation  mode.  For Decaying  Pitch Modulation , the rate setting controls the time it takes the oscillator to go from the highest frequency down to the base frequency (or from the lowest frequency and up if the modulation amount is negative) . At minimum rate setting, the oscillator is stuck at the highest / lowest frequency and never changes. Faster decay rates usually make the sound sharp and 'snappy'. For Sine Pitch Modulation , the rate setting controls the frequency with which the sine modulator oscillates. Just as with the decaying mode, the minimum rate setting makes the oscillator stick to its initial  frequency  and  never  changes.  The  highest  modulation  rates  generate  an FM-like  effect  that  can  be  used  to  create  inharmonic  sounds. Random  Pitch Modulation uses a low-pass filtered noise as the modulation source, and for this mode, the rate controls the cut-off frequency of the low-pass filter. A low rate setting  modulates the pitch slowly, and a high rate modulates the pitch rapidly. At zero  rate,  it  simply  randomizes  the  pitch  by  the  chosen  amount  every  time  the drum patch is triggered. At the maximum rate, the random modulator will cause a controllable band of noise to be added to the oscillator.

- Oscillator Attack

Range: 0ms to 10 000ms (10s) Default: 0ms

The Oscillator Attack parameter controls the total time for the attack phase of the oscillator envelope. The oscillator and noise sections have separate amplitude envelope generators (noise envelopes are more complex and offer three different shapes) .  With  the  default  attack  setting  of  zero,  the  envelope's  attack  phase  is eliminated,  and  the  oscillator  triggers  at  full  force  directly.  This  causes  a  sharp transient with a distinct clicking sound. If you desire a softer sound, you can reduce this click by increasing the attack time just a little. Longer attack times suggests a sound that has been reversed in time because the oscillator attack phase is always exponential (most synthesizers have a linear attack and an exponential decay) .

- Oscillator Decay

Range: 10ms to 10 000ms Default: 316.23ms

(10s)

The Oscillator Decay setting controls the time it takes for the oscillator to fade from max to zero volume. The oscillator envelope always runs from maximum to zero amplitude with an exponential decay curve (the most natural-sounding type of  decay) . (The  observant  reader  may  object  that  an  exponential  decay  never reaches zero, which is true in theory; thus, the time setting is only an approximate.)

## Noise Section

The Noise Generator is used to add noisy elements to the drum patch. For example, it can be a noisy punch sound, such as the sound of a hand or a drumstick hitting a drum, or the sound of the vibrating snares on a snare drum. The noise generator also features a stereo mode where two uncorrelated noise sources are used for the left and right output channels. This mode creates a dispersed stereo effect similar to the sound of a reverb. Furthermore, the noise section features a multi-mode filter with low-pass, band-pass, and high-pass modes as well as an amplitude envelope with different shapes allowing you to sculpt the noise just the way that fits your sound.

## ‣ Noise Filter Mode

Choices: Low-pass, Band-pass, High-pass

Default: Low-pass

The noise generator signal passes through a filter featuring three different modes: Low-Pass , Band-Pass , and High-Pass . The Low-Pass mode cuts high frequencies from the signal spectrum but leaves the lower frequencies intact, making the noise softer and duller. This mode is great for adding space and reverb to bass drums and such. The Band-Pass mode cuts both low and high frequencies but lets  the  middle  range  through,  narrowing  the  sound  like  playing  something through a small tube or poor speaker. This mode is a good starting point for many percussion instruments like snare drums, handclaps, etc. The High-Pass mode removes the lower frequencies and makes the noise bright and clear, which you generally want for hi-hat and cymbal sounds.

## ‣ Noise Filter Frequency

Range: 20Hz to 20 000Hz (20kHz)

Default: 20kHz

This is the cut-off or center frequency of the filter, depending on the filter mode. The cut-off frequency for a Low and High-Pass filter  is  scientifically  defined  as the point on the filter curve where it drops below -3dB in gain. (Put simply, this is the point where you start cutting in the signal spectrum.) For the Band-Pass filter, the noise filter frequency is the center of the peak in the filter curve, i.e., the point where most of the signal's spectral power is present.

## ‣ Noise Filter Q

Range: 0.1 to 10 000.0

Default: 0.70710683

The Q Value of a filter affects the shape of the filter curve just around the cut-off frequency. A high q value creates a distinct peak in the filter curve while a low q value changes the curve's slope so that it becomes softer and lets more of the signal through. The q value can also be said to define the band-pass filter peak; lower q values make it broader, and higher make it narrower. When the noise filter q is turned up to its maximum, the noise signal becomes so narrow that it sounds more like an irregular sine tone than noise.

## ‣ Noise Stereo Mode

Choices: Off, On

Default: Off

The  noise  generator  features  a  stereo  mode  where  two  uncorrelated  noise sources are used for the left and right output channels. This mode creates a dispersed stereo effect on the noise, which can be used to simulate reverb effects of various kinds, or simply to broaden the stereo image of a sound.

## ‣ Noise Envelope Mode

Choices: Exponential, Linear, Modulated (clap)

Default: Exponential

The Oscillator and Noise Sections have  separate  amplitude  envelope  generators. The noise section's envelope generator is more advanced and features three different modes (or shapes) . The Exponential Mode is the default choice and the most common mode. It creates a natural sounding fade in its decay phase. Its attack phase is also exponential, which is not that common (most synthesizers have a linear attack and an exponential decay) . A long exponential attack will suggest a sound that has been reversed in time or the sound of a fast engine 'swooshing by' at great speed. The Linear Mode creates a more conventional attack, but its decay acquires a 'gated' effect, which is great for simulating gated snare drums. Finally, the Modulated Mode is tailor-made for handclaps. It works by retriggering the envelope in rapid successions before going into the decay phase. In this mode, the attack setting controls the time until the decay phase by changing the number of triggers and the time between them.

- Noise Attack

Range:  depends  on  noise  envelope  mode,  exponential  and  modulated:  0ms  to 10 000ms (10s) , linear: 0ms to 6 666.67ms Default: 0ms

The Noise Attack parameter controls the total time for the attack phase of the noise envelope. For the Exponential and Linear envelope modes, this is the time it  takes  the  noise  envelope  to  go  from  silence  to  maximum  amplitude.  For  the Modulated Mode ,  this is the total time spent triggering short 'clap bursts' until the decay phase kicks in. (See above for more information.) An attack setting of zero eliminates the envelope's attack phase and makes the drum patch trigger at full amplitude directly.

- Noise Decay

Range: depends on noise envelope mode: exponential and modulated: 10ms to 10 000ms (10s) , linear: 6.66667ms to 6 666.67ms

Default: 316.23ms / 210.82ms

The noise decay parameter controls the decay time or the time it takes the envelope generator to go from maximum amplitude back to silence. (The time for exponential  decay  is  approximate  since,  in  theory,  an  exponential  decay  never reaches zero.)

## Velocity Sensitivity Section

The Velocity Sensitivity Section controls how MIDI velocities and pattern accents affect the sound. You have three different settings, two of which control the oscillator and noise volumes, and one that controls the amount of pitch modulation on the oscillator. If all settings are zero, MIDI velocities and accents do not affect the sound at all.

## ‣ Oscillator Velocity Sensitivity

Range: 0% to 200%

Default: 0%

This parameter decides how MIDI velocities and accents affect the volume of the oscillator. A setting of zero percent means that velocities (and accents) do not affect the volume. Higher percentages make velocities (and accents) affect the volume more. A setting of 100% means that the oscillator is silent if a note has zero velocity. (A setting of 200% means that MIDI velocities of 64 or more are required for the oscillator to be heard at all.)

- Noise Velocity Sensitivity

Range: 0% to 200%

Default: 0%

This parameter decides how MIDI velocities and accents affect the volume of the noise generator. A setting of zero percent means that velocities (and accents) do not affect the volume. Higher percentages make velocities (and accents) affect the volume more. A setting of 100% means that the noise section is totally silent if a note  has  zero  velocity. (A  setting  of  200%  means  that  MIDI  velocities  of  64  or more are required for the noise to be heard.)

## ‣ Modulation Velocity Sensitivity

Range: 0% to 200%

Default: 0%

The final velocity parameter controls how MIDI velocities and accents affect the amount of pitch modulation on the oscillator. Set to zero percent, velocities (and accents) do not affect the pitch modulation at all; the pitch modulation amount is decided by the modulation amount parameter alone. Higher settings make velocities (and accents) change the modulation so that unaccented / softer notes have less modulation than accented / louder notes. Like the oscillator and noise velocity sensitivity settings, a setting of 100% or more means that the modulation can be turned off completely by low velocities.

## Pattern Section

At the bottom of the window, you have the pattern editor. To the left of the pattern steps, there is a small button to show and hide the Matrix Editor.

![Image](Microtonic%20User%20Guide_artifacts/image_000007_d389ea58672f256294da00b70b60fbe302672f2d62cac90875d51f1b43ad52c6.png)

## Pattern Menu Button

Click the Pattern Edit Menu button to show the pattern edit menu. The pattern edit menu contains functions that operate on the currently selected pattern. If you hold down the Control Key (Alt Key on Mac) when clicking the menu, the functions will operate on the currently selected channel of the pattern only; otherwise, all channels are affected.

## ‣ Cut Pattern / Pattern Channel

This function copies the currently selected pattern/pattern channel onto the clipboard and clears the pattern/pattern channel in Microtonic .

## ‣ Copy Pattern / Pattern Channel

This menu choice copies the current pattern/pattern channel onto the clipboard.

- ‣

## Paste Pattern / Pattern Channel

Use this menu item to paste patterns or pattern channels from the clipboard. The contents of the clipboard will replace the settings in Microtonic . (This menu item is only available if a Microtonic pattern or pattern channel is available on the clipboard.)

‣ Exchange Pattern / Pattern Channel Exchange pattern/pattern channel exchanges the pattern/pattern channel on the clipboard with the settings in Microtonic . This feature is useful if you would like to toggle between two different versions of a pattern. Copy the first version, change the pattern and use this menu to swap between the two different versions. It is also useful if you want to swap two channels in a pattern. (This menu item is only available if a Microtonic pattern or pattern channel is available on the clipboard.)

## ‣ Export Pattern To MIDI File

This menu item lets you export the currently selected pattern to a standard MIDI file.  If  the  currently selected pattern is part of a 'pattern chain', the entire chain will be exported. (See MIDI Drag below for a more convenient method if you wish to export patterns directly to your host sequencer for tweaking and arranging.)

## ‣ Export Pattern To Audio File

Use this menu item to export the currently selected pattern (or pattern chain) to a 'WAV file'. When you select this item, you will be presented with a 'save dialog' to choose further options for the export. The format choices are 16, 24, or 32-bit resolution and a selection of sample rates between 32 and 192 kHz. The 32-bit option will generate a so-called floating-point format with virtually infinite resolution and no clipping. You are also given a choice of how to treat the audio tail (i.e., what to do with the remaining audio after the pattern/pattern chain has played its length) .  Select None to  simply drop any remaining audio, select Append to  append the audio (and cut the file after the sound has decayed to silence) , or select Loop to merge the tail into the beginning of the file so that it works well for looping.

Shifts  the  entire  pattern  or  currently  selected  channel  one  step  to  the  left.  The pattern/pattern channel rotates so that the leftmost step is shifted out and then in (This function works on the designated length of the pattern.)

## ‣ Shift Pattern / Pattern Channel Left again from the right. To shift more than one step, use the Repeat Last Menu feature.

## Shift Pattern / Pattern Channel Right

Shifts the entire pattern or currently selected channel one step to the right. The pattern/pattern channel rotates so that the rightmost step is shifted out and then (This  function  works  on  the  designated  length  of  the To shift more than one step, use the Repeat Last Menu feature.

- in  again  from  the  left. pattern.)
- ‣

## Reverse Pattern / Pattern Channel

This menu choice reverses the pattern/pattern channel so that the first step becomes the last, and so on. (This function only reverses material within the designated length of the pattern.)

## ‣ Alter Pattern / Pattern Channel

The Alter Pattern / Pattern Channel function is a handy little feature that shuffles random parts of the pattern around. It may be used to create variations of a pattern. The algorithm works solely on existing material, so altering an empty pattern is meaningless. (Also, this function alters only material within the designated pattern length.) To test various alterations, use the Repeat Last Menu feature.

## ‣ Randomize Accents and Fills / Channel Accents and Fills

This menu item randomizes only the Accent and Fills of a pattern while not altering any triggers. It may be used to make minor variations of a pattern. To test various random results, use the Repeat Last Menu feature.

## ‣ Randomize Pattern / Pattern Channel

This menu item randomizes the pattern/pattern channel totally, wiping out any already existing contents. A lot of effort has been put into making the random function create musically interesting results (with an emphasis on the word 'interesting', as in experimental, not necessarily musically pleasing) .  T o  test  various  random results, use the Repeat Last Menu feature.

## Copy Pattern / Pattern Channel Button

This button works as a shortcut to the Copy Pattern / Pattern Channel menu. It copies the current pattern onto the clipboard or the currently selected pattern channel if you hold down the control key when clicking (alt key on Mac) .

## Paste Pattern / Pattern Channel Button

This button is a shortcut to the Paste Pattern / Pattern Channel menu. If a pattern or  pattern  channel  is  available  on  the  clipboard,  it  will  be  pasted;  otherwise,  this button is disabled.

## Chain From Previous Pattern / To Next Pattern

These buttons are used to create chains of patterns. When a pattern in a pattern chain  has  finished  playing,  the  next  pattern  in  the  chain  will  automatically  follow. The patterns in a chain are always played in alphabetical order. (On the screen that would be from left to right, top to bottom.) When the last pattern in a chain has finished playing, the first one will start again. The leftmost button chains the previous pattern with the current and the rightmost chains the current with the next. (Naturally, the left button is disabled for pattern A and the right button is disabled for pattern L.)

If  you  switch  to  a  pattern  that  is  part  of  a  chain, Microtonic does not necessarily start playing the first pattern in the chain, depending on the song position in your host sequencer. This behavior is to ensure a consistent and predictable result regardless of where you start the sequencer.

## Pattern Selection Group

You use this group of 12 buttons to select pattern A to L for playing and editing. A solid blue light indicates the currently chosen pattern. The currently chosen pattern is the pattern that you edit with the Pattern Edit Lanes . If the pattern engine is not currently playing this pattern, it will do so after it has finished playing its current pattern. The pattern that is currently playing is shown by a flashing green light.

If a pattern is empty (i.e., no triggers) , its label will be a lighter gray, and if the currently selected pattern is part of a chain, the patterns of the chain will have blue labels.

Right-clicking  a  pattern  button  will  bring  up  the  pattern  menu  with  some  actions that you can perform on the pattern (as described above under Pattern Menu Button) .

## Pattern Edit Lanes

The Pattern Edit Lanes are used to edit the currently selected pattern. Alternatively, you may wish to use the Matrix Editor described below for editing all channels simultaneously. The length lane is common for all the channels of a pattern, while the other  three  lanes  only  edit  the  currently  selected  channel.  All  lanes  are  clickable, and you may click and drag to set or reset several switches in one sweep. Furthermore, holding down the shift key will make the click alter all drum channels that are not currently muted. For example, you can use this feature to create a short pause or to set accents on a step for all the channels.

- Channel Triggers

This is the most important lane in the pattern editor. Each button in this lane represents one step in the pattern and decides if a drum should be triggered at that step or not. The buttons display the steps for the currently selected drum channel. A blue button indicates a trigger, and a white indicates a pause. (Note that the accent and fill  buttons  are  ignored  for  steps  without  triggers.) Clicking  a  button with the Control Key held  down (Alt  Key  on  Mac) will  switch  the  accent  of  the step as well as the trigger.

- Channel Accents

The Accents affect the velocities of drum hits triggered by the pattern engine. A step without an accent will trigger a drum with MIDI velocity 64, and a step with accent will play with MIDI velocity 127.

- Channel Fills

Each step also has a Fill switch that is used to create rapid drum rolls. The Fill Rate is set globally for all patterns in Microtonic and may be set to two times per step  up  to  eight  times  per  step.  The  velocities  in  these  fills  decay  by  a  fixed amount to achieve a natural rolling effect.

- Pattern Length

The  length  of  the  current  pattern  is  shown  and  edited  here.  The  length  is  displayed in blue, and a green light indicates the current play position within the pattern. The length is the same for all channels of a pattern, so the currently selected drum channel does not affect this lane.

## Global Section

![Image](Microtonic%20User%20Guide_artifacts/image_000008_cebdac9504a79397355c808197550dcb252050b4903a8559344304b4fba1c3aa.png)

## Stop Button

The Stop Button stops and deactivates the internal pattern engine. Similar to the synchronization of pattern changes, the stop is synchronized to the length of patterns. If the Stop Button is blinking, the engine will stop after the currently selected pattern has finished playing. You can force the pattern engine to stop immediately by clicking twice on the stop button.

(Starting and stopping patterns may be automated just as pattern changes.)

## Play Button

The Play Button enables  and  activates  the  internal  pattern  engine.  Patterns  start synchronized  to  the  song  position  in  your  host  sequencer  as  determined  by  the length  of  the  currently  selected  pattern. (For  example,  a  pattern  with  a  length  of three sixteenth notes would start at the first, fourth, and seventh sixteenth note in the song, and so on.)

If the pattern engine is set to start but has not done so yet, the Play Button flashes. This may be because the host sequencer is stopped or because the engine is waiting for a synchronized position to start at. You may force the pattern engine to start immediately by clicking the Play Button twice.

(Starting and stopping patterns may be automated just as pattern changes.)

## Step Rate

Choices: 1/8, 1/8T, 1/16, 1/16T, 1/32

Default: 0%

This  is  where  you  set  the  rate  with  which  the  pattern  engine  runs  relative  to  the tempo in your host. The setting goes from 1/8, meaning that each step is a oneeighth note in length, up to 1/32. There are also triplet rates that play three or six steps per quarter. (The rate is set globally for all patterns in a preset.)

## Swing

Range: 0% to 100% Default: 0%

The Swing parameter gives your patterns a looser, more human feel by delaying the playback of sixteenth notes that fall between the eights. (This feature also goes by the name of 'shuffle' in some products) . The amount is set between 0 and 100 percent, where 0% is perfectly 'we are the robots' stiff. (The swing amount is set globally for all patterns and channels in a preset.)

## Fill Rate

Range: 2x to 8x (per step) Default: 2x

Each step in a pattern has a Fill switch that may be used to create rapid drum rolls. The Fill  Rate setting  determines  the  speed  of  these  drum  rolls.  The  rate  is  measured in the number of hits per step. The default setting of two creates rolls of 32nd notes, and the maximum of eight creates rolls of 128th notes. Notice that you can use fractional rates as well (such as 2.5) for unsynchronized rolls. The velocities of fills decay by a fixed amount to achieve a natural rolling effect. If the step that triggers the roll is accented, the MIDI velocity will run from 128 to 64 over the roll; if not, it will run from 64 to 0. (The fill rate is set globally for all patterns and channels in a preset.)

## Master Volume

Range: -infinite dB to +10dB Default: 0dB

This is the final output volume for the preset. When turned down to its minimum, the sound  of Microtonic is  shut  off  completely.  Zero  decibels (the  default  setting) is considered normal volume, and the maximum of 10 decibels is around three times as loud. The master volume affects both stereo output pairs of Microtonic .

## Matrix Editor

The matrix editor allows you to view and edit the pattern steps for all eight channels simultaneously.  Accented  and  filled  steps  are  illustrated  using  different  colors, shapes, and sizes. While clicking, you can hold down the Control Key (Alt Key on Mac) to make the steps accented or the Alt Key (Command key on Mac) to add a 'fill' to the step or a combination of both.

![Image](Microtonic%20User%20Guide_artifacts/image_000009_93d120ac135bd1185853b6c970126a8122620ed20ed1b6e4a27d6725b85a47fd.png)

The channel numbers on the left serve the same purpose as the Channel Buttons , allowing you to both select and trigger drum sounds by clicking them. You can also rearrange the channel order by clicking a number, and with the mouse button held down, drag it to a new location. When the mouse button is released, the channel will be inserted into the new location, shifting other channels up or down as necessary.

Shift Click channel numbers to mute them and Control Click (Command Click on Mac) to solo.

(Channels that use 'choke mode' are indicated by a small choke symbol.)

## Open Preset Dialog

The Open Preset  Dialog includes  a  couple  of  options  for  previewing  the  preset within the browser. To preview, either click the Play/Stop Button or  simply select Auto to start previewing as soon as you select a new preset in the list.

You  can  preview  patterns  in  their  original  tempo  or  synchronized  to  the  host  sequencer by selecting In Sync . Use the Preview Patches option to try out the drum patches in a preset from your MIDI keyboard without having to open it first.

There are also options to load All data from the preset or only loading the Patches or Presets .

![Image](Microtonic%20User%20Guide_artifacts/image_000010_bacb921d20f83383b117a78b11ceb3a7e1662f2bea6ca2ed13931b884aa15ddc.png)

Clicking Go To Factory Presets changes the current directory to the factory preset directory. There you will find presets sorted by packages or all presets in one big flat directory.  Clicking Go  To  User  Presets takes  you  to  the  last  place  where  you opened or saved a (non-factory) Microtonic preset.

## Open Drum Patch Dialog

The Open Drum Patch dialog is similar to the preset dialog, and it also allows you to preview drum patches before loading. To preview them, click the Play/Stop Button or simply select Auto to preview the patch automatically when selecting them in the list.

Turn on Play Accented to preview the drum patches with accented instead of normal velocity. Use the Preview in Preset if you wish to preview a drum patch in the context of the currently playing pattern or from your MIDI keyboard.

Clicking Go  To  Factory  Patches changes  the  current  directory  to  the  factory patches directory. There you will find presets sorted by categories, packages, or all drum patches in one big flat directory. Clicking Go To User Patches takes you to the last place where you opened or saved a (non-factory) Microtonic drum patch.

## Preferences Dialog

The Preference Dialog is used to choose configuration settings for Microtonic . You access this dialog from the main menu (see the section Main Menu Button above) . The settings are 'global' and are used by all instances of Microtonic .

![Image](Microtonic%20User%20Guide_artifacts/image_000011_b2f077bd8ce1a146de158892532f5d7888d0ebf9572be8980c0e139c5798926d.png)

## Knob Behavior Mode

Here you select the default behavior mode for all knobs of Microtonic . The default option,  called Host Decides ,  makes Microtonic adhere  to  the  preference  of  your host sequencer. Circular mode lets you click and drag knobs in a circular manner, just like a real hardware knob. Relative Circular mode works like the circular mode but does not immediately turn the knob to the mouse cursor when you click. The Linear mode makes the knob work like faders; you click and drag up and down (or left and right) to turn it.

## Startup Bank

This setting decides which programs should automatically load when you open new instances of Microtonic . The default setting is to load the Last Used bank; you can change this to either: Initialized , which does not load any programs at all but starts Microtonic in an initialized state. Factory Demos loads the original sixteen factory demonstration  presets.  The  last  option  is  to  load  a Custom set  of  programs.  By clicking Use Current, Microtonic will start with the presets that you have currently loaded in memory.

## Check the Sonic Charge site for updates on startup

Activate this setting to automatically connect to the Sonic Charge internet site and check for upgrades. The check is performed in the background once every third day that you use Microtonic .

## Swap functions of right mouse button and control/alt key

You can use this option to change the way the right mouse button is used. The right mouse button will take the role of the Control Key (or Alt Key on Mac) and vice versa. For example, with this option turned on, you can right-click channel buttons to trigger drums accented or to set steps in the pattern editor to trigger accented. The context menus that you normally reach by right-clicking are then obtained by holding down the control key instead.

## Enable keyboard shortcuts

Lets you turn off keyboard shortcuts, allowing you to play MIDI notes with the MIDI keyboard without first having to click outside the Microtonic window.

## MIDI Config Dialog

With  the Midi  Config  Dialog ,  you  configure  how Microtonic responds  to  and transmits MIDI data. Each instance of Microtonic has its own configuration, and the configuration is stored with your song data when you save your project.

![Image](Microtonic%20User%20Guide_artifacts/image_000012_9a9a88373bcc4bb178c5627f4863b4fda18a140f1ec45bc6bfda3c47fcaec3ff.png)

## Pattern MIDI Notes

The Pattern Midi Notes setting  determines how patterns are 'launched' by MIDI notes. In Switch Next mode (default) , the current pattern will be played fully before the next pattern is launched. Switch Directly will switch patterns directly when pattern notes are received. Retrigger also switches directly and restarts the patterns from step 1. Retrigger Gated works like Retrigger but  will  stop  playing  patterns when MIDI keys are released.

## Pitch Wheel Range

Microtonic listens to Midi Pitch Wheel messages for real-time pitch transposition. Here you can set the range between 0 (Disabled) and 2 octaves. In Standard Mode , the pitch wheel will affect all channels simultaneously, while in Pitched Mode , it will affect individual channels.

## Enable MIDI Program Change

Enable this option to make Microtonic switch between the 16 program slots when it receives Midi  Program  Change messages.  Messages  with  program  numbers above 16 will be ignored.

## Stop voices with MIDI Note Off

Turning  this  option  on  will  make Microtonic mute  voices  on Midi  Note  Off messages. This allows you to change the length of the drum sounds by changing the length  of  the  notes  in  your  sequencer.  It  is  also  useful  when  using Pitched  Midi Mode to sustain held midi keys.

## Select drum channel with MIDI notes

When this option is turned on, Microtonic will select drum channels as you trigger them from your MIDI keyboard. This feature is especially useful in combination with the Midi Cc Operate On Selected Channel option if you plan to use the MIDI controller mapping features to edit your drum patches.

## Pattern Engine Sends MIDI Notes

Turn this option on to have Microtonic transmit MIDI notes for the patterns it plays. To the extent that your host preset supports it, you can use this option to trigger other MIDI devices and plug-ins from Microtonic . The MIDI keys transmitted are the same as for reception (normally C1 to G1) and can be edited as described in MIDI Controllers And Keys.

## MIDI CC operate on selected channel

Normally, MIDI controllers that you map are mapped to parameters of specific drum channels in Microtonic . If you instead want to map MIDI controllers to drum patch parameters regardless of channel, you should use this option. When turned on, any MIDI controller action will only affect the currently selected drum channel. This feature is especially useful if you plan to use your hardware MIDI controller for editing drum patches. Notice that if you wish to automate parameters with this feature, you must also turn on the ' Midi Controllers Generate Parameter Automation ' option.

(see the section on MIDI Controllers And Keys for more info) .

Notice that there is no way to distinguish MIDI notes that come from your keyboard from notes that come from MIDI tracks in your host sequencer. Therefore, it may be a good idea to turn this option off before starting sequencer playback.

## MIDI CC generate parameter automation

There are two different techniques for recording parameter automation with plugins. Either you use the parameter automation features in your host (usually referred to as reading and writing automation data) , or you record MIDI controller data from a hardware MIDI controller into your MIDI tracks. Both techniques have their pros and cons. With some sequencers, it is easier to keep MIDI notes and controller data together by using the latter technique. However, the first technique is generally more reliable since you can change the MIDI controller mapping without damaging your recorded  automation.  If  you  wish  to  use  the  first  technique  with  'automation writing', and you are using a hardware MIDI controller, you should turn on this option.

## Improve scaling of MIDI CC values

This option will remap MIDI Controller Change values so that zero is reachable on bipolar parameters. With this option turned on, MIDI CC on the three frequency parameters will be quantized to whole semitones.

## Knobs and buttons send MIDI CC

Enable this option to send MIDI controller data for mapped knobs and buttons back to  your  MIDI  controller  hardware.  This  option  is  useful  if  your  MIDI  controller  displays the current knob settings or if it has motorized faders. Enabling this feature allows you to keep your controller in sync when you change presets or channel selection in Microtonic . Notice that this feature only works with the VST version of Microtonic in  hosts  that  supports  routing  MIDI  from  plug-ins  to  your  MIDI  interface. Also, some MIDI controllers do not respond to MIDI CC data for updating displays. In Pitched Midi Mode (see below) , controller data will be sent on MIDI channel 1 to 8 (and 10 for global parameters like 'Master Volume') if Midi CC Operate On Selected Channel is off, or only 10 if it is on. Otherwise, in Standard Midi Mode all controller data is sent on the selected Transmit Channel .

## Standard MIDI Mode

In  standard  MIDI  mode, Microtonic will  respond  to  one  specific  MIDI  channel  as designated by the Receive Channel choice (or to all MIDI channels if the Receive Channel is set to 'any') .

If the Pattern Engine Sends Midi Notes option is turned on, Microtonic will transmit MIDI notes for the patterns it plays to one specific MIDI channel as designated by the Transmit Channel choice.

The keys used for triggering drum channels and selecting patterns can be edited as described in MIDI Controllers And Keys. Default assignments can be found below in MIDI Keyboard.

## Pitched MIDI Mode

Microtonic features  a  mode  called Pitched Midi Mode ,  which  allows  you  to  play melodies with  the  drum  patches  on  your  keyboard.  In  this  mode,  the  eight  drum channels are addressed with MIDI channels 1 to 8, and you have the entire keyboard  for  each  channel.  C3 (note  number  60) will  play  the  'original  pitch'  of  the drum patch. If MIDI transmission is enabled, the same channels and notes will be used for transmission.

MIDI channel 10 still responds as it does in standard mode, which allows you to control Microtonic fully from your MIDI keyboard (including changing programs, patterns, and muting channels) .

## Config

Use the Reset Button to reset the MIDI configuration to factory defaults. This button will also reset any MIDI controller mapping and customized MIDI keys. You can press Make  Default to  make  the  current  settings  the  default  configuration  used every time you create a new instance of Microtonic .

The  MIDI  configuration  is  not  stored  in Microtonic preset  files;  instead,  you  can Load and Save MIDI configurations to separate files with the file extension '.scmc'. These  files  also  contain  MIDI  controller  mappings  and  customized  MIDI  key  assignments.

## MIDI Controllers And Keys

Some hosts let you use the computer keyboard to perform common functions in Microtonic . A list of the relevant keys and functions can be found in Computer Keyboard.

Microtonic offers  features  for  controlling  the  software  from  an  external  hardware MIDI controller. With the on-screen editing of MIDI controllers and key mappings, it could not be easier to set up your favorite controller to work with Microtonic .

To enter the mentioned editing mode, simply choose Edit Midi Controllers / Keys from the Main Menu , and you will see small gray markers above all editable knobs, faders, and buttons. Rectangular markers (like those on top of the drum channel selection buttons) indicate assignable MIDI keys, while oval markers indicate that you can assign MIDI controllers. Click once in a marker to quickly enter MIDI learn for a button or controller (you will see a flashing MIDI connector symbol) . Now, press the desired key or turn the desired knob on your hardware controller, and you should see a note name or a controller number in the little marker.

To edit the assigned key or controller, click the marker and drag the mouse up and down while holding the button down. If you wish to remove a mapped controller or reset a key to its default assignment, simply click once inside the marker and then click anywhere else.

When you are ready with your setup, simply choose Edit Midi Controllers / Keys again to quit editing. If you like to save your configuration, you can do so with the Midi Config Dialog , which also contains several other useful options for controlling Microtonic with your hardware controller.

Finally, if you wish to quickly map a single knob on your hardware controller to a parameter in Microtonic , you do not have to go through all the fuss with entering and leaving  the  editing  mode.  Instead,  simply  right-click  the  knob  or  fader,  choose Learn Midi Controller ,  and turn the knob on your MIDI controller. It could not be easier!

Notice that the same MIDI key or controller number can be assigned to several different functions. Useful for layering several channels on a single MIDI key or controlling many parameters from the same knob.

## MIDI Keyboard

The MIDI keyboard (or MIDI sequencer tracks) can be used to trigger drum sounds, starting,  stopping,  changing  patterns  and  programs,  muting,  and  'un-muting' channels.  The  actions  you  perform  with  your  MIDI  keyboard  can  be  recorded  in MIDI sequencer tracks, but they cannot be automated as parameter changes. The MIDI keys you use for these actions can be customized with the on-screen MIDI controller and key editing, described in MIDI Controllers And Keys. Following is a list of the default settings with MIDI keys, note-numbers, and their functions:

| Key (s)   | Note #   | Function                                                                                                                                                    |
|-----------|----------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| C1 to G1  | 36 to 43 | Trigger drum channel one to eight. MIDI velocity may affect the sound as determined by drum patch set- tings.                                               |
| C2 to G2  | 48 to 55 | Mute channel one to eight. The channel will stay muted for as long as the key is held down. Foot- switch (MIDI controller 64) can be used to sustain mutes. |
| C3 to B3  | 60 to 71 | Select pattern a to l. (Start pattern engine if it is stopped. Press twice to start immediately.)                                                           |
| C4        | 72       | Stop and deactivate the pattern engine. (Press twice to stop and deactivate immediately.)                                                                   |

In the table above, C3 is 'middle-C'. If you use the pitched midi mode, these keys only respond on MIDI channel 10.

## Computer Keyboard

If your host supports the routing of keys to plug-ins, you may use the keyboard to control Microtonic and perform some of the most common functions. Following is a list of the relevant keys and their functions.

| Windows Key (s)   | Mac Key (s)   | Function (s)                                                                                                                                                                                                                                                       |
|-------------------|---------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 to 8            | 1 to 8        | Press once to select channel one to eight and preview its drum patch. With Control Key : preview accented. With Shift Key : mute (or 'un-mute') channel (may be auto- mated) . With Control Key and Shift Key : solo (or 'un-solo') channel (may be auto- mated) . |
| A to L            | A to L        | Select pattern A to L (may be automated) .                                                                                                                                                                                                                         |
| S                 | S             | Press once: stop and deactivate the pattern engine synchronized. Press twice: stop and deactivate immediately. (May be automated.)                                                                                                                                 |

| Windows Key (s)   | Mac Key (s)   | Function (s)                                                                                                                                       |
|-------------------|---------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| P                 | P             | Press once: activate the pattern engine and start playing synchronized. Press twice: ac- tivate and start playing immediately. (May be automated.) |
| Control-X         | Command-X     | Cut pattern.                                                                                                                                       |
| Control-C         | Command-C     | Copy pattern.                                                                                                                                      |
| Control-V         | Command-V     | Paste pattern / drum patch / preset.                                                                                                               |
| Control-J         | Command-J     | Shift pattern left (one step) .                                                                                                                    |
| Control-K         | Command-K     | Shift pattern right (one step) .                                                                                                                   |
| Control-L         | Command-L     | Alter pattern.                                                                                                                                     |
| Control-Z         | Command-Z     | Undo.                                                                                                                                              |
| Control-Y         | Command-Y     | Redo.                                                                                                                                              |

## Requirements

The minimum requirements for installing and running Microtonic are:

- Microsoft Windows 7 A host that supports 64-bit VST 2.4, or VST3 plug-ins
- macOS High Sierra (10.13) A host that supports 64-bit VST 2.4, VST3, or AudioUnit 2 plug-ins

## Change History

## Version 3.3.4 (2022-11-07)

- VST3 support.
- (Windows) Deprecated 32-bit support.
- Bug and compatibility fixes.

## Version 3.3.3 (2022-02-16)

- (Mac) Native support for Apple Silicon.
- Bug fixes, 'under the hood' maintenance, and improvements.
- Support for alternative 'skins'.

## Version 3.3.1 (2020-08-24)

- Added support for time-limited licensing.
- Made a workaround to handle a rare Windows problem with generating a unique machine-id.
- Fixed an issue that could cause the host user-interface to become sluggish when running many instances of Microtonic (even with closed plug-in windows).

## Version 3.3 (2020-03-04)

- Scalable GUI and retina support.
- Scripts can now be written in Javascript.
- Scripts can display custom user interface panels.
- A new button in preferences lets you turn off keyboard shortcuts, allowing you to play MIDI notes with the MIDI keyboard without first having to click outside the Microtonic window.
- Fixed an issue where exporting audio and MIDI produced invalid files when running under trial.
- Fixed a problem with the 'Optimize for PO-32' button after user scripts had been run.
- Program names with international characters should now show correctly in most hosts.
- (Mac) Several fixes to the standalone version, including better support for various audio interfaces.
- New algorithm for the 'system unique identifier' used for authorization. Hopefully fixing the problem where the plug-in became unregistered spontaneously.
- Fixed a bug that could leave temporary files behind when saving and replacing files.
- (Mac) Solved a compatibility problem with DAWs that are built with recent Apple SDK's, e.g. Cubase 10.5.
- (Mac) Notarized installer for Catalina.
- (Mac) New 64-bit compatible uninstaller.
- (Mac) 'Go to folder' buttons in browser now work in Catalina.
- (Mac) 64-bit  Audio  Units  no  longer  depend  on  the  'Component  Manager'.  This means you should not need to restart after installation.
- (Mac) Preferences and registration  data  is  now  shared  with  'sandboxed'  DAWs like GarageBand (meaning Authenticator works with these DAWs too) .
- (Mac) Fixed a problem where under certain conditions the preferences data could stay locked if the DAW crashed, requiring a full system restart.
- Lots of other minor bug and compatibility fixes.

## Version 3.2 (2017-02-02)

- Functions for transferring patches and patterns to the Teenage Engineering / Sonic Charge PO-32 Tonic hardware.
- Redesigned GUI with toolbar at top plus lots of new shining pixels everywhere.
- First standalone version for Mac.
- Fixed a problem with morph (the GUI sometimes lit up the incorrect waveform, envelope modes etc) .
- Fixed a problem with 64-bit Mac that caused Cubase to crash when unloading the plug-in.
- Fixed a division by zero in the audio engine.
- Reassigned controller count in midi config dialog was wrong. Fixed.
- Swap control and right-click (as set in preferences) didn't work properly with pattern editing. Now it does.
- 64-bit Mac version now receives keyboard input like the other versions (1-8 for selecting and playing drums etc) .
- Search-box in file browsers now work on 64-bit Mac.
- All versions except 64-bit Mac: fixed a bug that caused Microtonic to stop listening to keyboard input after closing about box.

## Version 3.1.1 (2014-12-16)

- Open browsers now feature two buttons to quickly take you to factory and user preset directories.
- Many minor bug-fixes.

## Version 3.1 (2013-05-17)

- First fully 64-bit compatible version.
- Supports MIDI CC out for updating MIDI controllers with current parameter values.
- Supports MIDI CC for mutes as well as MIDI keys.
- Supports using MIDI CC to perform pattern edits.
- Drum patches that are loaded from factory preset folders are now referenced to with relative file paths meaning that it will be easier to use the drum patch browser to change patches in factory presets.
- Signed packages on Mac OS X.
- Name of DLL's changed from MicroTonic(Multi)VST.dll to Microtonic(Multi).dll only. Name of plug-in is Microtonic from now on, not µTonic or MicroTonic. Logotype is still µTONIC.

- Fixed a bug where MIDI events occurring simultaneously would be processed in incorrect order, e.g. a note with 100% legato could stop the next note with its note off event.
- Max length of program name reported to host is 64 characters.
- De-initializes  and  releases  all  resources  when  last  instance  is  removed  from project.
- Updated to latest  support  libraries.  Lots  of  minor  stability  and  compatibility  improvements.
- In the file browser, drum patches are previewed on the individual outputs that they will load into.
- Supports automatic online registration through Sonic Charge Authenticator.
- New skin. Bigger. Beautifuller. Spacier. Space is the future.

## Version 3.0.1 (2011-05-10)

- Fixed  a  problem  in  the  AU  version  when  loading  version  2  projects  that  could make Microtonic use the wrong program number.
- Fixed a problem with clicks in exported audio when using the 'Append' tail mode.
- Fixed a rare timing drift problem between GUI and audio when using certain audio cards.
- Solved a compatibility problem with the Windows version of Nuendo that froze the application when using copy functions in Microtonic .
- Eliminated some almost inaudible but unwanted aliasing in the sine oscillator.
- Microtonic now  displays  a  special  message  when  attempting  to  register  with  a version 1 or 2 key.
- Code is PPC G4 compatible again (just as version 2 was) .
- Updated scripting engine.

## Version 3.0 (2010-12-31)

## Major New Features

- A morph slider allows you to interpolate all eight drum patches simultaneously between two end-points. You can edit the drum patches with the morph slider at any position and it will adjust the interpolation end-points accordingly. Morph is an automatable and MIDI controllable parameter, so it can be used both as a performance parameter as well as an editing feature.
- Matrix editor window for viewing and editing the pattern steps of all eight drum channels simultaneously. (Bonus feature: you can drag channel labels to reorder the drum channels of the preset.)
- MIDI Program Change message support for instantaneously switching between up to 16 different presets (loaded into memory) . (It is also possible to switch programs with MIDI notes.)

- Drag and drop patterns as MIDI files directly into your sequencer (supported by all major hosts) .
- 'Edit All Channels' button enables adjusting parameters for all drum channels simultaneously. E.g. turn up the distortion on all channels. Microtonic will write parameter automation for all affected channels, so the effect can be automated.
- A new 'choke group' feature lets you assign two or more channels that should play mutually exclusively and cut each other off. Useful for closed and open hihats for example.
- A preset name display sits in the top of the window, with previous and next preset buttons and a popup list when you click the display.
- Support for MIDI Pitch Wheel for real-time pitch transposition with a range of up to 2 octaves.
- A new 'Stop Voices with MIDI Note Off' option makes it possible to sustain the sound while you hold down MIDI keys (if you use Microtonic as a synth). It is also useful  for  changing  the  length  of  drum  sounds  with  note  lengths  in  your  sequencer.
- Alternative 'pattern launch modes': 'Switch Next' (the behavior in earlier versions of Microtonic ) , 'Switch Directly', 'Retrigger' and 'Retrigger Gated'.
- Undo / redo.
- Shift-click menu buttons to quickly repeat last selected menu item.
- New 3D-rendered skin. Sharper, brighter, sexier with 32-bit graphics. (Yes, Microtonic version 2.0 was using 20th century 16-bit RGB graphics still) .

## Other Improvements

- New  menu  functions:  'Transpose' (all  channels  or  a  single  drum  patch). 'Alter Drum Patches' (all  channels  or  a  single  drum  patch) .  'Randomize  Accents  and Fills' (on entire pattern or single channel) .  'Revert to Saved' (revert to last saved version of preset) .
- New trial system: up to three weeks of trial with full functionality. After that Microtonic will go silent until you register. Weeks will only be counted when you actually use Microtonic .
- Microtonic will now reload the most recently used program bank (of 16 programs) automatically  when  you  create  a  new  instance. (This  behavior  can  be  changed from the Preference Dialog.)
- From the file browser you can now select to load only the patches or the patterns from a preset.
- All  filter  algorithms  have  been  replaced  with  a  new  custom  design  based  on  a modified state-variable topology. It allows exact replication of the original filter response,  but  with  better  accuracy,  speed  and  stability  when  the  frequencies  are modulated.
- Parameter smoothing improved to allow quicker unfiltered parameter changes as well as slower filtered changes.

- You can optionally remap MIDI Controller Change values so that you can reach zero exactly on bipolar parameters. Also, with this option turned on, MIDI CC on the three frequency parameters will be quantized to whole semitones.
- Microtonic can  execute  simple  script  files (written  in PikaScript ) from  a  popup menu. A script button will show in the top-left corner if Microtonic finds script-files in a designated scripts folder.
- A write protect switch can protect the programs in memory by reverting any edits as soon as you switch to another program.
- You can assign the same MIDI key or controller number to several different functions. Useful for layering several channels on a single MIDI key or controlling many parameters from the same knob.
- All  existing  presets  (including  those  in  our  additional  downloadable  patch  packages)  have  been  modified  to  obtain  a  high  degree  of  consistency  concerning which drum channels plays what type of sound. E.g. channel 1 plays bass drum patches, channel 5 plays snare drum patches etc. Also, all presets have been volume normalized using an automatic RMS algorithm.

## Minor Changes

- Microtonic 3.0 uses a new registration key system. Registration keys for version 3.0 are not compatible with previous versions of Microtonic .
- All file formats have been upgraded (using a cleaner and more consistent text format) . Microtonic version 3.0 files are not downwards compatible with Microtonic version 2.0, so new file extensions have been chosen: '.mtpreset' and '.mtdrum'.
- Noise levels are much more consistent across different sample rates (the levels at 44.1khz were used as reference) .
- New WAV export code allows for 24-bit audio export and improved WAV-file compatibility.
- Improved timing accuracy of notes and parameter changes, especially when host is using odd (or just very large) buffer sizes. This is at the expense of a constant latency of 64 samples, but all major hosts should support latency compensation.
- The envelopes are slightly improved. E.g., they now go all the way down to level 0 exactly (avoiding clicks if Microtonic is run through extreme compression) .
- Changes to 'Mod Amount' will affect the sound immediately. (Previously, changes to this parameter didn't have effect until the next note on.)
- Step buttons light up when notes are played (Yeah, this is the minor changes section.)

## Bug / Compatibility Fixes

- Fixed bug with VST midi events outside the current processing block.
- Fixed minor bug in linear noise envelope decay.
- Automation now starts directly when you click a fader (instead of waiting until it is dragged) .
- Improved problem with Microtonic spontaneously losing registration.
- Fixed a problem where holes (!) would appear in the user-interface on Mac.

- Fixed a problem with the oscilloscope when using the multiple outputs version (in certain hosts) .
- Works (better) with host and bridges (e.g. VST Bridge) that runs different instances in different threads.
- Many, many other minor fixes.

## Version 2.0.3 (2007-11-29) (Mac OS X only)

- Fixed a compatibility problem in the Audio Units version that made the separate outputs of Microtonic unusable in Apple Logic version 8 and MOTU Digital Performer.
- Fixed a GUI compatibility issue under OS X 10.5 with certain hosts (e.g. Logic, GarageBand) that caused the user interface to come up white and empty when opening the plug-in.
- Exported MIDI files can now be imported into MOTU Digital Performer.
- The Mac version now features an uninstaller application. It can be found under the Sonic Charge Microtonic folder in your Applications folder.

## Version 2.0.2 (2006-05-18) (Mac OS X only)

- The Mac OS X version of Microtonic is now a so called 'universal binary', compatible and optimized for Intel Macs.
- Better compatibility with certain AU hosts (e.g. Plogue Bidule).

## Version 2.0.1 (2005-11-17)

- Introduced AudioUnit version for Mac OS X, ported using Symbiosis technology from NuEdge Development.
- Both AudioUnit and VST version for Mac OS X are compiled with XCode version 2.1 (GCC version 4.0) . This makes the code incompatible with any Mac OS X version prior to 10.3.9.
- Registration  dialog  includes  e-mail  address  field (optional  for  older  registrations, but will become mandatory in the future) .
- Prepared for physical distribution by detecting and loading registration information from removable media.
- New logotype and about box artworks by Bitplant.

## Version 2.0 (2005-06-20)

## Major New Features

- There is an alternative version of Microtonic with separate outputs for each drum channel ('MicrotonicVSTMulti') .
- Microtonic features  built-in  support  for  MIDI  controllers  with  easy-to-use  onscreen editing and 'MIDI learn'. A MIDI controller can be assigned to a parameter

in  a  specific  drum  channel,  or  if  you  prefer,  to  edit  the  currently  selected  drum channel. Once you have created your assignments you can save and load them.

- The  preset  and  drum  patch  file  browsers  feature  direct  previewing  within  the browsers.  You  can  preview  presets  and  patterns  in  their  original  tempo  or  synchronized to the music you are playing. Presets and individual drum patches can be previewed and compared directly without leaving the browser.
- There is a 'pitched MIDI mode' which allows you to actually play melodies with the drum patches on your keyboard. In 'pitched mode', the eight drum channels are  addressed  with  MIDI  channels  1  to  8  and  you  have  the  entire  keyboard  for each channel. C3 (note  number 60) will  play  the  'original  pitch'.  This  opens  up new possibilities when you will be able to use Microtonic not only for drums, but also for melodies and bass lines. (Disclaimer: Microtonic will  of  course  remain a drum synthesizer primarily.)
- The oscillator section has been blessed with an attack parameter so that you can achieve a softer sound and reduce the click of those 808-style bass drums. The attack envelope is exponential, just like the default envelope mode of the noise section.
- You can export individual patterns (or chains) to standard MIDI files and WAV files.

## Cool Improvements

- You can freely assign which MIDI keys the drum channels respond to and which keys to use for triggering patterns and muting channels.
- There is a preference dialog where you can choose how knobs should react (circular,  relative circular, linear or decided by host) .  Y ou can also set a default startup preset and switch the functions of the right mouse button and the Control / Alt Key . The latter is excellent for working quicker with editing accents etc.
- You can play Microtonic patterns even when the host sequencer is stopped. Just click the play button while it is flashing in 'waiting state' and it will start playing at once.
- Right-clicking drum channels give you some options on the entire channels (like cut / copy / paste etc) . Useful if you want to trade places of channels etc.
- Note names are displayed in the popup hints for the frequency sliders so that you can tune to an exact pitch. This is handy with the new 'pitched mode' (described earlier) .
- You can right-click a knob or slider to set an exact value with text (you may enter note names as well for frequency sliders) . You can also right-click to quickly assign MIDI controllers to knobs, sliders or buttons.
- Knobs and sliders have sub-pixel precision. This goes well in line with Microtonic sound engine, which features an infinitely fine resolution on all parameters. Now the visuals are virtually infinite in resolution as well. (Try dragging some knobs and faders with the shift key down, and boy is that smooth.)
- Patterns that are part of the current 'pattern chain' are lit in blue color. Empty patterns show up gray.
- You  can  drag  and  drop  presets  and  drum  patch  files  from  the  explorer  /  finder onto Microtonic .

- Clicking the drum patch name display will pop up a list with drum patches residing in the same directory for quick loading.
- There is a 'select channel with MIDI switch' that allows you to select drum channels for editing from your MIDI keyboard.
- Shift-clicking a step button (trigger, accent or repeat) sets or resets that step button for all (un-muted) channels at once. Try it out for making abrupt breaks or intense fills in patterns.

## Minor (but still cool) Improvements

- Windows version is even further optimized and generally uses around 15% less CPU.
- No longer does Microtonic pattern  engine  start  to  play  automatically  every  time you open up a preset. If it is stopped, it stays stopped.
- The randomization features  have  been  tuned  a  little,  they  should  now  hopefully give even more… erhm… interesting results.
- Soloing a channel will select that channel. This makes it easier to solo out and edit one channel at a time.
- A star (*) is appended to the preset name when it is modified (just like with drum patches) .
- Both the noise and oscillator envelope now have techniques to prevent clicking when retriggering with slow attack settings. This improves the sound of the noise envelope compared to version 1.0, especially with high q settings.
- If  a  host  reports  that  output  b  is  not  connected,  all  the  drum  patches  routed  to output b will go to a.
- Better compatibility with certain hosts (no names) by implementing yet some more 'safety measures'. Amongst other things we now prevent a problem were Microtonic would actually get unregistered spontaneously. (I know it sounds weird, but it is true.)
- File names and directories can contain unicode characters. They may not be displayed properly, but they should work. Previously you would get errors if you used unicode characters in names.
- Microtonic skin has undergone slight cosmetic surgery and now looks and feels clearer with sharper contrasts and a new drum patch display / selector amongst other improvements.

And lets not forget the numerous new presets and patches contributed by Elmodic, Rory Dows and others.

## Credits and Contacts

Sonic Charge Microtonic v1.0 - v3.3.4 (2003 - 2022)

Created by:

Magnus Lidström

Graphical design and additional development:

Fredrik Lidström

Additional artwork and design:

Bitplant

## Presets and patches:

AC

Anthony Chou

DS

Darryn Summers

EM

Ingo Nordloh / Elmodic

JN

Jaime C Newman

JvB

Jeffrey van Beek

KP

Kalle Paulsson

RD

Rory Dow.

SC Sonic Charge / Magnus Lidström

Used technologies:

(see Copyrights section below for more info)

NuXPixels &amp; AU/VST Symbiosis by NuEdge Development

libpng by G. Randers-Pehrson zlib by Jean-loup Gailly and Mark Adler stack\_trace by Edd Dawson VST PlugIn Technology by Steinberg Audio Units SDK by Apple

## Sonic Charge website:

[https://soniccharge.com](http://soniccharge.com/)

Special thanks go out to the following companies, people and animals for their concrete and  inspirational  contributions  to  the  shaping  and  the  making  of Microtonic :  Thomas  &amp; Wolfgang 'Bitplant' Merkle, Peter 'Fleecelabs' Lindberg, Andrew 'Cytomic' Simper, Ben 'Camel  Audio'  Gillett,  Marcus Zetterquist,  Kalle  Paulsson,  Lars  Erlandsson,  Tore  Jarlo, Markus 'Majken' Höglund, Martin Eklund, Helena Iggander, Kasper and Junia Lidström Iggander,  Gunnar Lidberg,  Kerstin Lidström,  David Kvarnberg,  Arne Scheffler,  Frank Hoffmann, Matt Black, Rob Swire, Jean-Michel Jarre, Tetris, Russin, Sunshee + all the brilliant folks at Propellerhead Software, XLN Audio and Teenage Engineering.

Also, endless thanks to our marvelous beta testers (you know who you are). You did a fabulous job!

## Copyrights And Trademarks

The Sonic  Charge Microtonic (a.k.a. µTONIC ) software  and  documentation  are owned and copyright by NuEdge Development 2003-2022, all rights reserved.

![Image](Microtonic%20User%20Guide_artifacts/image_000013_bd275720af5b39caaef04851c0e35ce3f0c9a57595b956e8bac7daccbb04eec2.png)

Symbiosis version 1.2 - 2.0, Copyright © 2010-2022, NuEdge Development / Magnus Lidström. All rights reserved.

![Image](Microtonic%20User%20Guide_artifacts/image_000014_20d729924dc15358750ca4715e77d28abe8d810917b0dbd8b14c6fae46a28453.png)

Steinberg  VST  PlugIn  SDK,  Copyright  Steinberg  Soft-  und  Hardware GmbH.

libpng versions 1.2.6 - 1.6.37, Copyright © 2004-2019 Glenn Randers-Pehrson.

zlib version 1.2.3 - 1.2.11, Copyright © 1995-2017 Jean-loup Gailly and Mark Adler.

VST® is a trademark of Steinberg Media Technologies GmbH, registered in Europe and other countries.

Windows is a registered  trademark  of  Microsoft  Corporation  in  the  United  States and/or other countries.

Apple, Mac, OS X, and macOS are trademarks of Apple Inc., registered in the U.S. and other countries and regions.

Microtonic software and documentation are protected by Swedish copyright laws and international treaty provisions. You may not remove the copyright notice from any copy of Microtonic .

Please, read the end-user license agreement enclosed in the package for a lot more legal mumbo-jumbo.

The contractor/manufacturer for Sonic Charge Microtonic is:

NuEdge Development / Magnus Lidström

Sågargatan 1b S-116 36 Stockholm Sweden