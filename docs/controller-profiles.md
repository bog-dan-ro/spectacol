# Controller profiles

Open **Options → (Virtual) Gamepad → Controller profiles**, or use the shortcut on the controller mapping page.
Save the current mapping under a name, select a saved profile and load it, or assign one to the current game.
The mapping page edits directions, keys and button actions. On-screen controls and fast-forward has the
hold/toggle choice and separate keyboard, joystick and action-button opacity settings.

A game profile overrides the defaults temporarily. Mapping edits made while it is active stay temporary until
**Save current mapping** updates a named profile. Other games and the next application session retain the global
defaults. Select **Use default mapping** to remove an assignment. Deleting a profile removes its assignments too.

Game matching uses the filename, without a media suffix, with Unicode normalization and case folding; moving or
copying the same name to another folder keeps its selection. Different games with the same filename share an
assignment, so give them distinct names or choose the profile explicitly. Quick-snapshot timestamps are ignored.
Saving a snapshot under a new name remembers that name as another entry for the current game's profile.
Drive A insertion also selects a game; drive B insertion keeps the current selection.

A single supported joystick declaration in a Z80/SZX snapshot takes priority over the profile's joystick on load.
Otherwise an assigned profile supplies the joystick and suppresses the snapshot chooser. Without a profile or a
supported declaration, the existing **Ask for the joystick** setting applies. TC/TS2068 hardware is required for
Timex joystick input; unavailable hardware falls back to None with an explanation.

## Portable files

Export writes a standalone JSON file with `format: "spectacol-controller-profile"`, `version: 1`, `name`, and
`mapping`. The mapping contains:

- `directionMode`: 0 selected joystick, 1 QAOP, 2 custom keys.
- `directionKeys`: five Qt key codes, ordered Up, Down, Left, Right, Fire.
- `joystick`: 0 None, 1 Cursor, 2 Kempston, 3 Sinclair 1, 4 Sinclair 2, 5 Fuller, 6 Timex 1, 7 Timex 2.
- `deadZone`: 0 through 0.95, and `fastForwardToggle`: a boolean.
- `actions`: customizable Qt gamepad button codes mapped to Spectrum key codes or Spectacol action codes.
  Start stays reserved for opening the menu; D-pad directions retain their input ownership.

Import validates the whole document before changing any profile. Unsupported versions, invalid codes, malformed
or oversized files are rejected. Importing an existing name creates a numbered copy instead of overwriting it.
Export replaces local files atomically; Android document-provider destinations use the selected document directly.
Profiles and game assignments live in Spectacol's normal settings; imported files contain no machine state or media.

## Fast-forward lifecycle

Hold mode lasts until every owning input is released. Toggle mode changes on a completed press/release;
cancelled touches do not toggle. Disconnecting a controller clears its hold or latch without cancelling a separate
touch owner. Pause, backgrounding and mapping changes clear all manual overrides. Automatic tape turbo remains
independent. Playback indicators are driven by changes, with no polling timer or additional work on frame delivery.
