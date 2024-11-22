#include "oxygen/scripting/JSBindings.h"
#include "oxygen/scripting/JSProgram.h"

#include "oxygen/simulation/bindings/RendererBindings.h"
#include "oxygen/simulation/bindings/LemonScriptBindings.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/resources/FontCollection.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/RuntimeEnvironment.h"

#include <lemon/program/ModuleBindingsBuilder.h>

void JSBindings::Init()
{
    JSBindings::System();
    JSBindings::Renderer();
}


/* SYSTEM */


duk_ret_t System_RMX_LOG_INFO(duk_context* ctx)
{
    const char* group = duk_require_string(JSProgram::ctx, 0);

    RMX_LOG_INFO(group);
    return false;
}

void JSBindings::System()
{
    duk_push_object(JSProgram::ctx);

    // exposed name, native name, arg count
    const duk_function_list_entry list[] = {
        { "RMX_LOG_INFO", System_RMX_LOG_INFO, 1 },

        { NULL, NULL, 0 }
    };

    duk_put_function_list(JSProgram::ctx, -1, list);
    duk_put_global_string(JSProgram::ctx, "System");
}


/* DISPLAY */


duk_ret_t Renderer_getScreenHeight(duk_context* ctx)
{
    duk_push_uint(JSProgram::ctx, (uint16)VideoOut::instance().getScreenWidth());
    return 1;
}

duk_ret_t Renderer_drawSprite1(duk_context* ctx)
{
    std::string key = duk_require_string(ctx, 0);

    std::hash<std::string> hasher;
    uint64 key_hash = static_cast<uint64_t>(hasher(key));

    int16 px = duk_require_int(ctx, 1);
    int16 py = duk_require_int(ctx, 2);
    uint16 atex = duk_require_uint(ctx, 3);
    uint8 flags = duk_require_uint(ctx, 4);
    uint16 renderQueue = duk_require_uint(ctx, 5);

    RMX_LOG_INFO(key_hash);
    RenderParts::instance().getSpriteManager().drawCustomSprite(key_hash, Vec2i(px, py), atex, flags, renderQueue);
    return false;
}

void JSBindings::Renderer()
{
    duk_push_object(JSProgram::ctx);

    // exposed name, native name, arg count
    const duk_function_list_entry list[] = {
        { "getScreenWidth", Renderer_getScreenHeight, 0 },
        { "drawCustomSprite", Renderer_drawSprite1, 6 },

        { NULL, NULL, 0 }
    };

    duk_put_function_list(JSProgram::ctx, -1, list);
    duk_put_global_string(JSProgram::ctx, "Renderer");
}