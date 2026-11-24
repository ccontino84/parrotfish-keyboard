<!--
SPDX-FileCopyrightText: 2026 ccontino84
SPDX-License-Identifier: LGPL-2.1-or-later
-->

# Changelog

## 1.4.0 (unreleased)

### New

- **Download languages from the settings.** *Typing languages* lists every
  language: tap one to download it (2–5 MB; on mobile data, tap twice),
  switch the ones on the phone on or off, and delete downloaded ones with a
  long press. Newer dictionaries are listed under *Updates*, with *Update
  all* in the pull-down menu. The keyboard itself still never connects to
  the internet: a download you start in the settings is the only network
  access, and only the files this version lists are accepted, checked by
  SHA-256. The language packages stay, for installing without a
  connection; a downloaded language is used instead of the package's.
  When a layout has no language of its alphabet, or a language needs an
  update, the suggestion bar says so.
- **Cyrillic and Greek.** Two new layouts with Jolla's keys, *Auto
  (ЙЦУКЕН)* for Russian and Ukrainian (experimental) and *Auto
  (Ελληνικά)* for Greek (experimental), downloadable in the settings or
  in two new packages, `parrotfish-keyboard-languages-cyrillic` and
  `parrotfish-keyboard-languages-greek`. Each layout types the languages of
  its alphabet. With both Russian and Ukrainian, a globe key switches
  between their keys, and the other language's letters are on long press
  (ы і, э є, ъ ї, г ґ, е ё). Greek: the final ς and the accents are added as
  you type, and Jolla's tonos key is there too.
- **Numpad:** the Layout page's new *Digits* setting offers a numpad on the
  ?123 page, with 0 under the 8, besides the number row (normal or
  smaller) or none.
- **Put the cursor right after a word to correct it:** the word is taken
  back, underlined, and the suggestions offer replacements; after a space,
  they offer the next word. In Android apps, a picked replacement can
  leave an extra space.

### Improved

- **Accented letters of other languages** are corrected too: a typo with
  à in Dutch text is now fixed like one with the language's own accents.
- **Voice input is part of the keyboard** (aarch64): the update replaces
  `parrotfish-keyboard-voice` and keeps the downloaded speech model.
- Each layout remembers its last language, so French can stay on AZERTY
  and German on QWERTZ.
- *Typing languages* is grouped by alphabet. When an alphabet's Parrotfish
  layouts are all off in Keyboards, its languages are off and greyed out
  (your choice comes back with the layout); each alphabet keeps one
  language on.
- *Look & feel* is now *Appearance*, and *Swipe to switch keyboards* is in
  *Layout*.
- *Space before ! ? : ;* applies to French text only, and can be turned
  on when French is a typing language.
- The sticky key is for the Latin layouts.
- Diagnostics list the dictionaries in use and the speech model.

## 1.3.0 (2026-11-17)

### New

- **Voice input,** recognised on the phone, in a new optional package,
  `parrotfish-keyboard-voice` (aarch64). Hold the spacebar and lift the
  finger on the microphone: the text appears phrase by phrase as you speak,
  and ✓ types it. Tap or drag over words and say the replacement; undo
  takes back the last change, and shows what it would change while you hold
  it. The mic button pauses and resumes.
- The speech model (NVIDIA Parakeet TDT 0.6B v3 as post-trained by
  Moondream, 670 MB, CC BY 4.0) is downloaded once from the new *Voice
  input* settings page, with a warning on mobile data. This download is the
  only network use in Parrotfish: the recognizer itself can't use the
  network, and the audio is neither sent nor saved.

### Improved

- **Automatic capitals can be turned off,** in *Typing aids*. Off, the
  next-word suggestions at the start of a sentence keep their usual case
  too.

The changes are in `parrotfish-keyboard` and the new
`parrotfish-keyboard-voice`; the language packages are the same as 1.2.2's
apart from the version. The new settings texts were translated without
native speakers: corrections are welcome.

## 1.2.2 (2026-11-14)

### Improved

- **A Layout page in the settings,** between Typing and Look & feel. The
  number row, the language keys and the sticky key move there from Look &
  feel, as they change which keys the keyboard has; your settings are
  kept.
- With language keys on but none of your typing languages using any, the
  setting now says so, instead of seeming to do nothing.

### Fixed

- After rotating the phone, the keys could keep the wrong widths until a
  size setting was changed (when portrait and landscape had the same key
  height).
- 1.2.1's notes said that Harpoon installs all three packages: for now it
  installs only the keyboard. Install the language packages as the README
  says.

The changes are in `parrotfish-keyboard`; the language packages are the
same as 1.2.1's apart from the version. The new settings texts were
translated without native speakers: corrections are welcome.

## 1.2.1 (2026-11-13)

### Improved

- **The language packages are renamed** `parrotfish-keyboard-languages-latin`
  and `parrotfish-keyboard-languages-latin-experimental`, so that
  installers that take a release's packages together, such as
  [Harpoon](https://github.com/JuhaniLehtimaeki/Harpoon), install and update
  all three. Installing them replaces the packages with the old names.

### Fixed

- The package description no longer lists the QWERTY ÅÖÄ layout, gone
  since 1.1.

Install all three RPMs this time; your settings and learned words are kept.

## 1.2.0 (2026-11-13)

### New

- **Emoji:** the button at the right end of the suggestion bar opens an
  emoji panel in place of the keys, and the same button (then a keyboard)
  goes back; split landscape, which has no bar, gets an emoji key. The
  panel scrolls sideways through the groups, with the recent emoji first,
  in colour. A long press offers skin tones and gender forms (the one
  picked is remembered), or removes a recent emoji. Backspace deletes a
  whole emoji, even one made of several characters. How emoji look in an
  app is up to the app.
- **Emoticons:** after typing one such as `:-)`, `<3` or `\o/`, the emoji
  button offers its emoji, and a tap puts it in place. *Replace emoticons*
  (off by default) does it on Space instead; Backspace straight after
  brings the emoticon back.
- **Typing aids:** a new page in Typing for the helpers you can turn on
  or off. New among them, and on by default: **period on double space**,
  Space twice after a word types ". " and capitalises the next word
  (Backspace straight after goes back to one space). Also there: space
  before ! ? : ; and replace emoticons.
- **Space key width,** in Look & feel: from 82% to 118% of Jolla's
  spacebar; ?123 and Enter take the difference, and are never narrower
  than Shift.

### Improved

- **Bottom row:** comma and period are as wide as the letter keys (?123
  and Enter a little narrower), and the row stays the same with language
  keys on, so that comma and period stay easy to hit.
- **Autocorrect at the start of a text:** while the keyboard isn't sure
  of the language yet, it now fixes obvious slips (what Gentle fixes),
  instead of nothing.
- Words typed before the keyboard is sure of the language are learned in
  the language it settles on, not in its first guess.
- Words and emoji typed in the settings' preview are not kept, and a line
  under it says so.

### Fixed

- **Password fields in some apps** could be typed in as normal text
  fields (the whole word underlined) until the keyboard was switched.
- Dots typed after a picked suggestion stay together (`word...`,
  `word?!`), instead of getting a space after the first one.
- With language keys on, Danish, Finnish, Norwegian and Swedish no longer
  get an é key next to M. That place gets a language key only if one of
  your languages has a letter for it.

The changes are in `parrotfish-keyboard`; the language packages are the
same as 1.1's apart from the version, so installing all three together
keeps them in step. The new settings texts were translated without native
speakers: corrections are welcome.

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
