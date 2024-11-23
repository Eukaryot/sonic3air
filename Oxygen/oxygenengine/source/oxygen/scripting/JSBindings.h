
#include "oxygen/scripting/duk/duktape.h"

struct JSBindings
{
    static void Init();

    static void Internal();
    static void System();
    static void Renderer();
};