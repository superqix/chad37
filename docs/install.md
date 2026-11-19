# Installing Chad37

Chad37 replaces the SMK-37's own firmware. You install it over USB from a computer, and you can go back to M-Vave's
firmware at any time.

| Keyboard | Status |
| :--- | :--- |
| SMK-37 Elite | In daily use |
| SMK-37 Pro | Tested by a user |
| MKE-P37 | Built from the same code but never installed. Try it only if you're comfortable going back to stock if something is wrong, and please open an issue either way. |

**The first install clears the stock firmware's settings**, such as your knob and pad MIDI assignments: write them
down or take photos of them first. M-Vave's own firmware rebuilds its settings if you go back. Your calibration (keys,
wheels, pads) is kept. Updating to a newer Chad37 keeps your presets, chord banks and settings.

## The easy way: from your browser

You need Chrome or Edge on Windows, macOS, Linux or Android (Safari, Firefox, iPhone and iPad can't do it), and a USB
cable that carries data (some charging cables don't).

1. Plug the keyboard in and switch it on. Quit everything that uses it: your DAW, synth apps, M-Vave's MIDI Suite,
   other browser tabs.
2. Open the **[Chad37 web installer](https://superqix.github.io/chad37/)** and press **Find my keyboard**. The
   browser asks to control MIDI devices: allow it.
3. The page shows your model and its firmware, and fetches the right package. Press **Install**, and don't unplug the
   keyboard while it writes (a minute or two).

When it's done, the keyboard shows the Chad37 face, then the main screen. If an install stops part way (cable out,
computer asleep), press **Find my keyboard** again: the keyboard waits in update mode and the page finishes it.

## The command-line way

The same install from a terminal, with Python. It needs:

- **Python 3**: Windows, macOS or Linux.
- Python's MIDI libraries, installed once:

  ```bash
  python3 -m pip install mido python-rtmidi
  ```

  On Windows, type `python` where this guide says `python3`.
- From the [latest release](https://github.com/superqix/chad37/releases/latest): the installer, `fm1_install.py`,
  and the package for your model (step 2). Put them in the same folder, and open a terminal there (not Python's own
  window: the commands below go in the system's terminal, such as PowerShell or Terminal, not after Python's `>>>`
  prompt).

### 1. Close anything using the keyboard

Quit every program that has the keyboard's MIDI ports open: your DAW, synth apps, M-Vave's MIDI Suite, and browser
tabs using Web MIDI. The installer can't reach the keyboard while another program holds it.

### 2. Find your model

Plug the keyboard in, switch it on, and run:

```bash
python3 fm1_install.py --info
```

It prints the keyboard's identity, such as `SMK-37 Elite_016  [running]` (an Elite on M-Vave's V016). Pick the
package that matches:

| Identity starts with | Package |
| :--- | :--- |
| `SMK-37 Elite` | `chad37-smk37-elite-VERSION.fwsc` |
| `SMK-37 Pro` | `chad37-smk37-VERSION.fwsc` |
| `MKE-P37` | `chad37-smk37-mke-VERSION.fwsc` |

If you pick the wrong one, the installer refuses it before anything is written.

### 3. Install

```bash
python3 fm1_install.py chad37-smk37-elite-VERSION.fwsc
```

Use your own package's file name. Answer `y`. The installer:

1. checks the package with the running firmware, which then restarts into its update mode;
2. writes Chad37;
3. waits for the keyboard to restart, and checks that it now reports Chad37's identity.

**Don't unplug the keyboard while it writes.** When it's done, the keyboard shows the Chad37 face, then the main
screen.

If the computer shows the keyboard under its old name afterwards, unplug it and plug it back in. On Windows the old
name can stay until you remove the device in Device Manager.

### Updating with the command line

The same command with the new package. Your presets, chord banks and settings are kept.

## Bluetooth

On an SMK-37 Elite, Chad37 advertises Bluetooth MIDI as **Chad37**:

- **macOS:** Audio MIDI Setup, Window, Show MIDI Studio, the Bluetooth button, then Connect next to Chad37.
- **iPhone and iPad:** connect from inside your music app's Bluetooth MIDI settings, not from the system Bluetooth
  menu.
- **Windows:** whether your music app sees Bluetooth MIDI depends on the app; the USB cable always works.

If you had paired the stock firmware or an older Chad37, forget that device first.

## Going back to stock

M-Vave's web workbench reinstalls their firmware from Chrome or Edge:

1. Open [open.m-vave.com](https://open.m-vave.com) and choose your keyboard's page.
2. Tick **Force flash**, then click **Restore factory V16**, and confirm the target (for an Elite,
   `SMK-37 Elite_016`).

This has been done on an Elite: it came back with its calibration, pitch-bend rest and pads as they were.

## If something goes wrong

| What you see | What to do |
| :--- | :--- |
| `not found` | Close every program using the keyboard's MIDI ports (step 1), check the cable, and try again. |
| `needs mido and python-rtmidi` | Install them (see The command-line way). |
| `the device is …, the package is for …` | That package is for another model: pick yours from step 2. |
| The install stopped part way (cable out, computer asleep) | Press **Find my keyboard** on the web installer again, or run the same install command again: the keyboard waits in update mode, also after being switched off, and the installer finishes the write. |

## A full backup (optional, for the cautious)

Going back to stock doesn't need a backup. If you want an exact copy of your keyboard's flash anyway, before the
first install, [flash-backup.md](flash-backup.md) shows how. It's Windows-only and needs an extra tool.
