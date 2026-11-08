<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Changelog

## 1.0.0 (2026-11-08)

The first release.

- Word suggestions, next-word prediction and autocorrect with undo, in
  several languages at once, following the language word by word: Dutch,
  English, French, German, Italian, Portuguese (Brazil and Portugal) and
  Spanish; Danish, Finnish, Norwegian Bokmål, Romanian and Swedish as
  experimental. Four autocorrect strengths: Strong, Normal (the default),
  Gentle, Off. Chat shorthands (idk, btw) and abbreviations typed in
  lower case (ok, sms) are left as typed.
- Learning on the phone: your words, the words after them, and corrections
  you undo. Learned words can be seen and forgotten, per language.
- Four layouts in Jolla's list: Auto (QWERTY), Auto (QWERTZ), Auto (AZERTY)
  and Auto (QWERTY ÅÖÄ), each for any language; long-press menus with your
  languages' accents first, and a sticky key.
- Settings in Text input, translated into all the keyboard's languages:
  typing languages, autocorrect, learning, and the look of the keys with a
  live preview.
- Three packages: `parrotfish-keyboard` for your device, and the
  Latin-script languages in `parrotfish-languages-latin` and
  `parrotfish-languages-latin-experimental` (the same files for every
  device). See the README for installing.

Tested on Sailfish OS 5.1 and 5.2 (aarch64). The armv7hl build is untested:
reports are welcome. So are corrections from native speakers to the
translations and to the lists of chat shorthands (`data/rules/*.shorthands`). To report a bug: double-tap the version in Parrotfish's settings,
then *Report a bug* (it fills in diagnostics, never words you typed).
