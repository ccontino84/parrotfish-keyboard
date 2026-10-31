#!/bin/sh
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD
#
# Regenerates data/rules/<lang>.{misspellings,supplement,forms,boost}, the starting
# next-word data data/pairs/<lang>.tsv.gz and the test lists
# build/data/oov-<lang>.txt from the sentence corpora.
#
#   tools/regenerate-lists.sh CORPUS_DIR [--fetch] [LANG...]
#
# --fetch downloads Common Voice (sparse clone) and the Tatoeba exports into
# CORPUS_DIR first. Without LANG, all languages. Review the diff of
# data/rules afterwards.
set -eu
C=$1
shift
cd "$(dirname "$0")/.."
mkdir -p "$C" build/data

ALL="en it de es fr pt_BR pt_PT nl sv da nb ro fi"

# The corpus language of a dictionary: both Portuguese lists share one.
src() { case $1 in pt_*) echo pt ;; *) echo $1 ;; esac; }
cvdir() { case $1 in sv) echo sv-SE ;; nb) echo nb-NO ;; *) src $1 ;; esac; }
aosp() { case $1 in en) echo data/aosp/en_US_wordlist.combined.gz ;; *) echo data/aosp/$1_wordlist.combined.gz ;; esac; }
tatoeba() {
    case $(src $1) in
        en) echo eng ;; it) echo ita ;; de) echo deu ;; es) echo spa ;; fr) echo fra ;; pt) echo por ;;
        nl) echo nld ;; sv) echo swe ;; da) echo dan ;; nb) echo nob ;; ro) echo ron ;; fi) echo fin ;;
    esac
}
# Common Voice files that are not running text: lists of foreign phrases,
# places, addresses and single words.
cvfiles() {
    ls "$C"/cv/server/data/$(cvdir $1)/*.txt | grep -v -e DictionaryofForeignPhrases -e countries-and-cities \
        -e german-cities -e singleword-benchmark -e /addresses- -e lulucmy-places -e ofis_publik
}

if [ "${1:-}" = "--fetch" ]; then
    shift
    if [ ! -d "$C/cv" ]; then
        git clone --depth 1 --filter=blob:none --sparse https://github.com/common-voice/common-voice "$C/cv"
        git -C "$C/cv" sparse-checkout set $(for l in $ALL; do echo server/data/$(cvdir $l); done | sort -u)
    fi
    for t in $(for l in $ALL; do tatoeba $l; done | sort -u); do
        curl -sSfL -o "$C/${t}_sentences.tsv.bz2" \
            "https://downloads.tatoeba.org/exports/per_language/$t/${t}_sentences.tsv.bz2"
    done
fi
LANGS=${*:-$ALL}
echo "Common Voice commit: $(git -C "$C/cv" rev-parse HEAD)"

# The test sentences (tests/data) are left out of every count.
exclude() { echo "--exclude tests/data/sentences-$(src $1).txt --exclude tests/data/tatoeba-$(src $1).txt"; }

for s in $(for l in $LANGS; do src $l; done | sort -u); do
    t="$C/$(tatoeba $s)_sentences.tsv.bz2"
    python3 -I tools/count-words.py "$C/$s-cv.tsv" $(cvfiles $s) $(exclude $s) &
    python3 -I tools/count-words.py "$C/$s-tatoeba.tsv" "$t" $(exclude $s) &
    python3 -I tools/count-words.py "$C/$s-all.tsv" $(cvfiles $s) "$t" $(exclude $s) &
done
wait

for l in $LANGS; do
    s=$(src $l)
    # Common words of the other languages are not added to this one (the
    # other Portuguese list is the same language).
    others=""
    for m in $ALL; do [ $(src $m) = $s ] || others="$others --other $(aosp $m)"; done
    python3 -I tools/find-accent-misspellings.py $l "$(aosp $l)" "$C/$s-all.tsv" data/rules data/rules/$l.misspellings
    python3 -I tools/find-supplement.py $l "$(aosp $l)" "$C/$s-cv.tsv" "$C/$s-tatoeba.tsv" data/rules \
        data/rules/$l.supplement $others
    python3 -I tools/complete-paradigms.py $l "$(aosp $l)" data/rules/$l.supplement "$C/$s-all.tsv" data/rules \
        data/rules/$l.forms
    python3 -I tools/find-boosts.py $l "$(aosp $l)" data/rules/$l.supplement "$C/$s-tatoeba.tsv" data/rules \
        data/rules/$l.boost
    python3 -I tools/make-oov-list.py "$C/$s-cv.tsv" "$(aosp $l)" data/rules/$l.supplement build/data/oov-$l.txt
done

for l in $LANGS; do
    python3 -I tools/count-pairs.py data/pairs/$l.tsv.gz "$(aosp $l)" data/rules/$l.supplement data/rules \
        $(cvfiles $l) "$C/$(tatoeba $l)_sentences.tsv.bz2" $(exclude $l) &
done
wait
