#!/bin/sh
# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Regenerates data/rules/<lang>.{respellings,misspellings,supplement,forms,boost}, the starting
# next-word data data/pairs/<lang>.tsv.gz and the test lists
# tests/data/unknown-<lang>.txt from the sentence corpora.
#
#   tools/data/regenerate-lists.sh CORPUS_DIR [--fetch | --reuse-counts] [LANG...]
#
# --fetch downloads Common Voice (sparse clone) and the Tatoeba exports into
# CORPUS_DIR first. --reuse-counts keeps the word counts made by an earlier
# run (CORPUS_DIR/<src>-{cv,tatoeba,all}.tsv). Without LANG, all languages.
# Review the diff of data/rules afterwards.
set -eu
C=$1
shift
cd "$(dirname "$0")/../.."
mkdir -p "$C" build/data

ALL="en it de es fr pt_BR pt_PT nl sv da nb ro fi pl cs sl hr"

# The corpus language of a dictionary: both Portuguese lists share one.
src() { case $1 in pt_*) echo pt ;; *) echo $1 ;; esac; }
cvdir() { case $1 in sv) echo sv-SE ;; nb) echo nb-NO ;; *) src $1 ;; esac; }
# The wordlist of a language (data/CMakeLists.txt): AOSP's, or sl's from Sloleks.
aosp() {
    case $1 in
        en) echo data/aosp/en_US_wordlist.combined.gz ;; sl) echo data/sloleks/sl_wordlist.combined.gz ;;
        *) echo data/aosp/$1_wordlist.combined.gz ;;
    esac
}
tatoeba() {
    case $(src $1) in
        en) echo eng ;; it) echo ita ;; de) echo deu ;; es) echo spa ;; fr) echo fra ;; pt) echo por ;;
        nl) echo nld ;; sv) echo swe ;; da) echo dan ;; nb) echo nob ;; ro) echo ron ;; fi) echo fin ;;
        pl) echo pol ;; cs) echo ces ;; sl) echo slv ;; hr) echo hrv ;;
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
COUNT=1
if [ "${1:-}" = "--reuse-counts" ]; then
    shift
    COUNT=
fi
LANGS=${*:-$ALL}
echo "Common Voice commit: $(git -C "$C/cv" rev-parse HEAD)"

# The test sentences (tests/data) are left out of every count.
exclude() { echo "--exclude tests/data/sentences-$(src $1).txt --exclude tests/data/tatoeba-$(src $1).txt"; }

for s in $(for l in $LANGS; do src $l; done | sort -u); do
    [ -n "$COUNT" ] || break
    t="$C/$(tatoeba $s)_sentences.tsv.bz2"
    python3 -I tools/data/count-words.py "$C/$s-cv.tsv" $(cvfiles $s) $(exclude $s) &
    python3 -I tools/data/count-words.py "$C/$s-tatoeba.tsv" "$t" $(exclude $s) &
    python3 -I tools/data/count-words.py "$C/$s-all.tsv" $(cvfiles $s) "$t" $(exclude $s) &
done
wait

for l in $LANGS; do
    s=$(src $l)
    # Common words of the other languages are not added to this one (the
    # other Portuguese list is the same language).
    others=""
    for m in $ALL; do
        [ $(src $m) = $s ] || others="$others --other $(aosp $m) --other-counts $C/$(src $m)-all.tsv"
    done
    # Old spellings first: the other lists read the dictionary with them.
    python3 -I tools/data/find-respellings.py $l "$(aosp $l)" "$C/$s-all.tsv" data/rules data/rules/$l.respellings
    python3 -I tools/data/find-accent-misspellings.py $l "$(aosp $l)" "$C/$s-all.tsv" data/rules data/rules/$l.misspellings
    python3 -I tools/data/find-supplement.py $l "$(aosp $l)" "$C/$s-cv.tsv" "$C/$s-tatoeba.tsv" data/rules \
        data/rules/$l.supplement $others
    python3 -I tools/data/complete-paradigms.py $l "$(aosp $l)" data/rules/$l.supplement "$C/$s-all.tsv" data/rules \
        data/rules/$l.forms
    python3 -I tools/data/find-boosts.py $l "$(aosp $l)" data/rules/$l.supplement "$C/$s-tatoeba.tsv" data/rules \
        data/rules/$l.boost
    python3 -I tools/data/make-oov-list.py $l "$C/$s-cv.tsv" "$C/$s-tatoeba.tsv" "$(aosp $l)" data/rules \
        tests/data/unknown-$l.txt $([ $l = da ] && echo --no-cv)
done

# Word pairs counted elsewhere, added to a language's own (count-pairs.py):
# sl, the Kres corpus n-grams 2.0 (CC BY-SA 4.0, hdl.handle.net/11356/1193:
# all_1-5-grams_Kres.zip, its 2-gram file, in CORPUS_DIR/kres/).
morepairs() {
    case $1 in
        sl) echo "--bigrams $C/kres/kres_lc_c-no_n-2_t-1_x-1.txt --corpus-weight 300" ;;
    esac
}
for l in $LANGS; do
    python3 -I tools/data/count-pairs.py data/pairs/$l.tsv.gz "$(aosp $l)" data/rules/$l.supplement data/rules \
        $(cvfiles $l) "$C/$(tatoeba $l)_sentences.tsv.bz2" $(exclude $l) --respellings data/rules/$l.respellings \
        $(morepairs $l) &
done
wait
