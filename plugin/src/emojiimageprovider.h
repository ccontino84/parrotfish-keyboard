// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QMutex>
#include <QQuickImageProvider>

// Colour emoji for the emoji panel: image://parrotfish-emoji/<code points>,
// the code points in hex joined by "-" (1f9d1-200d-1f4bb). Qt 5.6 draws
// emoji only in black and white, so each is drawn here from the system's
// colour emoji font (found through fontconfig: Noto Color Emoji, COLRv1, on
// Sailfish OS 5.2) with HarfBuzz (sequences: tones, flags, ZWJ) and cairo,
// which draws COLRv1 and SVG glyphs. Images are made off the GUI thread; an
// emoji the font can't draw comes back empty (fully transparent).
class EmojiImageProvider : public QQuickImageProvider
{
public:
    EmojiImageProvider();
    ~EmojiImageProvider() override;

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    bool open();

    QMutex m_mutex;
    bool m_tried = false;
    void *m_library = nullptr;   // FT_Library
    void *m_face = nullptr;      // FT_Face
    void *m_cairoFace = nullptr; // cairo_font_face_t
    void *m_hbFont = nullptr;    // hb_font_t
};
