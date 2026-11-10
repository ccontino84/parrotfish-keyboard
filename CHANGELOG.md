<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Changelog

## 1.1.0 (2026-11-10)

- **Language keys:** the most used accented letters of the language you
  are typing get keys of their own (å ö ä for Swedish, ł ę ż for Polish,
  é è à for French), and change with the language. Off by default, as
  they make the letters narrower (on QWERTY and AZERTY, rows of 11 keys
  instead of 10): turn them on in Look & feel, on the right or the left.
- **Auto (QWERTZ) without language keys** has no ü ö ä keys any more, and
  wider letters instead; ü ö ä are on long press, and autocorrect often
  puts them back when typed plain. Turn Language keys on for 1.0's keys.
- **The Auto (QWERTY ÅÖÄ) layout is gone:** with Language keys on and
  Swedish or Finnish typed, Auto (QWERTY) has å ö ä in the same places, and
  Danish and Norwegian get their own å æ ø there. After the update, a phone
  that had it on switches to the next keyboard in its list; if it was the
  only one, no keyboard opens until you pick one. Either way, turn on Auto
  (QWERTY) in Settings > Text input > Keyboards, and Language keys in
  Parrotfish's Look & feel.
- **New languages:** Czech, Polish and Slovenian, and Croatian as
  experimental, with their letters on long press and the settings
  translated. The Slovenian dictionary comes from the Sloleks lexicon and
  the Kres corpus (CC BY-SA 4.0), so `parrotfish-languages-latin` now
  includes CC BY-SA 4.0 data (see THIRD-PARTY.md).
- The Typing settings page has the live preview too, and both previews
  have a clear button.
- Picking a suggestion now does everything Space does: the word counts
  towards the language the keyboard follows, and the next word is
  capitalised when it should be.
- Data fixes: the word-pair counts had included the benchmarks' test
  sentences, so the next-word numbers published for 1.0 were slightly too
  high (by 0.1–0.4 points for the main languages, 2–4 for the experimental
  ones); prediction on other text is unchanged. The word lists of every
  language were regenerated, after a fix that had dropped real words also
  used in another language.

All three packages have changed: install the new keyboard and language
RPMs together. Native speakers of the new languages: corrections to the
dictionaries, the language keys and the translations are welcome.

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
