
#include "oxygen/pch.h"

#include <filesystem>


#include "oxygen/application/Application.h"
#include "oxygen/simulation/Simulation.h"

#include "oxygen/scripting/JSBindings.h"
#include "oxygen/scripting/JSProgram.h"

#include "oxygen/simulation/bindings/RendererBindings.h"
#include "oxygen/simulation/bindings/LemonScriptBindings.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/resources/FontCollection.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/RuntimeEnvironment.h"

#include <lemon/program/DataType.h>
#include <lemon/program/ModuleBindingsBuilder.h>
#include <lemon/runtime/Runtime.h>
#include <lemon/utility/FlyweightString.h>

void JSBindings::Init()
{
    JSBindings::Internal();
    JSBindings::System();
    JSBindings::Renderer();
}


/* INTERAL */


duk_ret_t include(duk_context* ctx)
{
    const char* filename = duk_require_string(ctx, 0);

    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "base_path");
    const char* base_path = duk_get_string(ctx, -1);
    duk_pop_2(ctx);

    std::filesystem::path base_path_fs(base_path);
    std::filesystem::path full_path = base_path_fs / ("scripts/" + std::string(filename) + ".js");

    std::wstring full_path_wstr = full_path.wstring();

    String script;
    if (!script.loadFile(full_path_wstr))
    {
        std::string errFMT;
        errFMT.append("Could not open script (")
            .append(full_path.string())
            .append(")");

        return duk_error(ctx, DUK_ERR_ERROR, errFMT.c_str());
    }

    JSProgram::RunScript(script.toStdString().c_str());

    return false;
}

duk_ret_t Lemon_callFunction(duk_context* ctx)
{
    CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
    LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();

    const char* funcName = duk_require_string(ctx, 0);
    int argCount = duk_get_top(ctx) - 1;

    if (argCount > 0)
    {
        CodeExec::FunctionExecData execData;
        for (int i = 1; i <= argCount; ++i)
        {
            lemon::Runtime::FunctionCallParameters::Parameter& param = vectorAdd(execData.mParams.mParams);

            if (duk_is_boolean(ctx, i))
            {
                param.mDataType = &lemon::PredefinedDataTypes::BOOL;
                param.mStorage = duk_require_boolean(ctx, i);
            }
            else if (duk_is_number(ctx, i))
            {
                double arg = duk_get_number(ctx, i);

                if (std::fmod(arg, 1.0) == 0.0)
                {
                    if (arg >= std::numeric_limits<int8>::min() && arg <= std::numeric_limits<int8>::max())
                    {
                        param.mDataType = &lemon::PredefinedDataTypes::INT_8;
                        param.mStorage = static_cast<int8>(arg);
                    }
                    else if (arg >= std::numeric_limits<uint8>::min() && arg <= std::numeric_limits<uint8>::max())
                    {
                        param.mDataType = &lemon::PredefinedDataTypes::UINT_8;
                        param.mStorage = static_cast<uint8>(arg);
                    }
                    else if (arg >= std::numeric_limits<int16>::min() && arg <= std::numeric_limits<int16>::max())
                    {
                        param.mDataType = &lemon::PredefinedDataTypes::INT_16;
                        param.mStorage = static_cast<int16>(arg);
                    }
                    else if (arg >= std::numeric_limits<uint16>::min() && arg <= std::numeric_limits<uint16>::max())
                    {
                        param.mDataType = &lemon::PredefinedDataTypes::UINT_16;
                        param.mStorage = static_cast<uint16>(arg);
                    }
                    else if (arg >= std::numeric_limits<int32>::min() && arg <= std::numeric_limits<int32>::max())
                    {
                        param.mDataType = &lemon::PredefinedDataTypes::INT_32;
                        param.mStorage = static_cast<int32>(arg);
                    }
                    else if (arg >= std::numeric_limits<uint32>::min() && arg <= std::numeric_limits<uint32>::max())
                    {
                        param.mDataType = &lemon::PredefinedDataTypes::UINT_32;
                        param.mStorage = static_cast<uint32>(arg);
                    }
                    else if (arg >= std::numeric_limits<int64>::min() && arg <= std::numeric_limits<int64>::max())
                    {
                        param.mDataType = &lemon::PredefinedDataTypes::INT_64;
                        param.mStorage = static_cast<int64>(arg);
                    }
                    else if (arg >= std::numeric_limits<uint64>::min() && arg <= std::numeric_limits<uint64>::max())
                    {
                        param.mDataType = &lemon::PredefinedDataTypes::UINT_64;
                        param.mStorage = static_cast<uint64>(arg);
                    }
                }
                else
                {
                    param.mDataType = &lemon::PredefinedDataTypes::FLOAT;
                    param.mStorage = *reinterpret_cast<uint64*>(&arg); // Store the float as uint64 for compatibility
                }
            }
            else if (duk_is_string(ctx, i))
            {
                param.mDataType = &lemon::PredefinedDataTypes::STRING;
                param.mStorage = runtime.getInternalLemonRuntime().addString(duk_require_string(ctx, i));
            }
        }
        duk_push_boolean(ctx, codeExec.executeScriptFunction(funcName, false, &execData));
    }
    else
    {
        duk_push_boolean(ctx, codeExec.executeScriptFunction(funcName, false));
    }

    return false;
}

void JSBindings::Internal()
{
    duk_push_global_object(JSProgram::ctx);

    // exposed name, native name, arg count
    const duk_function_list_entry global_list[] = {
        { "include", include, 1 },

        { NULL, NULL, 0 }
    };

    duk_put_function_list(JSProgram::ctx, -1, global_list);

    /* LEMON */

    duk_push_object(JSProgram::ctx);

    // exposed name, native name, arg count
    const duk_function_list_entry lemon_list[] = {
        { "callFunction", Lemon_callFunction, DUK_VARARGS },

        { NULL, NULL, 0 }
    };

    duk_put_function_list(JSProgram::ctx, -1, lemon_list);
    duk_put_global_string(JSProgram::ctx, "Lemon");
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


/* RENDERER */


duk_ret_t getScreenWidth(duk_context* ctx)
{
    duk_push_uint(ctx, (uint16)VideoOut::instance().getScreenWidth());
    return 1;
}

duk_ret_t getScreenHeight(duk_context* ctx)
{
    duk_push_uint(ctx, (uint16)VideoOut::instance().getScreenHeight());
    return 1;
}

duk_ret_t getScreenCenterX(duk_context* ctx)
{
    duk_push_uint(ctx, (uint16)VideoOut::instance().getScreenWidth() / 2);
    return 1;
}

duk_ret_t getScreenCenterY(duk_context* ctx)
{
    duk_push_uint(ctx, (uint16)VideoOut::instance().getScreenHeight() / 2);
    return 1;
}

duk_ret_t getScreenExtend(duk_context* ctx)
{
    duk_push_uint(ctx, (uint16)(VideoOut::instance().getScreenWidth() - 320) / 2);
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

    duk_push_uint(ctx, color.getRGBA32());
    return 1;
}

duk_ret_t Color_lerp(duk_context* ctx)
{
    uint32 a = (uint32)duk_require_uint(ctx, 0);
    uint32 b = (uint32)duk_require_uint(ctx, 1);
    float factor = (float)duk_require_number(ctx, 2);

    duk_push_uint(ctx, Color::interpolateColor(Color::fromRGBA32(a), Color::fromRGBA32(b), factor).getRGBA32());
    return 1;
}

duk_ret_t Renderer_drawSprite1(duk_context* ctx)
{
    lemon::FlyweightString key = duk_require_string(ctx, 0);
    int16 px = (int16)duk_require_int(ctx, 1);
    int16 py = (int16)duk_require_int(ctx, 2);
    uint16 atex = (uint16)duk_require_uint(ctx, 3);
    uint8 flags = (uint8)duk_require_uint(ctx, 4);
    uint16 renderQueue = (uint16)duk_require_uint(ctx, 5);

    RenderParts::instance().getSpriteManager().drawCustomSprite(key.getHash(), Vec2i(px, py), atex, flags, renderQueue);
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

    /* RENDERER */

    duk_push_object(JSProgram::ctx);

    // exposed name, native name, arg count
    const duk_function_list_entry renderer_list[] = {
        { "drawCustomSprite", Renderer_drawSprite1, 6 },

        { NULL, NULL, 0 }
    };

    duk_put_function_list(JSProgram::ctx, -1, renderer_list);
    duk_put_global_string(JSProgram::ctx, "Renderer");
}