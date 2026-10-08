#!/bin/sh
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
#
# Regenerates data/rules/<lang>.{misspellings,supplement} and the test lists
# build/data/oov-<lang>.txt from the sentence corpora.
#
#   tools/regenerate-lists.sh CORPUS_DIR [--fetch]
#
# --fetch downloads Common Voice (sparse clone) and the Tatoeba exports into
# CORPUS_DIR first. Review the diff of data/rules afterwards.
set -eu
C=$1
cd "$(dirname "$0")/.."
mkdir -p "$C" build/data

if [ "${2:-}" = "--fetch" ]; then
    if [ ! -d "$C/cv" ]; then
        git clone --depth 1 --filter=blob:none --sparse https://github.com/common-voice/common-voice "$C/cv"
        git -C "$C/cv" sparse-checkout set server/data/en server/data/it server/data/de
    fi
    for t in eng ita deu; do
        curl -sSfL -o "$C/${t}_sentences.tsv.bz2" \
            "https://downloads.tatoeba.org/exports/per_language/$t/${t}_sentences.tsv.bz2"
    done
fi
echo "Common Voice commit: $(git -C "$C/cv" rev-parse HEAD)"

aosp() { case $1 in en) echo data/aosp/en_US_wordlist.combined.gz ;; *) echo data/aosp/$1_wordlist.combined.gz ;; esac; }
tatoeba() { case $1 in en) echo eng ;; it) echo ita ;; de) echo deu ;; esac; }
LANGS="en it de"

for l in $LANGS; do
    # Lists of foreign phrases and place names are not running text.
    cv=$(ls "$C"/cv/server/data/$l/*.txt | grep -v -e DictionaryofForeignPhrases -e countries-and-cities -e german-cities)
    t="$C/$(tatoeba $l)_sentences.tsv.bz2"
    python3 -I tools/count-words.py "$C/$l-cv.tsv" $cv &
    python3 -I tools/count-words.py "$C/$l-tatoeba.tsv" "$t" &
    python3 -I tools/count-words.py "$C/$l-all.tsv" $cv "$t" &
done
wait

for l in $LANGS; do
    others=""
    for m in $LANGS; do [ $m = $l ] || others="$others --other $(aosp $m)"; done
    python3 -I tools/find-accent-misspellings.py $l "$(aosp $l)" "$C/$l-all.tsv" data/rules data/rules/$l.misspellings
    python3 -I tools/find-supplement.py $l "$(aosp $l)" "$C/$l-cv.tsv" "$C/$l-tatoeba.tsv" data/rules \
        data/rules/$l.supplement $others
    python3 -I tools/make-oov-list.py "$C/$l-cv.tsv" "$(aosp $l)" data/rules/$l.supplement build/data/oov-$l.txt
done
