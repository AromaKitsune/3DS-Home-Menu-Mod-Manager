# 3DS Home Menu Mod Manager

A Nintendo 3DS homebrew app designed to seamlessly swap LayeredFS-based Home
Menu mods.

![](/screenshots/app.png)

## Prerequisites
* A console running Luma3DS custom firmware.
* "Enable game patching" must be activated in the Luma3DS configuration menu.

## Installation & mods setup
1. Copy the `.3dsx` file to the `/3ds/` folder on the SD card.
2. Place Home Menu mods inside `sdmc:/luma/titles/`.
3. Inactive mod folders must follow the naming convention:
   * `<TitleID> [<Mod Name>]`
   * Example: `0004003000009802 [My Custom Home Menu UI]`
4. Every mod folder must contain a plain text file named `mod_name.txt`. The
   contents of this text file dictate the exact name displayed within the app
   interface (example: `My Custom Home Menu UI`). If this file is missing, the
   app defaults to displaying "Unknown Mod".

![](/screenshots/mods-setup.png)

You can find my
[Home Menu mod](https://aromakitsune.github.io/3DS-Custom-Home-Menu-UI) here,
and more mods on the bottom of the page.

## Usage
* Launch the app via the Homebrew Launcher. The app displays all available mods
  on the top screen.
  When the mod is activated, the inactive mod folder `<TitleID> [<Mod Name>]`
  is renamed to `<TitleID>`.
* Once the mod is applied, press `SELECT` to reboot the system.

## Compatible system regions:
* EUR - `0004003000009802`
* USA - `0004003000008F02`
* JPN - `0004003000008202`
* KOR - `000400300000A902`
* CHN - `000400300000B102`
* TWN - `000400300000B202`
