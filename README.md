<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Parrotfish Keyboard

Better typing for the Sailfish OS keyboard: word suggestions, autocorrect
with undo, next-word prediction and learning, in several languages at once.

- **Several languages at once.** Choose the languages you type in. The
  keyboard follows the one you are writing, word by word, so a message can
  mix them with no switching.
- **Autocorrect you can undo.** A correction is applied on Space or
  punctuation. Backspace straight after it brings back what you typed, and a
  correction you keep undoing stops. Four strengths:
  - **Strong**: also fixes real words typed by mistake.
  - **Normal**: fixes words not in the dictionaries.
  - **Gentle**: fixes only obvious slips.
  - **Off**.
- **Suggestions and next words.** The bar shows the correction first. After
  a word, it suggests the next one.
- **Learning on the phone.** Learns the words you use, the words you type
  after them, and new words once you have typed them in three different texts.
  You can see and forget learned words, or turn learning off.
- **Four arrangements:** QWERTY, QWERTZ, AZERTY and QWERTY with å ö ä. Any
  language can be typed on any of them. Accents are on long press.
- **Look & feel:** key fill and border, key height, space between keys, a
  number row, a sticky key for the last sign picked from a long press, and a
  dot on keys with more characters. A preview shows the changes as you make
  them.

Languages: Dutch, English, French, German, Italian, Portuguese (Brazil and
Portugal), Spanish. Experimental: Danish, Finnish, Norwegian Bokmål,
Romanian, Swedish. There is too little public text to build and test these
as well as the others; feedback from native speakers is welcome.

**Status:** in development, not released yet.

## Install

1. Download the RPM for your device from the
   [latest release](https://github.com/ccontino84/parrotfish-keyboard/releases/latest):
   `aarch64` for most current devices, `armv7hl` for 32-bit installations.
2. In **Settings > Untrusted software**, turn on *Allow untrusted software*.
3. Open the downloaded file (from the browser's downloads or the Files app)
   and install it. The keyboard restarts by itself.
4. In **Settings > Text input > Keyboards**, turn on the Parrotfish layout
   for your arrangement, for example *Auto (QWERTY)*. The *Auto* layouts are
   at the top of the list. Switch to it as to any other keyboard: long-press
   the spacebar, or swipe.
5. Its settings are under **Settings > Text input**, in the Parrotfish
   section. Start with *Typing languages*.

**Upgrading** works the same way: install the new RPM over the old one.
Your settings and learned words are kept.

With Developer mode, the same from a terminal:

```sh
devel-su pkcon install-local parrotfish-keyboard-*.rpm
```

## Uninstall

From a terminal (Developer mode):

```sh
devel-su pkcon remove parrotfish-keyboard
```

Settings stay in dconf and learned words in your home directory. To remove
them too:

```sh
dconf reset -f /apps/parrotfish-keyboard/
rm -r ~/.local/share/parrotfish-keyboard
```

## Safety and privacy

- **Nothing leaves the phone.** The keyboard makes no network connections.
  Learned words are stored only on the phone. *Clear all learned data*, in
  the settings, removes them.
- **No system file is changed.** Parrotfish adds its own layouts, handler
  and settings to the stock keyboard. Jolla's own layouts are left as they
  are.
- **Bug reports:** *Report a bug* in the settings opens a GitHub issue. It
  fills in diagnostics: the versions, the keyboards and settings in use, and
  counts of learned data. They contain no words you typed. You see the
  report before sending it.

## Development

Building, testing, and how it works inside: [DEVELOPMENT.md](DEVELOPMENT.md).
Third-party data and its licences: [THIRD-PARTY.md](THIRD-PARTY.md).

This project is developed with AI assistance.

## License

GNU Lesser General Public License, version 2.1 or (at your option) any later
version (LGPL-2.1-or-later), see [LICENSE](LICENSE). Parts adapted from
Jolla's keyboard are also under BSD-3-Clause, and the dictionary data keeps
its own licences (Apache-2.0, CC0, CC BY 2.0 FR): see
[THIRD-PARTY.md](THIRD-PARTY.md).
Copyright (c) 2026 [ccontino84](https://github.com/ccontino84).
