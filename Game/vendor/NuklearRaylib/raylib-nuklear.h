/**********************************************************************************************
*
*   raylib-nuklear - Nuklear for Raylib.
*
*   FEATURES:
*       - Use the nuklear immediate-mode graphical user interface in raylib.
*
*   DEPENDENCIES:
*       - raylib 4.2 https://www.raylib.com/
*       - nuklear https://github.com/Immediate-Mode-UI/Nuklear
*
*   LICENSE: zlib/libpng
*
*   raylib-nuklear is licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software:
*
*   Copyright (c) 2020 Rob Loach (@RobLoach)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

/*
* LICENSE
* This source is altered from the original
*/

#ifndef RAYLIB_NUKLEAR_H
#define RAYLIB_NUKLEAR_H

#include <raylib/src/raylib.h>
#include <math.h>

// Nuklear defines
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_COMMAND_USERDATA
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_SINGLE_FILE 0
// TODO: Replace NK_INCLUDE_DEFAULT_ALLOCATOR with MemAlloc() and MemFree()
#include <Nuklear/nuklear.h>

// TODO: Figure out if we can use STANDARD_BOOL here?
//#define NK_INCLUDE_STANDARD_BOOL
//#ifndef NK_BOOL
//#define NK_BOOL bool
//#endif  // NK_BOOL

#ifndef NK_ASSERT
#define NK_ASSERT(condition) do { if (!(condition)) { TraceLog(LOG_WARNING, "NUKLEAR: Failed assert \"%s\" (%s:%i)", #condition, "nuklear.h", __LINE__); }} while (0)
#endif  // NK_ASSERT

// Note(bscal): struct definition from SUI.h
struct nk_sprite
{
    nk_handle handle;
    nk_ushort region[4];
    struct nk_vec2 origin;
    float rotation;
};

struct nk_command_scal_sprite
{
    struct nk_command header;
    short x, y;
    unsigned short w, h;
    struct nk_sprite sprite;
    struct nk_color col;
};

#ifdef __cplusplus
extern "C" {
#endif

NK_API void UpdateNuklear(struct nk_context * ctx);                 // Update the input state and internal components for Nuklear
NK_API void DrawNuklear(struct nk_context * ctx);                   // Render the Nuklear GUI on the screen
NK_API struct nk_color ColorToNuklear(Color color);                 // Convert a raylib Color to a Nuklear color object
NK_API struct nk_colorf ColorToNuklearF(Color color);               // Convert a raylib Color to a Nuklear floating color
NK_API struct Color ColorFromNuklear(struct nk_color color);        // Convert a Nuklear color to a raylib Color
NK_API struct Color ColorFromNuklearF(struct nk_colorf color);      // Convert a Nuklear floating color to a raylib Color
NK_API struct Rectangle RectangleFromNuklear(struct nk_context * ctx, struct nk_rect rect); // Convert a Nuklear rectangle to a raylib Rectangle
NK_API struct nk_rect RectangleToNuklear(struct nk_context * ctx, Rectangle rect); // Convert a raylib Rectangle to a Nuklear Rectangle
NK_API struct nk_image TextureToNuklear(Texture tex);               // Convert a raylib Texture to A Nuklear image
NK_API struct Texture TextureFromNuklear(struct nk_image img);      // Convert a Nuklear image to a raylib Texture
NK_API struct nk_image LoadNuklearImage(const char* path);          // Load a Nuklear image
NK_API void UnloadNuklearImage(struct nk_image img);                // Unload a Nuklear image. And free its data
NK_API void CleanupNuklearImage(struct nk_image img);               // Frees the data stored by the Nuklear image
NK_API void SetNuklearScaling(struct nk_context * ctx, float scaling); // Sets the scaling for the given Nuklear context
NK_API float GetNuklearScaling(struct nk_context * ctx);            // Retrieves the scaling of the given Nuklear context

#ifdef __cplusplus
}
#endif

#endif  // RAYLIB_NUKLEAR_H

#ifdef RAYLIB_NUKLEAR_IMPLEMENTATION
#ifndef RAYLIB_NUKLEAR_IMPLEMENTATION_ONCE
#define RAYLIB_NUKLEAR_IMPLEMENTATION_ONCE

// Math
#ifndef NK_COS
#define NK_COS cosf
#endif  // NK_COS
#ifndef NK_SIN
#define NK_SIN sinf
#endif  // NK_SIN
#ifndef NK_INV_SQRT
#define NK_INV_SQRT(value) (1.0f / sqrtf(value))
#endif  // NK_INV_SQRT

#define NK_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include <Nuklear/nuklear.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RAYLIB_NUKLEAR_DEFAULT_FONTSIZE
/**
 * The default font size that is used when a font size is not provided.
 */
#define RAYLIB_NUKLEAR_DEFAULT_FONTSIZE 10
#endif  // RAYLIB_NUKLEAR_DEFAULT_FONTSIZE

#ifndef RAYLIB_NUKLEAR_DEFAULT_ARC_SEGMENTS
/**
 * The amount of segments used when drawing an arc.
 *
 * @see NK_COMMAND_ARC_FILLED
 */
#define RAYLIB_NUKLEAR_DEFAULT_ARC_SEGMENTS 20
#endif  // RAYLIB_NUKLEAR_DEFAULT_ARC_SEGMENTS

/**
 * The user data that's leverages internally through Nuklear.
 */
typedef struct NuklearUserData {
    float scaling;
    float deltaTime;
} NuklearUserData;

/**
 * Nuklear callback; Get the width of the given text.
 *
 * @internal
 */
NK_API float
nk_raylib_font_get_text_width(nk_handle handle, float height, const char *text, int len)
{
    NK_UNUSED(handle);

    if (len > 0) {
        // Grab the text with the cropped length so that it only measures the desired string length.
        const char* subtext = TextSubtext(text, 0, len);

        return (float)MeasureText(subtext, (int)height);
    }

    return 0;
}

/**
 * Nuklear callback; Get the width of the given text (userFont version)
 *
 * @internal
 */
NK_API float
nk_raylib_font_get_text_width_user_font(nk_handle handle, float height, const char *text, int len)
{
    if (len > 0) {
        // Grab the text with the cropped length so that it only measures the desired string length.
        const char* subtext = TextSubtext(text, 0, len);

        // Spacing is determined by the font size divided by 10.
        return MeasureTextEx(*(Font*)handle.ptr, subtext, height, height / 10.0f).x;
    }

    return 0;
}

/**
 * Nuklear callback; Paste the current clipboard.
 *
 * @internal
 */
NK_API void
nk_raylib_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = GetClipboardText();
    NK_UNUSED(usr);
    if (text != NULL) {
        nk_textedit_paste(edit, text, (int)TextLength(text));
    }
}

/**
 * Nuklear callback; Copy the given text.
 *
 * @internal
 */
NK_API void
nk_raylib_clipboard_copy(nk_handle usr, const char *text, int len)
{
    NK_UNUSED(usr);
    NK_UNUSED(len);
    SetClipboardText(text);
}

/**
 * Convert the given Nuklear color to a raylib color.
 */
NK_API Color
ColorFromNuklear(struct nk_color color)
{
    Color rc;
    rc.a = color.a;
    rc.r = color.r;
    rc.g = color.g;
    rc.b = color.b;
    return rc;
}

/**
 * Convert the given raylib color to a Nuklear color.
 */
NK_API struct nk_color
ColorToNuklear(Color color)
{
    struct nk_color rc;
    rc.a = color.a;
    rc.r = color.r;
    rc.g = color.g;
    rc.b = color.b;
    return rc;
}

/**
 * Convert the given Nuklear float color to a raylib color.
 */
NK_API Color
ColorFromNuklearF(struct nk_colorf color)
{
    return ColorFromNuklear(nk_rgba_cf(color));
}

/**
 * Convert the given raylib color to a raylib float color.
 */
NK_API struct nk_colorf
ColorToNuklearF(Color color)
{
    return nk_color_cf(ColorToNuklear(color));
}

/**
 * Draw the given Nuklear context in raylib.
 *
 * @param ctx The nuklear context.
 */
NK_API void
DrawNuklear(struct nk_context * ctx)
{
    const struct nk_command *cmd;
    const float scale = GetNuklearScaling(ctx);

    nk_foreach(cmd, ctx) {
        switch (cmd->type) {
            case NK_COMMAND_NOP: {
                break;
            }

            case NK_COMMAND_SCISSOR: {
                // TODO(RobLoach): Verify if NK_COMMAND_SCISSOR works.
                const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
                BeginScissorMode((int)(s->x * scale), (int)(s->y * scale), (int)(s->w * scale), (int)(s->h * scale));
            } break;

            case NK_COMMAND_LINE: {
                const struct nk_command_line *l = (const struct nk_command_line *)cmd;
                Color color = ColorFromNuklear(l->color);
                Vector2 startPos = {(float)l->begin.x * scale, (float)l->begin.y * scale};
                Vector2 endPos = {(float)l->end.x * scale, (float)l->end.y * scale};
                DrawLineEx(startPos, endPos, l->line_thickness * scale, color);
            } break;

            case NK_COMMAND_CURVE: {
                const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
                Color color = ColorFromNuklear(q->color);
                // Vector2 start = {(float)q->begin.x, (float)q->begin.y};
                Vector2 start = {(float)q->begin.x * scale, (float)q->begin.y * scale};
                // Vector2 controlPoint1 = (Vector2){q->ctrl[0].x, q->ctrl[0].y};
                // Vector2 controlPoint2 = (Vector2){q->ctrl[1].x, q->ctrl[1].y};
                // Vector2 end = {(float)q->end.x, (float)q->end.y};
                Vector2 end = {(float)q->end.x * scale, (float)q->end.y * scale};
                // TODO: Encorporate segmented control point bezier curve?
                // DrawLineBezier(start, controlPoint1, (float)q->line_thickness, color);
                // DrawLineBezier(controlPoint1, controlPoint2, (float)q->line_thickness, color);
                // DrawLineBezier(controlPoint2, end, (float)q->line_thickness, color);
                // DrawLineBezier(start, end, (float)q->line_thickness, color);
                DrawLineBezier(start, end, (float)q->line_thickness * scale, color);
            } break;

            case NK_COMMAND_RECT: {
                const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
                Color color = ColorFromNuklear(r->color);
                Rectangle rect = {(float)r->x * scale, (float)r->y * scale, (float)r->w * scale, (float)r->h * scale};
                if (r->rounding > 0) {
                    float roundness = (float)r->rounding * 4.0f / (rect.width + rect.height);
                    // TODO: DrawRectangleRoundedLines - Is 1 the correct line segments?
                    DrawRectangleRoundedLines(rect, roundness, 1, r->line_thickness * scale, color);
                }
                else {
                    DrawRectangleLinesEx(rect, r->line_thickness * scale, color);
                }
            } break;

            case NK_COMMAND_RECT_FILLED: {
                const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
                Color color = ColorFromNuklear(r->color);
                Rectangle rect = {(float)r->x * scale, (float)r->y * scale, (float)r->w * scale, (float)r->h * scale};
                if (r->rounding > 0) {
                    float roundness = (float)r->rounding * 4.0f / (rect.width + rect.height);
                    DrawRectangleRounded(rect, roundness, 1, color);
                }
                else {
                    DrawRectangleRec(rect, color);
                }
            } break;

            case NK_COMMAND_RECT_MULTI_COLOR: {
                const struct nk_command_rect_multi_color* rectangle = (const struct nk_command_rect_multi_color *)cmd;
                Rectangle position = {(float)rectangle->x * scale, (float)rectangle->y * scale, (float)rectangle->w * scale, (float)rectangle->h * scale};
                Color left = ColorFromNuklear(rectangle->left);
                Color top = ColorFromNuklear(rectangle->top);
                Color bottom = ColorFromNuklear(rectangle->bottom);
                Color right = ColorFromNuklear(rectangle->right);
                DrawRectangleGradientEx(position, left, bottom, right, top);
            } break;

            case NK_COMMAND_CIRCLE: {
                const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
                Color color = ColorFromNuklear(c->color);
                DrawEllipseLines((int)(c->x * scale + c->w * scale / 2.0f), (int)(c->y * scale + c->h * scale / 2.0f), c->w * scale / 2.0f, c->h * scale / 2.0f, color);
            } break;

            case NK_COMMAND_CIRCLE_FILLED: {
                const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
                Color color = ColorFromNuklear(c->color);
                DrawEllipse((int)(c->x * scale + c->w * scale / 2.0f), (int)(c->y * scale + c->h * scale / 2.0f), c->w * scale / 2.0f, c->h * scale / 2.0f, color);
            } break;

            case NK_COMMAND_ARC: {
                const struct nk_command_arc *a = (const struct nk_command_arc*)cmd;
                Color color = ColorFromNuklear(a->color);
                Vector2 center = {(float)a->cx, (float)a->cy};
                DrawRingLines(center, 0, a->r * scale, a->a[0] * RAD2DEG - 45, a->a[1] * RAD2DEG - 45, RAYLIB_NUKLEAR_DEFAULT_ARC_SEGMENTS, color);
            } break;

            case NK_COMMAND_ARC_FILLED: {
                const struct nk_command_arc_filled *a = (const struct nk_command_arc_filled*)cmd;
                Color color = ColorFromNuklear(a->color);
                Vector2 center = {(float)a->cx * scale, (float)a->cy * scale};
                DrawRing(center, 0, a->r * scale, a->a[0] * RAD2DEG - 45, a->a[1] * RAD2DEG - 45, RAYLIB_NUKLEAR_DEFAULT_ARC_SEGMENTS, color);
            } break;

            case NK_COMMAND_TRIANGLE: {
                const struct nk_command_triangle *t = (const struct nk_command_triangle*)cmd;
                Color color = ColorFromNuklear(t->color);
                Vector2 point1 = {(float)t->b.x * scale, (float)t->b.y * scale};
                Vector2 point2 = {(float)t->a.x * scale, (float)t->a.y * scale};
                Vector2 point3 = {(float)t->c.x * scale, (float)t->c.y * scale};
                DrawTriangleLines(point1, point2, point3, color);
            } break;

            case NK_COMMAND_TRIANGLE_FILLED: {
                const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled*)cmd;
                Color color = ColorFromNuklear(t->color);
                Vector2 point1 = {(float)t->b.x * scale, (float)t->b.y * scale};
                Vector2 point2 = {(float)t->a.x * scale, (float)t->a.y * scale};
                Vector2 point3 = {(float)t->c.x * scale, (float)t->c.y * scale};
                DrawTriangle(point1, point2, point3, color);
            } break;

            case NK_COMMAND_POLYGON: {
                // TODO: Confirm Polygon
                const struct nk_command_polygon *p = (const struct nk_command_polygon*)cmd;
                Color color = ColorFromNuklear(p->color);
                struct Vector2* points = (struct Vector2*)MemAlloc(p->point_count * (unsigned short)sizeof(Vector2));
                unsigned short i;
                for (i = 0; i < p->point_count; i++) {
                    points[i].x = p->points[i].x * scale;
                    points[i].y = p->points[i].y * scale;
                }
                DrawTriangleStrip(points, p->point_count, color);
                MemFree(points);
            } break;

            case NK_COMMAND_POLYGON_FILLED: {
                // TODO: Polygon filled expects counter clockwise order
                const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled*)cmd;
                Color color = ColorFromNuklear(p->color);
                struct Vector2* points = (struct Vector2*)MemAlloc(p->point_count * (unsigned short)sizeof(Vector2));
                unsigned short i;
                for (i = 0; i < p->point_count; i++) {
                    points[i].x = p->points[i].x * scale;
                    points[i].y = p->points[i].y * scale;
                }
                DrawTriangleFan(points, p->point_count, color);
                MemFree(points);
            } break;

            case NK_COMMAND_POLYLINE: {
                // TODO: Polygon expects counter clockwise order
                const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
                Color color = ColorFromNuklear(p->color);
                struct Vector2* points = (struct Vector2*)MemAlloc(p->point_count * (unsigned short)sizeof(Vector2));
                unsigned short i;
                for (i = 0; i < p->point_count; i++) {
                    points[i].x = p->points[i].x * scale;
                    points[i].y = p->points[i].y * scale;
                }
                DrawTriangleStrip(points, p->point_count, color);
                MemFree(points);
            } break;

            case NK_COMMAND_TEXT: {
                const struct nk_command_text *text = (const struct nk_command_text*)cmd;
                Color color = ColorFromNuklear(text->foreground);
                
                float fontSize = text->font->height * scale;
                Font* font = (Font*)text->font->userdata.ptr;
                if (font != NULL) {
                    Vector2 position = {(float)text->x * scale, (float)text->y * scale};
                    DrawTextEx(*font, (const char*)text->string, position, fontSize, fontSize / font->baseSize, color);
                }
                else {
                    DrawText((const char*)text->string, (int)(text->x * scale), (int)(text->y * scale), (int)fontSize, color);
                }
            } break;

            case NK_COMMAND_IMAGE: {
                const struct nk_command_image *i = (const struct nk_command_image *)cmd;
                Texture texture = *(Texture*)i->img.handle.ptr;
                Rectangle source = { (float)i->img.region[0], (float)i->img.region[1], (float)i->img.region[2], (float)i->img.region[3] };
                Rectangle dest = {(float)i->x * scale, (float)i->y * scale, (float)i->w * scale, (float)i->h * scale};
                Color tint = ColorFromNuklear(i->col);
                DrawTexturePro(texture, source, dest, { 0, 0 }, 0, tint);
            } break;

            //case NK_COMMAND_SCAL_SPRITE: {
            //    const struct nk_command_scal_sprite* i = (const struct nk_command_scal_sprite*)cmd;
            //    Texture texture = *(Texture*)i->sprite.handle.ptr;
            //    Rectangle source = { (float)i->sprite.region[0], (float)i->sprite.region[1], (float)i->sprite.region[2], (float)i->sprite.region[3] };
            //    Vector2 origin = { i->sprite.origin.x, i->sprite.origin.y };
            //    Rectangle dest = { ((float)i->x + origin.x) * scale, ((float)i->y + origin.y) * scale, (float)i->w * scale, (float)i->h * scale };
            //    float rotation = i->sprite.rotation;
            //    Color tint = ColorFromNuklear(i->col);
            //    DrawTexturePro(texture, source, dest, origin, rotation, tint);
            //} break;

            case NK_COMMAND_CUSTOM: {
                TraceLog(LOG_WARNING, "NUKLEAR: Unverified custom callback implementation NK_COMMAND_CUSTOM");
                const struct nk_command_custom *custom = (const struct nk_command_custom *)cmd;
                custom->callback(NULL, (short)(custom->x * scale), (short)(custom->y * scale), (unsigned short)(custom->w * scale), (unsigned short)(custom->h * scale), custom->callback_data);
            } break;

            default: {
                TraceLog(LOG_WARNING, "NUKLEAR: Missing implementation %i", cmd->type);
            } break;
        }
    }

    nk_clear(ctx);
}

/**
 * Update the Nuklear context for the keyboard input from raylib.
 *
 * @param ctx The nuklear context.
 *
 * @internal
 */
NK_API void nk_raylib_input_keyboard(struct nk_context * ctx)
{
    bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    nk_input_key(ctx, NK_KEY_SHIFT, shift);
    nk_input_key(ctx, NK_KEY_CTRL, control);
    nk_input_key(ctx, NK_KEY_DEL, IsKeyDown(KEY_DELETE));
    nk_input_key(ctx, NK_KEY_ENTER, IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_KP_ENTER));
    nk_input_key(ctx, NK_KEY_TAB, IsKeyDown(KEY_TAB));
    nk_input_key(ctx, NK_KEY_BACKSPACE, IsKeyDown(KEY_BACKSPACE));
    nk_input_key(ctx, NK_KEY_COPY, IsKeyPressed(KEY_C) && control);
    nk_input_key(ctx, NK_KEY_CUT, IsKeyPressed(KEY_X) && control);
    nk_input_key(ctx, NK_KEY_PASTE, IsKeyPressed(KEY_V) && control);
    nk_input_key(ctx, NK_KEY_TEXT_LINE_START, IsKeyPressed(KEY_B) && control);
    nk_input_key(ctx, NK_KEY_TEXT_LINE_END, IsKeyPressed(KEY_E) && control);
    nk_input_key(ctx, NK_KEY_TEXT_UNDO, IsKeyDown(KEY_Z) && control);
    nk_input_key(ctx, NK_KEY_TEXT_REDO, IsKeyDown(KEY_R) && control);
    nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, IsKeyDown(KEY_A) && control);
    nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, IsKeyDown(KEY_LEFT) && control);
    nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, IsKeyDown(KEY_RIGHT) && control);
    nk_input_key(ctx, NK_KEY_LEFT, IsKeyDown(KEY_LEFT) && !control);
    nk_input_key(ctx, NK_KEY_RIGHT, IsKeyDown(KEY_RIGHT) && !control);
    //nk_input_key(ctx, NK_KEY_TEXT_INSERT_MODE, IsKeyDown());
    //nk_input_key(ctx, NK_KEY_TEXT_REPLACE_MODE, IsKeyDown());
    //nk_input_key(ctx, NK_KEY_TEXT_RESET_MODE, IsKeyDown());
    nk_input_key(ctx, NK_KEY_UP, IsKeyDown(KEY_UP));
    nk_input_key(ctx, NK_KEY_DOWN, IsKeyDown(KEY_DOWN));
    nk_input_key(ctx, NK_KEY_TEXT_START, IsKeyDown(KEY_HOME));
    nk_input_key(ctx, NK_KEY_TEXT_END, IsKeyDown(KEY_END));
    nk_input_key(ctx, NK_KEY_SCROLL_START, IsKeyDown(KEY_HOME) && control);
    nk_input_key(ctx, NK_KEY_SCROLL_END, IsKeyDown(KEY_END) && control);
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, IsKeyDown(KEY_PAGE_DOWN));
    nk_input_key(ctx, NK_KEY_SCROLL_UP, IsKeyDown(KEY_PAGE_UP));

    // Keys
    if (IsKeyPressed(KEY_APOSTROPHE)) nk_input_unicode(ctx, shift ? 34 : (nk_rune)KEY_APOSTROPHE);
    if (IsKeyPressed(KEY_COMMA)) nk_input_unicode(ctx, shift ? 60 : (nk_rune)KEY_COMMA);
    if (IsKeyPressed(KEY_MINUS)) nk_input_unicode(ctx, shift ? 95 : (nk_rune)KEY_MINUS);
    if (IsKeyPressed(KEY_PERIOD)) nk_input_unicode(ctx, shift ? 62 : (nk_rune)KEY_PERIOD);
    if (IsKeyPressed(KEY_SLASH)) nk_input_unicode(ctx, shift ? 63 : (nk_rune)KEY_SLASH);
    if (IsKeyPressed(KEY_ZERO)) nk_input_unicode(ctx, shift ? 41 : (nk_rune)KEY_ZERO);
    if (IsKeyPressed(KEY_ONE)) nk_input_unicode(ctx, shift ? 33 : (nk_rune)KEY_ONE);
    if (IsKeyPressed(KEY_TWO)) nk_input_unicode(ctx, shift ? 64 : (nk_rune)KEY_TWO);
    if (IsKeyPressed(KEY_THREE)) nk_input_unicode(ctx, shift ? 35 : (nk_rune)KEY_THREE);
    if (IsKeyPressed(KEY_FOUR)) nk_input_unicode(ctx, shift ? 36 : (nk_rune)KEY_FOUR);
    if (IsKeyPressed(KEY_FIVE)) nk_input_unicode(ctx, shift ? 37 : (nk_rune)KEY_FIVE);
    if (IsKeyPressed(KEY_SIX)) nk_input_unicode(ctx, shift ? 94 : (nk_rune)KEY_SIX);
    if (IsKeyPressed(KEY_SEVEN)) nk_input_unicode(ctx, shift ? 38 : (nk_rune)KEY_SEVEN);
    if (IsKeyPressed(KEY_EIGHT)) nk_input_unicode(ctx, shift ? 42 : (nk_rune)KEY_EIGHT);
    if (IsKeyPressed(KEY_NINE)) nk_input_unicode(ctx, shift ? 40 : (nk_rune)KEY_NINE);
    if (IsKeyPressed(KEY_SEMICOLON)) nk_input_unicode(ctx, shift ? 41 : (nk_rune)KEY_SEMICOLON);
    if (IsKeyPressed(KEY_EQUAL)) nk_input_unicode(ctx, shift ? 43 : (nk_rune)KEY_EQUAL);
    if (IsKeyPressed(KEY_A)) nk_input_unicode(ctx, shift ? KEY_A : KEY_A + 32);
    if (IsKeyPressed(KEY_B)) nk_input_unicode(ctx, shift ? KEY_B : KEY_B + 32);
    if (IsKeyPressed(KEY_C)) nk_input_unicode(ctx, shift ? KEY_C : KEY_C + 32);
    if (IsKeyPressed(KEY_D)) nk_input_unicode(ctx, shift ? KEY_D : KEY_D + 32);
    if (IsKeyPressed(KEY_E)) nk_input_unicode(ctx, shift ? KEY_E : KEY_E + 32);
    if (IsKeyPressed(KEY_F)) nk_input_unicode(ctx, shift ? KEY_F : KEY_F + 32);
    if (IsKeyPressed(KEY_G)) nk_input_unicode(ctx, shift ? KEY_G : KEY_G + 32);
    if (IsKeyPressed(KEY_H)) nk_input_unicode(ctx, shift ? KEY_H : KEY_H + 32);
    if (IsKeyPressed(KEY_I)) nk_input_unicode(ctx, shift ? KEY_I : KEY_I + 32);
    if (IsKeyPressed(KEY_J)) nk_input_unicode(ctx, shift ? KEY_J : KEY_J + 32);
    if (IsKeyPressed(KEY_K)) nk_input_unicode(ctx, shift ? KEY_K : KEY_K + 32);
    if (IsKeyPressed(KEY_L)) nk_input_unicode(ctx, shift ? KEY_L : KEY_L + 32);
    if (IsKeyPressed(KEY_M)) nk_input_unicode(ctx, shift ? KEY_M : KEY_M + 32);
    if (IsKeyPressed(KEY_N)) nk_input_unicode(ctx, shift ? KEY_N : KEY_N + 32);
    if (IsKeyPressed(KEY_O)) nk_input_unicode(ctx, shift ? KEY_O : KEY_O + 32);
    if (IsKeyPressed(KEY_P)) nk_input_unicode(ctx, shift ? KEY_P : KEY_P + 32);
    if (IsKeyPressed(KEY_Q)) nk_input_unicode(ctx, shift ? KEY_Q : KEY_Q + 32);
    if (IsKeyPressed(KEY_R)) nk_input_unicode(ctx, shift ? KEY_R : KEY_R + 32);
    if (IsKeyPressed(KEY_S)) nk_input_unicode(ctx, shift ? KEY_S : KEY_S + 32);
    if (IsKeyPressed(KEY_T)) nk_input_unicode(ctx, shift ? KEY_T : KEY_T + 32);
    if (IsKeyPressed(KEY_U)) nk_input_unicode(ctx, shift ? KEY_U : KEY_U + 32);
    if (IsKeyPressed(KEY_V)) nk_input_unicode(ctx, shift ? KEY_V : KEY_V + 32);
    if (IsKeyPressed(KEY_W)) nk_input_unicode(ctx, shift ? KEY_W : KEY_W + 32);
    if (IsKeyPressed(KEY_X)) nk_input_unicode(ctx, shift ? KEY_X : KEY_X + 32);
    if (IsKeyPressed(KEY_Y)) nk_input_unicode(ctx, shift ? KEY_Y : KEY_Y + 32);
    if (IsKeyPressed(KEY_Z)) nk_input_unicode(ctx, shift ? KEY_Z : KEY_Z + 32);
    if (IsKeyPressed(KEY_LEFT_BRACKET)) nk_input_unicode(ctx, shift ? 123 : (nk_rune)KEY_LEFT_BRACKET);
    if (IsKeyPressed(KEY_BACKSLASH)) nk_input_unicode(ctx, shift ? 124 : (nk_rune)KEY_BACKSLASH);
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) nk_input_unicode(ctx, shift ? 125 : (nk_rune)KEY_RIGHT_BRACKET);
    if (IsKeyPressed(KEY_GRAVE)) nk_input_unicode(ctx, shift ? 126 : (nk_rune)KEY_GRAVE);

    // Functions
    if (IsKeyPressed(KEY_SPACE)) nk_input_unicode(ctx, KEY_SPACE);
    //if (IsKeyPressed(KEY_TAB)) nk_input_unicode(ctx, 9);

    // Keypad
    if (IsKeyPressed(KEY_KP_0)) nk_input_unicode(ctx, KEY_ZERO);
    if (IsKeyPressed(KEY_KP_1)) nk_input_unicode(ctx, KEY_ONE);
    if (IsKeyPressed(KEY_KP_2)) nk_input_unicode(ctx, KEY_TWO);
    if (IsKeyPressed(KEY_KP_3)) nk_input_unicode(ctx, KEY_THREE);
    if (IsKeyPressed(KEY_KP_4)) nk_input_unicode(ctx, KEY_FOUR);
    if (IsKeyPressed(KEY_KP_5)) nk_input_unicode(ctx, KEY_FIVE);
    if (IsKeyPressed(KEY_KP_6)) nk_input_unicode(ctx, KEY_SIX);
    if (IsKeyPressed(KEY_KP_7)) nk_input_unicode(ctx, KEY_SEVEN);
    if (IsKeyPressed(KEY_KP_8)) nk_input_unicode(ctx, KEY_EIGHT);
    if (IsKeyPressed(KEY_KP_9)) nk_input_unicode(ctx, KEY_NINE);
    if (IsKeyPressed(KEY_KP_DECIMAL)) nk_input_unicode(ctx, KEY_PERIOD);
    if (IsKeyPressed(KEY_KP_DIVIDE)) nk_input_unicode(ctx, KEY_SLASH);
    if (IsKeyPressed(KEY_KP_MULTIPLY)) nk_input_unicode(ctx, 48);
    if (IsKeyPressed(KEY_KP_SUBTRACT)) nk_input_unicode(ctx, 45);
    if (IsKeyPressed(KEY_KP_ADD)) nk_input_unicode(ctx, 43);
}

/**
 * Update the Nuklear context for the mouse input from raylib.
 *
 * @param ctx The nuklear context.
 *
 * @internal
 */
NK_API void nk_raylib_input_mouse(struct nk_context * ctx)
{
    const float scale = GetNuklearScaling(ctx);
    const int mouseX = (int)((float)GetMouseX() / scale);
    const int mouseY = (int)((float)GetMouseY() / scale);

    nk_input_motion(ctx, mouseX, mouseY);
    nk_input_button(ctx, NK_BUTTON_LEFT, mouseX, mouseY, IsMouseButtonDown(MOUSE_LEFT_BUTTON));
    nk_input_button(ctx, NK_BUTTON_RIGHT, mouseX, mouseY, IsMouseButtonDown(MOUSE_RIGHT_BUTTON));
    nk_input_button(ctx, NK_BUTTON_MIDDLE, mouseX, mouseY, IsMouseButtonDown(MOUSE_MIDDLE_BUTTON));

    // Mouse Wheel
    float mouseWheel = GetMouseWheelMove();
    if (mouseWheel != 0.0f) {
        struct nk_vec2 mouseWheelMove;
        mouseWheelMove.x = 0.0f;
        mouseWheelMove.y = mouseWheel;
        nk_input_scroll(ctx, mouseWheelMove);
    }
}

/**
 * Update the Nuklear context for raylib's state.
 *
 * @param ctx The nuklear context to act upon.
 */
NK_API void
UpdateNuklear(struct nk_context * ctx)
{
    // Update the time that has changed since last frame.
    ctx->delta_time_seconds = GetFrameTime();

    // Update the input state.
    nk_input_begin(ctx);
    {
        nk_raylib_input_mouse(ctx);
        nk_raylib_input_keyboard(ctx);
    }
    nk_input_end(ctx);
}

/**
 * Convert the given Nuklear rectangle to a raylib Rectangle.
 */
NK_API struct
Rectangle RectangleFromNuklear(struct nk_context* ctx, struct nk_rect rect)
{
    float scaling = GetNuklearScaling(ctx);
    Rectangle output;
    output.x = rect.x * scaling;
    output.y = rect.y * scaling;
    output.width = rect.w * scaling;
    output.height = rect.h * scaling;
    return output;
}

/**
 * Convert the given raylib Rectangle to a Nuklear rectangle.
 */
NK_API struct
nk_rect RectangleToNuklear(struct nk_context* ctx, Rectangle rect)
{
    float scaling = GetNuklearScaling(ctx);
    return nk_rect(rect.x / scaling, rect.y / scaling, rect.width / scaling, rect.height / scaling);
}

/**
 * Convert the given raylib texture to a Nuklear image
 */
NK_API struct nk_image TextureToNuklear(Texture tex)
{
	// Declare the img to store data and allocate memory
	// For the texture
	struct nk_image img;
	struct Texture* stored_tex = (struct Texture*)MemAlloc(sizeof(Texture));

	// Copy the data from the texture given into the new texture
	stored_tex->id = tex.id;
	stored_tex->width = tex.width;
	stored_tex->height = tex.height;
	stored_tex->mipmaps = tex.mipmaps;
	stored_tex->format = tex.format;

	// Initialize the nk_image struct
	img.handle.ptr = stored_tex;
	img.w = (nk_ushort)stored_tex->width;
	img.h = (nk_ushort)stored_tex->height;

	return img;
}

/**
 * Convert the given Nuklear image to a raylib Texture
 */
NK_API struct Texture TextureFromNuklear(struct nk_image img)
{
	// Declare texture for storage
	// And get back the stored texture
	Texture tex;
	Texture* stored_tex = (Texture*)img.handle.ptr;

	// Copy the data from the stored texture to the texture
	tex.id = stored_tex->id;
	tex.width = stored_tex->width;
	tex.height = stored_tex->height;
	tex.mipmaps = stored_tex->mipmaps;
	tex.format = stored_tex->format;

	return tex;
}

/**
 * Load a Nuklear image directly
 *
 * @param path The path to the image
 */
NK_API struct nk_image LoadNuklearImage(const char* path)
{
	return TextureToNuklear(LoadTexture(path));
}

/**
 * Unload a loaded Nuklear image
 *
 * @param img The Nuklear image to unload
 */
NK_API void UnloadNuklearImage(struct nk_image img)
{
	Texture tex = TextureFromNuklear(img);
	UnloadTexture(tex);
	CleanupNuklearImage(img);
}

/**
 * Cleans up memory used by a Nuklear image
 * Does not unload the image.
 *
 * @param img The Nuklear image to cleanup
*/
NK_API void CleanupNuklearImage(struct nk_image img)
{
    MemFree(img.handle.ptr);
}

/**
 * Sets the scaling of the given Nuklear context.
 *
 * @param ctx The nuklear context.
 * @param scaling How much scale to apply to the graphical user interface.
 */
NK_API void SetNuklearScaling(struct nk_context * ctx, float scaling)
{
    if (ctx == NULL) {
        return;
    }

    if (scaling <= 0.0f) {
        TraceLog(LOG_WARNING, "NUKLEAR: Cannot set scaling to be less than 0");
        return;
    }

    struct NuklearUserData* userData = (struct NuklearUserData*)ctx->userdata.ptr;
    if (userData != NULL) {
        userData->scaling = scaling;
    }
}

/**
 * Retrieves the scale value of the given Nuklear context.
 *
 * @return The scale value that had been set for the Nuklear context. 1.0f is the default scale value.
 */
NK_API float GetNuklearScaling(struct nk_context * ctx)
{
    if (ctx == NULL) {
        return 1.0f;
    }
    
    struct NuklearUserData* userData = (struct NuklearUserData*)ctx->userdata.ptr;
    if (userData != NULL) {
        return userData->scaling;
    }

    return 1.0f;
}

#ifdef __cplusplus
}
#endif

#endif  // RAYLIB_NUKLEAR_IMPLEMENTATION_ONCE
#endif  // RAYLIB_NUKLEAR_IMPLEMENTATION
