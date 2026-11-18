# sift

Fast, native Omarchy app for keyboard-and-swipe photo & video triage.

Choose a source folder and a discard destination folder, step through images and videos one by one (with the next image preloaded in memory), and apply all moves in a single batch at the end. Nothing is ever deleted — discarded files are moved to your chosen destination folder.

## Features

- **Native & fast:** C++ and Qt Quick, single binary, next image preloaded off-screen.
- **Safe batch workflow:** Decisions stay in memory until you click **Move** at the end.
- **Image & video support:** `.png`, `.jpg`, `.jpeg`, `.webp`, `.gif`, `.bmp`, `.avif`, `.svg`, `.tif`, `.mp4`, `.mov`, `.mkv`, `.webm`, `.avi`, `.m4v`.
- **Custom subfolders:** Press `N` to create subfolders inside the source folder and `1`–`9` to sort directly into them.
- **Clipboard copy:** `Ctrl+C` copies the current file URL (and image pixels for images).
- **Omarchy integration:** Follows `~/.local/state/omarchy/current/theme/colors.toml`, Hyprland `decoration:rounding`, and all 50 Omarchy languages live.

## Shortcuts

| Key | Action |
| --- | --- |
| `Space` / `→` / `L` | Keep item and advance |
| `←` / `H` / `Backspace` / `Delete` | Mark for discard folder |
| `Ctrl+Z` / `Z` | Undo last decision |
| `Ctrl+C` | Copy current image or file |
| `P` / `M` | Play/Pause or Mute video |
| `N` | Create a subfolder inside the source folder |
| `1`–`9` | Mark to move to subfolder 1–9 |
| `O` / `D` | Choose source or discard folder |
| `?` | Toggle shortcuts overlay |
| `Q` | Quit |

## Build, test, and install

```sh
./bin/build
./bin/test
./bin/install
```
