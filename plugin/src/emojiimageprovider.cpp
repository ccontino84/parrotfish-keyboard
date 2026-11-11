// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

// C libraries first (their headers are not Qt-safe in every order).
#include <cairo-ft.h>
#include <cairo.h>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <hb.h>

#include "emojiimageprovider.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QMutexLocker>
#include <QStringList>

#include <cmath>

namespace {

// The colour emoji font's file: fontconfig's best match for colour emoji.
QByteArray emojiFontFile()
{
    QByteArray file;
    if (!FcInit())
        return file;
    FcPattern *pattern = FcNameParse(reinterpret_cast<const FcChar8 *>("emoji:color=true"));
    if (!pattern)
        return file;
    FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);
    FcResult result;
    FcPattern *match = FcFontMatch(nullptr, pattern, &result);
    if (match) {
        FcChar8 *path = nullptr;
        FcBool color = FcFalse;
        FcPatternGetBool(match, FC_COLOR, 0, &color);
        if (FcPatternGetString(match, FC_FILE, 0, &path) == FcResultMatch && color)
            file = reinterpret_cast<const char *>(path);
        FcPatternDestroy(match);
    }
    FcPatternDestroy(pattern);
    if (file.isEmpty()) {
        // Sailfish OS's own places (5.2, 5.1), if fontconfig doesn't say.
        for (const char *known : { "/usr/share/fonts/notocoloremoji/Noto-COLRv1.ttf",
                                   "/usr/share/fonts/twemoji/TwitterColorEmoji-SVGinOT.ttf" }) {
            if (QFile::exists(QString::fromLatin1(known)))
                return QByteArray(known);
        }
    }
    return file;
}

} // namespace

EmojiImageProvider::EmojiImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image, QQmlImageProviderBase::ForceAsynchronousImageLoading)
{
}

EmojiImageProvider::~EmojiImageProvider()
{
    if (m_hbFont)
        hb_font_destroy(static_cast<hb_font_t *>(m_hbFont));
    if (m_cairoFace)
        cairo_font_face_destroy(static_cast<cairo_font_face_t *>(m_cairoFace));
    // The FT_Face belongs to the hb font (referenced) and the cairo face;
    // the library is left open: cairo may still hold the face.
}

bool EmojiImageProvider::open()
{
    if (m_tried)
        return m_hbFont != nullptr;
    m_tried = true;
    const QByteArray file = emojiFontFile();
    if (file.isEmpty()) {
        qWarning() << "parrotfish: no colour emoji font found";
        return false;
    }
    FT_Library library;
    FT_Face face;
    if (FT_Init_FreeType(&library))
        return false;
    if (FT_New_Face(library, file.constData(), 0, &face)) {
        qWarning() << "parrotfish: cannot open" << file;
        return false;
    }
    m_library = library;
    m_face = face;
    m_cairoFace = cairo_ft_font_face_create_for_ft_face(face, 0);
    m_hbFont = hb_ft_font_create_referenced(face);
    qDebug() << "parrotfish: colour emoji from" << file;
    return true;
}

QImage EmojiImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int side = requestedSize.height() > 0 ? requestedSize.height()
                     : requestedSize.width() > 0 ? requestedSize.width() : 64;
    if (size)
        *size = QSize(side, side);
    QImage image(side, side, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QString text;
    for (const QString &part : id.split(QLatin1Char('-'), QString::SkipEmptyParts)) {
        bool ok = false;
        const uint code = part.toUInt(&ok, 16);
        if (!ok || code > 0x10FFFF)
            return image;
        text += QString::fromUcs4(&code, 1);
    }
    if (text.isEmpty())
        return image;

    QMutexLocker lock(&m_mutex);
    if (!open())
        return image;
    auto *hbFont = static_cast<hb_font_t *>(m_hbFont);

    hb_buffer_t *buffer = hb_buffer_create();
    const QByteArray utf8 = text.toUtf8();
    hb_buffer_add_utf8(buffer, utf8.constData(), utf8.size(), 0, utf8.size());
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(hbFont, buffer, nullptr, 0);
    unsigned count = 0;
    hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, &count);
    hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, &count);

    // The font can't draw it: a missing glyph, or a sequence left in pieces
    // (one emoji is one glyph in a font that has it).
    bool drawable = count == 1;
    for (unsigned i = 0; i < count; ++i)
        drawable = drawable && infos[i].codepoint != 0;

    if (drawable) {
        const double pixelSize = side * 0.8;
        const double scale = pixelSize / hb_face_get_upem(hb_font_get_face(hbFont));
        cairo_glyph_t glyph;
        glyph.index = infos[0].codepoint;
        glyph.x = positions[0].x_offset * scale;
        glyph.y = 0;

        cairo_surface_t *surface = cairo_image_surface_create_for_data(
            image.bits(), CAIRO_FORMAT_ARGB32, image.width(), image.height(), image.bytesPerLine());
        cairo_t *cr = cairo_create(surface);
        cairo_set_font_face(cr, static_cast<cairo_font_face_t *>(m_cairoFace));
        cairo_set_font_size(cr, pixelSize);
        // Centred on its ink.
        cairo_text_extents_t ink;
        cairo_glyph_extents(cr, &glyph, 1, &ink);
        glyph.x += (side - ink.width) / 2 - ink.x_bearing;
        glyph.y += (side - ink.height) / 2 - ink.y_bearing;
        cairo_show_glyphs(cr, &glyph, 1);
        cairo_destroy(cr);
        cairo_surface_flush(surface);
        cairo_surface_destroy(surface);
    }
    hb_buffer_destroy(buffer);
    return image;
}
