# Full flash backup of the SMK-37

Before the first install, read the whole 1 MiB flash while the stock app is still in place. This follows Method A
in [fm1-to-smk37](https://github.com/genkigenki/fm1-to-smk37)'s `docs/02-update-and-recovery.md`, written here for
this repo's tools and a Windows PC.

**Do the backup and the first install in one sitting.** The check step can leave the keyboard in Chordpad37's
update mode until an install finishes. From there, the way back to stock is to install Chordpad37 and then use
M-Vave's updater. That return is untested on this keyboard until M1 tries it. On the first try, an Elite on V016
came back from the UBOOT disk to stock rather than to the update mode, so this may not happen.

## What gets written

Nothing in the app area. The stock app accepts our package's check step, copies our update loader into a staging
area and writes an update record, then restarts into that loader. The dump therefore contains the loader and the
record. The calibration, Bluetooth pairing and key data are untouched.

## One-time setup

Python on Windows needs `mido` and `python-rtmidi` for the installer. They're already installed on this PC.

[jl-uboot-tool](https://github.com/kagaimiq/jl-uboot-tool) (MIT, by kagaimiq) reads the flash. Install it with its
own Python environment, at the commit this was written against:

```powershell
git clone https://github.com/kagaimiq/jl-uboot-tool $HOME\tools\jl-uboot-tool
cd $HOME\tools\jl-uboot-tool
git checkout adb3f18
python -m venv .venv
.venv\Scripts\python -m pip install -r requirements.txt
```

Its README lists the WL82 (AC791N) as "unknown", but the port kit read an SMK-37 Pro with it (`--chip wl82`).
Only its `read` command is used here. Never run `erase`, `write`, `erasechip` or `burnchipkey` against the
keyboard.

## Steps

1. **Close every program that uses the keyboard's MIDI ports:** DAWs, MIDI-OX, M-Vave's MIDI Suite.
2. **Check step and UBOOT,** in a normal PowerShell in this repo:

   ```powershell
   python base/tools/fm1_install.py app/build/smk37-elite/chordpad-smk37-elite.fwsc --check-only --uboot
   ```

   Answer `y`. Expect, in order:
   - `device: SMK-37 Elite_016`
   - the check step's reads
   - `in update mode: ota-SMK-37 Elite_900`
   - `left USB MIDI`

   The keyboard comes back as a USB disk named `WL82 UBOOT`.

   **If Windows offers to format the disk, cancel. Never format it.**
3. **Read the flash twice,** in a PowerShell run as administrator: Win+X, then Terminal (Admin), and accept the
   prompt. Its title bar says Administrator. In a normal shell the reads hang, so the script refuses to start there.
   Progress is shown as it reads:

   ```powershell
   & $HOME\tools\jl-uboot-tool\.venv\Scripts\python.exe C:\GitHub\chordpad37\tools\smk_backup.py
   ```

   It finds the disk, reads twice and checks:
   - both reads are 1,048,576 bytes;
   - they have the same SHA-256;
   - `trimdat` is at `0xC1224`.

   It also reports the flash id. The board accepts `0xEB6014` and the FM-1's Puya `0x856014`, which this Elite has. The dumps go to `~\smk37-backups\DATE-TIME\` with
   `SHA256SUMS` and a `README.txt`. If any check fails, run it again; don't trust a failed dump.
4. **Keep two copies** of that folder, one off this PC. Never put it in a repository: `0xC0000–0xC1FFF`
   (calibration, Bluetooth pairing) and `0xFF000–0xFFFFF` (key data) are unique to this keyboard.
5. **Power-cycle the keyboard.** It starts in Chordpad37's update loader (`ota-SMK-37 Elite_900`). Finish the
   install:

   ```powershell
   python base/tools/fm1_install.py app/build/smk37-elite/chordpad-smk37-elite.fwsc
   ```

## If something goes wrong

- **No `WL82 UBOOT` disk after step 2:** power-cycle. The keyboard comes back in the update loader, and
  `python base/tools/fm1_install.py --uboot` sends the key again.
- **A read stops with no progress:** after 5 minutes the script gives up. Power-cycle the keyboard, send the key
  again with `python base/tools/fm1_install.py --uboot` and run the script again.
- **The script can't open the disk:** the shell isn't elevated, or another program holds the disk. A fallback is
  Linux (root, `--device /dev/sgN`), for example WSL with the device passed through by usbipd-win.
- **Anything else:** stop and review. Don't write to the flash.
