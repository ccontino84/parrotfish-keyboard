<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Parrotfish Keyboard

Better typing for the Sailfish OS keyboard, in several languages at once.

**Why Parrotfish?** Writing in more than one language on Sailfish means
switching keyboards again and again, often in the middle of a sentence.
Parrotfish follows the language you are writing instead. And it stays just
a keyboard: fully open source, it changes nothing in the system, never goes
online, and leaves out everything that isn't about typing.

What it adds to Jolla's keyboard:

- **Your languages, word by word.** Pick them in the settings: suggestions,
  corrections and learning follow the language of each word, so one message
  can mix them.
- **Suggestions, autocorrect and learning.** Corrections (even of two words
  typed without a space) and next words, improved by what you type, learned
  only on the phone. One Backspace undoes a correction, and one you keep
  undoing stops. Four strengths, from Strong (also fixes real words typed by
  mistake) to Off.
- **Emoji.** A button at the end of the suggestion bar opens an emoji
  panel, with your recent emoji first, skin tones and gender forms on long
  press, in colour. Type an emoticon such as `:-)` and the button offers
  its emoji.
- **Typing aids,** each one on or off: a period on double space, a space
  before ! ? : ; (as in French), and emoticons replaced by their emoji.
- **Three arrangements:** QWERTY, QWERTZ and AZERTY, for any language.
  Optional language keys put the most used accented letters of the
  language you are typing on keys of their own (å ö ä for Swedish, ł ę ż
  for Polish), and change with it. A dot marks the keys with more characters; a long press
  shows your languages' accents first, and the sticky key keeps the last
  sign you picked.
- **Customizable, and at home on Sailfish:** it looks like Jolla's keyboard,
  and key fill, border, height, spacing, the width of the space key, the
  number row and hints can be changed, with a live preview.

Languages: Czech, Dutch, English, French, German, Italian, Polish,
Portuguese (Brazil and Portugal), Slovenian, Spanish. Experimental:
Croatian, Danish, Finnish, Norwegian Bokmål, Romanian, Swedish, in a
separate package. There is too little public text
to build and test these as well as the others; feedback from native speakers
is welcome. The settings are translated into all of them.

**Status:** 1.2. Tested on Sailfish OS 5.2 (aarch64);
the 32-bit build (armv7hl) is untested, reports are welcome.

## Install

1. From the
   [latest release](https://github.com/ccontino84/parrotfish-keyboard/releases/latest),
   download:
   - `parrotfish-keyboard-<version>` for your device: `aarch64` for most
     current devices, `armv7hl` for 32-bit installations;
   - `parrotfish-keyboard-languages-latin-<version>.noarch`: Czech, Dutch,
     English, French, German, Italian, Polish, Portuguese, Slovenian,
     Spanish;
   - for Croatian, Danish, Finnish, Norwegian, Romanian or Swedish, also
     `parrotfish-keyboard-languages-latin-experimental-<version>.noarch`.
2. In **Settings > Untrusted software**, turn on *Allow untrusted software*.
3. Open the downloaded files (from the browser's downloads or the Files app)
   and install them, `parrotfish-keyboard` first. The keyboard restarts by
   itself. Without a language package it types without suggestions.
4. In **Settings > Text input > Keyboards**, turn on the Parrotfish layout
   for your arrangement, for example *Auto (QWERTY)*. The *Auto* layouts are
   at the top of the list. Switch to it as to any other keyboard: long-press
   the spacebar, or swipe.
5. Its settings are under **Settings > Text input**, in the Parrotfish
   section. Start with *Typing languages*.

**Upgrading** works the same way: install the new RPMs over the old ones,
`parrotfish-keyboard` first. The language packages only need updating when
the release notes say so.
Your settings and learned words are kept.

With Developer mode, the same from a terminal:

```sh
devel-su pkcon install-local parrotfish-*.rpm
```

[Harpoon](https://github.com/JuhaniLehtimaeki/Harpoon) can't install the
language packages yet, only the keyboard: use one of the ways above.

## Uninstall

From a terminal (Developer mode):

```sh
devel-su pkcon remove parrotfish-keyboard-languages-latin-experimental   # if installed
devel-su pkcon remove parrotfish-keyboard-languages-latin parrotfish-keyboard
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
  the settings, removes them. Recent emoji are kept with the settings; a
  long press in the panel removes one.
- **No system file is changed.** Parrotfish adds its own layouts, handler
  and settings to the stock keyboard. Jolla's own layouts are left as they
  are.
- **No root, no extra permissions.** It installs like any app and runs
  entirely as your user, inside Jolla's keyboard: no system services, no
  programs that run as root, no added permission rules.
- **Bug reports:** double-tap the version in Parrotfish's settings, then
  *Report a bug*. It opens a GitHub issue and fills in diagnostics: the versions, the keyboards and settings in use, and
  counts of learned data. They contain no words you typed. You see the
  report before sending it.

## Development

Building, testing, and how it works inside: [DEVELOPMENT.md](DEVELOPMENT.md).
The published methods behind each engine feature: [docs/prior-art.md](docs/prior-art.md).
Adding a typing language: [docs/adding-a-language.md](docs/adding-a-language.md).
Third-party data and its licences: [THIRD-PARTY.md](THIRD-PARTY.md).

This project is developed with AI assistance.

## License

GNU Lesser General Public License, version 2.1 or (at your option) any later
version (LGPL-2.1-or-later), see [LICENSE](LICENSE). Parts adapted from
Jolla's keyboard are also under BSD-3-Clause. The dictionaries are built
from the Android Open Source Project's wordlists (Apache-2.0) and from
sentences of Mozilla Common Voice (CC0) and Tatoeba (tatoeba.org, CC BY 2.0
FR): see [THIRD-PARTY.md](THIRD-PARTY.md).
Copyright (c) 2026 [ccontino84](https://github.com/ccontino84).
