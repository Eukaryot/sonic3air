#include "oxygen/scripting/JSBindings.h"
#include "oxygen/scripting/JSProgram.h"

#include "oxygen/simulation/bindings/RendererBindings.h"
#include "oxygen/simulation/bindings/LemonScriptBindings.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/resources/FontCollection.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/RuntimeEnvironment.h"

#include <lemon/program/ModuleBindingsBuilder.h>

void JSBindings::Init()
{
    JSBindings::System();
    JSBindings::Renderer();
}


/* SYSTEM */


duk_ret_t System_writeDisplayLine(duk_context* ctx)
{
    const char* text = duk_require_string(ctx, 0);

    if (text)
        LogDisplay::instance().setLogDisplay(text, 2.0f);

    return false;
}

void JSBindings::System()
{
    duk_push_object(JSProgram::ctx);

    // exposed name, native name, arg count
    const duk_function_list_entry list[] = {
        { "writeDisplayLine", System_writeDisplayLine, 1 },

        { NULL, NULL, 0 }
    };

    duk_put_function_list(JSProgram::ctx, -1, list);
    duk_put_global_string(JSProgram::ctx, "System");
}


/* Renderer */


duk_ret_t getScreenWidth(duk_context* ctx)
{
    duk_push_uint(JSProgram::ctx, (uint16)VideoOut::instance().getScreenWidth());
    return 1;
}

duk_ret_t getScreenHeight(duk_context* ctx)
{
    duk_push_uint(JSProgram::ctx, (uint16)VideoOut::instance().getScreenHeight());
    return 1;
}

duk_ret_t getScreenCenterX(duk_context* ctx)
{
    duk_push_uint(JSProgram::ctx, (uint16)VideoOut::instance().getScreenWidth() / 2);
    return 1;
}

duk_ret_t getScreenCenterY(duk_context* ctx)
{
    duk_push_uint(JSProgram::ctx, (uint16)VideoOut::instance().getScreenHeight() / 2);
    return 1;
}

duk_ret_t getScreenExtend(duk_context* ctx)
{
    duk_push_uint(JSProgram::ctx, (uint16)(VideoOut::instance().getScreenWidth() - 320) / 2);
    return 1;
}

duk_ret_t Color_fromHSV(duk_context* ctx)
{
    int arg_count = duk_get_top(ctx);

    float hue = (float)duk_require_number(ctx, 0);
    float saturation = (float)duk_require_number(ctx, 1);
    float value = (float)duk_require_number(ctx, 2);

    Color color;
    color.setFromHSV(Vec3f(hue, saturation, value));
    color.a = (arg_count == 3) ? 1.0f : (float)duk_require_number(ctx, 3);

    duk_push_uint(JSProgram::ctx, color.getRGBA32());
    return 1;
}

duk_ret_t Color_lerp(duk_context* ctx)
{
    uint32 a = duk_require_uint(ctx, 0);
    uint32 b = duk_require_uint(ctx, 1);
    float factor = (float)duk_require_number(ctx, 2);

    duk_push_uint(JSProgram::ctx, Color::interpolateColor(Color::fromRGBA32(a), Color::fromRGBA32(b), factor).getRGBA32());
    return 1;
}

duk_ret_t Renderer_drawSprite1(duk_context* ctx)
{
    uint64 key = duk_require_uint(ctx, 0);
    int16 px = duk_require_int(ctx, 1);
    int16 py = duk_require_int(ctx, 2);
    uint16 atex = duk_require_uint(ctx, 3);
    uint8 flags = duk_require_uint(ctx, 4);
    uint16 renderQueue = duk_require_uint(ctx, 5);

    RenderParts::instance().getSpriteManager().drawCustomSprite(key, Vec2i(px, py), atex, flags, renderQueue);
    return false;
}

void JSBindings::Renderer()
{
    duk_push_global_object(JSProgram::ctx);

    // exposed name, native name, arg count
    const duk_function_list_entry global_list [] = {
        { "getScreenWidth", getScreenWidth, 0 },
        { "getScreenHeight", getScreenHeight, 0 },
        { "getScreenCenterX", getScreenCenterX, 0 },
        { "getScreenCenterY", getScreenCenterY, 0 },
        { "getScreenExtend", getScreenExtend, 0 },

        { NULL, NULL, 0 }
    };

    duk_put_function_list(JSProgram::ctx, -1, global_list);

    /* COLOR */

    duk_push_object(JSProgram::ctx);

    // exposed name, native name, arg count
    const duk_function_list_entry color_list[] = {
        { "fromHSV", Color_fromHSV, DUK_VARARGS },
        { "lerp", Color_lerp, 3 },

        { NULL, NULL, 0 }
    };

    duk_put_function_list(JSProgram::ctx, -1, color_list);
    duk_put_global_string(JSProgram::ctx, "Color");
}