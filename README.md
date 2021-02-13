# TR1Main

This is a dynamic library for the classic Tomb Raider I game (TombATI version).
The purpose of the library is to reimplement all the routines performed by the
game and enhance the gameplay with new options.

This project is inspired by Arsunt's
[TR2Main](https://github.com/Arsunt/TR2Main/) project.

## Installing

Get a copy of the latest release from
[here](https://github.com/rr-/TR1Main/releases) and unpack the contents to your
game directory. Make sure you overwrite existing files.

## Configuring

To configure TR1Main, edit the `TR1Main.json` file in your text editor such as
Notepad.

Currently the following configuration options are supported:

- `disable_medpacks`: removes all medpacks (for No Meds challenge runs).
- `disable_magnums`: removes all magnum guns and ammo pickups.
- `disable_uzis`: removes all UZI guns and ammo pickups.
- `disable_shotgun`: removes all shotgun and shotgun shells pickups.
- `disable_healing_between_levels`: disables healing Lara between level reloads
  (for No Heal challenge runs).
- `enable_red_healthbar`: replaces the default golden healthbar with a red one.
- `enable_enemy_healthbar`: enables showing healthbar for the active enemy.
- `enable_look_while_running`: allows the player to look while running, jumping
  etc. (similar to TR2 style).
- `fix_end_of_level_freeze`: fix game freeze when ending the level with the
  Action key held.
- `fix_tihocan_secret_sound`: disable the secret sound incorrectly playing
  during using the golden key in Tomb of Tihocan.

## Decompilation progress

![](docs/progress.svg)

## License

This project is licensed under the GNU General Public License - see the
[COPYING.md](COPYING.md) file for details.

## Copyright

(c) 2021 Marcin Kurczewski. All rights reserved. Original game is created by
Core Design Ltd. in 1996. Lara Croft and Tomb Raider are trademarks of Square
Enix Ltd.
