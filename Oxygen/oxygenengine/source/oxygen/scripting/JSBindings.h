
#include "oxygen/scripting/duk/duktape.h"

#include <rmxmedia.h>
#include <rmxext_oggvorbis.h>

#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/drawing/Drawer.h"
#include "oxygen/helper/Logging.h"

struct JSBindings
{
    static void Init();

    static void System();
    static void Renderer();
};