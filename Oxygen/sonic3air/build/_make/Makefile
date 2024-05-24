# This Makefile is primarily used for building the Switch version.

.DEFAULT_GOAL := all

NAME		=  sonic3air
BUILD_STRING	= `grep BUILD_STRING ../../source/sonic3air/version.inc | cut -d'"' -f2`
BUILD_VARIANT	= `grep BUILD_VARIANT ../../source/sonic3air/version.inc | cut -d'"' -f2`
SUFFIX		=  
PKGCONFIG	=  pkg-config
DEBUG		?= 0
STATIC		?= 1
VERBOSE		?= 0
PROFILE		?= 0
STRIP		?= strip
BASEDIR		?= ../../../..

CFLAGS		?= -std=c++17 -Wno-psabi

ENDUSER		?= 1

ifeq ($(ENDUSER),1)
	DEFINES += -DENDUSER
endif

# =============================================================================
# Detect default platform if not explicitly specified
# (Mostly unused, most platforms use CMake)
# =============================================================================

ifdef EMSCRIPTEN
	PLATFORM = Emscripten
endif

ifndef PLATFORM
	ifeq ($(OS),Windows_NT)
		PLATFORM = Windows
		ARCH ?= $(shell gcc -dumpmachine)
	else
		UNAME_S := $(shell uname -s)

		ifeq ($(UNAME_S),Linux)
			PLATFORM = Linux
			ARCH ?= $(shell gcc -dumpmachine)
		endif

		ifeq ($(UNAME_S),Darwin)
			PLATFORM = macOS
			ARCH ?= $(shell gcc -dumpmachine)
		endif
	endif
endif

PLATFORM ?= Unknown

# =============================================================================

OUTDIR = bin/$(PLATFORM)
OBJDIR = obj/$(PLATFORM)

ifdef ARCH
	OUTDIR := $(OUTDIR)/$(ARCH)
	OBJDIR := $(OBJDIR)/$(ARCH)
endif

include Makefile_cfgs/Platforms/$(PLATFORM).cfg

# =============================================================================

ifeq ($(STATIC),1)
	PKGCONFIG +=  --static
endif

ifeq ($(DEBUG),1)
	CFLAGS += -g2
	STRIP = :
	DEFINES += -DDEBUG
else
	CFLAGS += -g0 -O3
endif

ifeq ($(PROFILE),1)
	CFLAGS += -pg -g -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls -fno-default-inline
endif

ifeq ($(VERBOSE),0)
	CC := @$(CC)
	CXX := @$(CXX)
endif

ifeq ($(VERBOSE),2)
	CFLAGS += -v
endif

# =============================================================================

CFLAGS += `$(PKGCONFIG) --cflags sdl2 ogg vorbis theora vorbisfile theoradec zlib minizip`
LIBS   += `$(PKGCONFIG) --libs-only-l --libs-only-L sdl2 ogg vorbis theora vorbisfile theoradec zlib minizip`

ifeq ($(STATIC),1)
	CFLAGS += -static
endif

INCLUDES  += \
				-I$(BASEDIR)/Oxygen/lemonscript/source \
				-I$(BASEDIR)/Oxygen/oxygenengine/source \
				-I$(BASEDIR)/Oxygen/sonic3air/source \
				-I$(BASEDIR)/librmx/source

INCLUDES += $(LIBS)

# Main Sources
SOURCES	+=	\
			Oxygen/lemonscript/source/lemon/pch \
			Oxygen/lemonscript/source/lemon/compiler/Compiler \
			Oxygen/lemonscript/source/lemon/compiler/LineNumberTranslation \
			Oxygen/lemonscript/source/lemon/compiler/Node \
			Oxygen/lemonscript/source/lemon/compiler/Operators \
			Oxygen/lemonscript/source/lemon/compiler/Preprocessor \
			Oxygen/lemonscript/source/lemon/compiler/Token \
			Oxygen/lemonscript/source/lemon/compiler/TokenHelper \
			Oxygen/lemonscript/source/lemon/compiler/TypeCasting \
			Oxygen/lemonscript/source/lemon/compiler/backend/FunctionCompiler \
			Oxygen/lemonscript/source/lemon/compiler/frontend/CompilerFrontend \
			Oxygen/lemonscript/source/lemon/compiler/frontend/TokenProcessing \
			Oxygen/lemonscript/source/lemon/compiler/parser/Parser \
			Oxygen/lemonscript/source/lemon/compiler/parser/ParserHelper \
			Oxygen/lemonscript/source/lemon/program/BaseType \
			Oxygen/lemonscript/source/lemon/program/ConstantArray \
			Oxygen/lemonscript/source/lemon/program/DataType \
			Oxygen/lemonscript/source/lemon/program/Define \
			Oxygen/lemonscript/source/lemon/program/Function \
			Oxygen/lemonscript/source/lemon/program/FunctionWrapper \
			Oxygen/lemonscript/source/lemon/program/GlobalsLookup \
			Oxygen/lemonscript/source/lemon/program/Module \
			Oxygen/lemonscript/source/lemon/program/ModuleSerializer \
			Oxygen/lemonscript/source/lemon/program/Opcode \
			Oxygen/lemonscript/source/lemon/program/Program \
			Oxygen/lemonscript/source/lemon/program/StringRef \
			Oxygen/lemonscript/source/lemon/runtime/BuiltInFunctions \
			Oxygen/lemonscript/source/lemon/runtime/ControlFlow \
			Oxygen/lemonscript/source/lemon/runtime/OpcodeProcessor \
			Oxygen/lemonscript/source/lemon/runtime/Runtime \
			Oxygen/lemonscript/source/lemon/runtime/RuntimeFunction \
			Oxygen/lemonscript/source/lemon/runtime/StandardLibrary \
			Oxygen/lemonscript/source/lemon/runtime/provider/DefaultOpcodeProvider \
			Oxygen/lemonscript/source/lemon/runtime/provider/NativizedOpcodeProvider \
			Oxygen/lemonscript/source/lemon/runtime/provider/OptimizedOpcodeProvider \
			Oxygen/lemonscript/source/lemon/translator/Nativizer \
			Oxygen/lemonscript/source/lemon/translator/NativizerInternal \
			Oxygen/lemonscript/source/lemon/translator/SourceCodeWriter \
			Oxygen/lemonscript/source/lemon/translator/Translator \
			Oxygen/lemonscript/source/lemon/utility/FlyweightString \
			Oxygen/lemonscript/source/lemon/utility/PragmaSplitter \
			Oxygen/oxygenengine/source/oxygen/pch \
			Oxygen/oxygenengine/source/oxygen/application/Application \
			Oxygen/oxygenengine/source/oxygen/application/Configuration \
			Oxygen/oxygenengine/source/oxygen/application/EngineMain \
			Oxygen/oxygenengine/source/oxygen/application/GameLoader \
			Oxygen/oxygenengine/source/oxygen/application/GameProfile \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioCollection \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioOutBase \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioPlayer \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioSourceBase \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioSourceManager \
			Oxygen/oxygenengine/source/oxygen/application/audio/EmulationAudioSource \
			Oxygen/oxygenengine/source/oxygen/application/audio/OggAudioSource \
			Oxygen/oxygenengine/source/oxygen/application/input/ControlsIn \
			Oxygen/oxygenengine/source/oxygen/application/input/InputConfig \
			Oxygen/oxygenengine/source/oxygen/application/input/InputManager \
			Oxygen/oxygenengine/source/oxygen/application/input/InputRecorder \
			Oxygen/oxygenengine/source/oxygen/application/input/RumbleEffectQueue \
			Oxygen/oxygenengine/source/oxygen/application/mainview/GameView \
			Oxygen/oxygenengine/source/oxygen/application/menu/GameSetupScreen \
			Oxygen/oxygenengine/source/oxygen/application/menu/MenuItems \
			Oxygen/oxygenengine/source/oxygen/application/menu/OxygenMenu \
			Oxygen/oxygenengine/source/oxygen/application/modding/Mod \
			Oxygen/oxygenengine/source/oxygen/application/modding/ModManager \
			Oxygen/oxygenengine/source/oxygen/application/overlays/BackdropView \
			Oxygen/oxygenengine/source/oxygen/application/overlays/CheatSheetOverlay \
			Oxygen/oxygenengine/source/oxygen/application/overlays/DebugLogView \
			Oxygen/oxygenengine/source/oxygen/application/overlays/DebugSidePanel \
			Oxygen/oxygenengine/source/oxygen/application/overlays/DebugSidePanelCategory \
			Oxygen/oxygenengine/source/oxygen/application/overlays/MemoryHexView \
			Oxygen/oxygenengine/source/oxygen/application/overlays/ProfilingView \
			Oxygen/oxygenengine/source/oxygen/application/overlays/SaveStateMenu \
			Oxygen/oxygenengine/source/oxygen/application/overlays/TouchControlsOverlay \
			Oxygen/oxygenengine/source/oxygen/application/video/VideoOut \
			Oxygen/oxygenengine/source/oxygen/download/Downloader \
			Oxygen/oxygenengine/source/oxygen/download/DownloadManager \
			Oxygen/oxygenengine/source/oxygen/drawing/DrawCollection \
			Oxygen/oxygenengine/source/oxygen/drawing/DrawCommand \
			Oxygen/oxygenengine/source/oxygen/drawing/Drawer \
			Oxygen/oxygenengine/source/oxygen/drawing/DrawerTexture \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLDrawer \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLDrawerResources \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLDrawerTexture \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLSpriteTextureManager \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLTexture \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/Upscaler \
			Oxygen/oxygenengine/source/oxygen/drawing/software/Blitter \
			Oxygen/oxygenengine/source/oxygen/drawing/software/SoftwareDrawer \
			Oxygen/oxygenengine/source/oxygen/drawing/software/SoftwareDrawerTexture \
			Oxygen/oxygenengine/source/oxygen/drawing/software/SoftwareRasterizer \
			Oxygen/oxygenengine/source/oxygen/file/FilePackage \
			Oxygen/oxygenengine/source/oxygen/file/FileStructureTree \
			Oxygen/oxygenengine/source/oxygen/file/PackedFileProvider \
			Oxygen/oxygenengine/source/oxygen/file/ZipFileProvider \
			Oxygen/oxygenengine/source/oxygen/helper/BitStream \
			Oxygen/oxygenengine/source/oxygen/helper/FileHelper \
			Oxygen/oxygenengine/source/oxygen/helper/HighResolutionTimer \
			Oxygen/oxygenengine/source/oxygen/helper/JsonHelper \
			Oxygen/oxygenengine/source/oxygen/helper/Logging \
			Oxygen/oxygenengine/source/oxygen/helper/Profiling \
			Oxygen/oxygenengine/source/oxygen/helper/TextInputHandler \
			Oxygen/oxygenengine/source/oxygen/helper/Transform2D \
			Oxygen/oxygenengine/source/oxygen/helper/Utils \
			Oxygen/oxygenengine/source/oxygen/platform/AndroidJavaInterface \
			Oxygen/oxygenengine/source/oxygen/platform/CrashHandler \
			Oxygen/oxygenengine/source/oxygen/platform/PlatformFunctions \
			Oxygen/oxygenengine/source/oxygen/rendering/Geometry \
			Oxygen/oxygenengine/source/oxygen/rendering/RenderResources \
			Oxygen/oxygenengine/source/oxygen/rendering/opengl/OpenGLRenderer \
			Oxygen/oxygenengine/source/oxygen/rendering/opengl/OpenGLRenderResources \
			Oxygen/oxygenengine/source/oxygen/rendering/opengl/shaders/DebugDrawPlaneShader \
			Oxygen/oxygenengine/source/oxygen/rendering/opengl/shaders/OpenGLShader \
			Oxygen/oxygenengine/source/oxygen/rendering/opengl/shaders/RenderComponentSpriteShader \
			Oxygen/oxygenengine/source/oxygen/rendering/opengl/shaders/RenderPaletteSpriteShader \
			Oxygen/oxygenengine/source/oxygen/rendering/opengl/shaders/RenderPlaneShader \
			Oxygen/oxygenengine/source/oxygen/rendering/opengl/shaders/RenderVdpSpriteShader \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/palette/PaletteManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/palette/Palette \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/PatternManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/PlaneManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/RenderItem \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/RenderParts \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/ScrollOffsetsManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/SpriteManager \
			Oxygen/oxygenengine/source/oxygen/rendering/software/SoftwareRenderer \
			Oxygen/oxygenengine/source/oxygen/rendering/sprite/ComponentSprite \
			Oxygen/oxygenengine/source/oxygen/rendering/sprite/PaletteSprite \
			Oxygen/oxygenengine/source/oxygen/rendering/sprite/SpriteDump \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/BufferTexture \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/Kosinski \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/PaletteBitmap \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/RenderUtils \
			Oxygen/oxygenengine/source/oxygen/resources/FontCollection \
			Oxygen/oxygenengine/source/oxygen/resources/PrintedTextCache \
			Oxygen/oxygenengine/source/oxygen/resources/ResourcesCache \
			Oxygen/oxygenengine/source/oxygen/resources/SpriteCache \
			Oxygen/oxygenengine/source/oxygen/simulation/CodeExec \
			Oxygen/oxygenengine/source/oxygen/simulation/EmulatorInterface \
			Oxygen/oxygenengine/source/oxygen/simulation/GameRecorder \
			Oxygen/oxygenengine/source/oxygen/simulation/LemonScriptProgram \
			Oxygen/oxygenengine/source/oxygen/simulation/LemonScriptRuntime \
			Oxygen/oxygenengine/source/oxygen/simulation/LogDisplay \
			Oxygen/oxygenengine/source/oxygen/simulation/PersistentData \
			Oxygen/oxygenengine/source/oxygen/simulation/SaveStateSerializer \
			Oxygen/oxygenengine/source/oxygen/simulation/Simulation \
			Oxygen/oxygenengine/source/oxygen/simulation/analyse/ROMDataAnalyser \
			Oxygen/oxygenengine/source/oxygen/simulation/bindings/LemonScriptBindings \
			Oxygen/oxygenengine/source/oxygen/simulation/bindings/RendererBindings \
			Oxygen/oxygenengine/source/oxygen/simulation/debug/DebugTracking \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/blip_buf \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/sn76489 \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/SoundDriver \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/SoundEmulation \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/ym2612 \
			Oxygen/oxygenengine/source/oxygen_netcore/pch \
			Oxygen/oxygenengine/source/oxygen_netcore/network/ConnectionManager \
			Oxygen/oxygenengine/source/oxygen_netcore/network/NetConnection \
			Oxygen/oxygenengine/source/oxygen_netcore/network/RequestBase \
			Oxygen/oxygenengine/source/oxygen_netcore/network/ServerClientBase \
			Oxygen/oxygenengine/source/oxygen_netcore/network/Sockets \
			Oxygen/oxygenengine/source/oxygen_netcore/network/internal/CryptoFunctions \
			Oxygen/oxygenengine/source/oxygen_netcore/network/internal/ReceivedPacketCache \
			Oxygen/oxygenengine/source/oxygen_netcore/network/internal/SentPacketCache \
			Oxygen/oxygenengine/source/oxygen_netcore/network/internal/WebSocketClient \
			Oxygen/oxygenengine/source/oxygen_netcore/network/internal/WebSocketWrapper \
			Oxygen/sonic3air/source/sonic3air/ConfigurationImpl \
			Oxygen/sonic3air/source/sonic3air/EngineDelegate \
			Oxygen/sonic3air/source/sonic3air/Game \
			Oxygen/sonic3air/source/sonic3air/main \
			Oxygen/sonic3air/source/sonic3air/pch \
			Oxygen/sonic3air/source/sonic3air/_nativized/NativizedCode \
			Oxygen/sonic3air/source/sonic3air/audio/AudioOut \
			Oxygen/sonic3air/source/sonic3air/audio/CustomAudioMixer \
			Oxygen/sonic3air/source/sonic3air/audio/RemasteredMusicDownload \
			Oxygen/sonic3air/source/sonic3air/client/GameClient \
			Oxygen/sonic3air/source/sonic3air/client/GhostSync \
			Oxygen/sonic3air/source/sonic3air/client/UpdateCheck \
			Oxygen/sonic3air/source/sonic3air/client/crowdcontrol/CrowdControlClient \
			Oxygen/sonic3air/source/sonic3air/data/PlayerProgress \
			Oxygen/sonic3air/source/sonic3air/data/PlayerRecorder \
			Oxygen/sonic3air/source/sonic3air/data/SharedDatabase \
			Oxygen/sonic3air/source/sonic3air/data/TimeAttackData \
			Oxygen/sonic3air/source/sonic3air/generator/ResourceScriptGenerator \
			Oxygen/sonic3air/source/sonic3air/helper/BlueSpheresRendering \
			Oxygen/sonic3air/source/sonic3air/helper/CommandForwarder \
			Oxygen/sonic3air/source/sonic3air/helper/DiscordIntegration \
			Oxygen/sonic3air/source/sonic3air/helper/DrawingUtils \
			Oxygen/sonic3air/source/sonic3air/helper/GameUtils \
			Oxygen/sonic3air/source/sonic3air/helper/PackageBuilder \
			Oxygen/sonic3air/source/sonic3air/menu/ActSelectMenu \
			Oxygen/sonic3air/source/sonic3air/menu/ExtrasMenu \
			Oxygen/sonic3air/source/sonic3air/menu/GameApp \
			Oxygen/sonic3air/source/sonic3air/menu/GameMenuBase \
			Oxygen/sonic3air/source/sonic3air/menu/GameMenuManager \
			Oxygen/sonic3air/source/sonic3air/menu/MainMenu \
			Oxygen/sonic3air/source/sonic3air/menu/MenuBackground \
			Oxygen/sonic3air/source/sonic3air/menu/PauseMenu \
			Oxygen/sonic3air/source/sonic3air/menu/SharedResources \
			Oxygen/sonic3air/source/sonic3air/menu/TimeAttackMenu \
			Oxygen/sonic3air/source/sonic3air/menu/TimeAttackResultsMenu \
			Oxygen/sonic3air/source/sonic3air/menu/context/ApplicationContextMenu \
			Oxygen/sonic3air/source/sonic3air/menu/entries/GeneralMenuEntries \
			Oxygen/sonic3air/source/sonic3air/menu/helper/GameMenuControlsDisplay \
			Oxygen/sonic3air/source/sonic3air/menu/helper/GameMenuScrolling \
			Oxygen/sonic3air/source/sonic3air/menu/mods/ModsMenu \
			Oxygen/sonic3air/source/sonic3air/menu/mods/ModsMenuEntries \
			Oxygen/sonic3air/source/sonic3air/menu/options/ControllerSetupMenu \
			Oxygen/sonic3air/source/sonic3air/menu/options/OptionsEntry \
			Oxygen/sonic3air/source/sonic3air/menu/options/OptionsMenu \
			Oxygen/sonic3air/source/sonic3air/menu/options/OptionsMenuEntries \
			Oxygen/sonic3air/source/sonic3air/menu/options/OptionsConfig \
			Oxygen/sonic3air/source/sonic3air/menu/overlays/SecretUnlockedWindow \
			Oxygen/sonic3air/source/sonic3air/menu/overlays/SkippableCutsceneWindow \
			Oxygen/sonic3air/source/sonic3air/resources/DynamicSprites \
			Oxygen/sonic3air/source/sonic3air/scriptimpl/ScriptImplementations \
			librmx/source/rmxbase/_jsoncpp/json_reader \
			librmx/source/rmxbase/_jsoncpp/json_value \
			librmx/source/rmxbase/_jsoncpp/json_writer \
			librmx/source/rmxbase/rmxbase \
			librmx/source/rmxbase/base/Basics \
			librmx/source/rmxbase/base/ErrorHandler \
			librmx/source/rmxbase/bitmap/Bitmap \
			librmx/source/rmxbase/bitmap/BitmapCodecBMP \
			librmx/source/rmxbase/bitmap/BitmapCodecICO \
			librmx/source/rmxbase/bitmap/BitmapCodecJPG \
			librmx/source/rmxbase/bitmap/BitmapCodecPNG \
			librmx/source/rmxbase/bitmap/Color \
			librmx/source/rmxbase/file/FileCrawler \
			librmx/source/rmxbase/file/FileHandle \
			librmx/source/rmxbase/file/FileIO \
			librmx/source/rmxbase/file/FileProvider \
			librmx/source/rmxbase/file/FileSystem \
			librmx/source/rmxbase/file/JsonHelper \
			librmx/source/rmxbase/file/RealFileProvider \
			librmx/source/rmxbase/math/Math \
			librmx/source/rmxbase/memory/BinarySerializer \
			librmx/source/rmxbase/memory/InputStream \
			librmx/source/rmxbase/memory/OneTimeAllocPool \
			librmx/source/rmxbase/memory/OutputStream \
			librmx/source/rmxbase/memory/RC4Encryption \
			librmx/source/rmxbase/memory/RmxDeflate \
			librmx/source/rmxbase/memory/String \
			librmx/source/rmxbase/memory/UnalignedMemory \
			librmx/source/rmxbase/memory/UTF8Conversion \
			librmx/source/rmxbase/memory/VectorBinarySerializer \
			librmx/source/rmxbase/memory/ZlibDeflate \
			librmx/source/rmxbase/tools/Logging \
			librmx/source/rmxbase/tools/Tools \
			librmx/source/rmxext_oggvorbis/OggLoader \
			librmx/source/rmxext_oggvorbis/rmxext_oggvorbis \
			librmx/source/rmxmedia/rmxmedia \
			librmx/source/rmxmedia/audiovideo/AudioBuffer \
			librmx/source/rmxmedia/audiovideo/AudioManager \
			librmx/source/rmxmedia/audiovideo/AudioMixer \
			librmx/source/rmxmedia/audiovideo/AudioReference \
			librmx/source/rmxmedia/audiovideo/VideoBuffer \
			librmx/source/rmxmedia/file/FileInputStreamSDL \
			librmx/source/rmxmedia/file/FileProviderSDL \
			librmx/source/rmxmedia/font/Font \
			librmx/source/rmxmedia/font/FontProcessor \
			librmx/source/rmxmedia/font/FontSource \
			librmx/source/rmxmedia/framework/AppFramework \
			librmx/source/rmxmedia/framework/FTX_System \
			librmx/source/rmxmedia/framework/GuiBase \
			librmx/source/rmxmedia/opengl/Framebuffer \
			librmx/source/rmxmedia/opengl/GLTools \
			librmx/source/rmxmedia/opengl/OpenGLFontOutput \
			librmx/source/rmxmedia/opengl/Painter \
			librmx/source/rmxmedia/opengl/Shader \
			librmx/source/rmxmedia/opengl/SpriteAtlas \
			librmx/source/rmxmedia/opengl/Texture \
			librmx/source/rmxmedia/opengl/VertexArrayObject \
			librmx/source/rmxmedia/threads/JobManager \
			librmx/source/rmxmedia/threads/Thread \

# =============================================================================

PKGSUFFIX ?= $(SUFFIX)

BINPATH = $(OUTDIR)/$(NAME)$(SUFFIX)
PKGPATH = $(OUTDIR)/$(NAME)$(PKGSUFFIX)

OBJECTS += $(addprefix $(OBJDIR)/, $(addsuffix .o, $(SOURCES)))

$(shell mkdir -p $(OUTDIR))
$(shell mkdir -p $(OBJDIR))

$(OBJDIR)/%.o: $(BASEDIR)/%.c
	@mkdir -p $(@D)
	@echo -n Compiling $<...
	$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@
	@echo " Done!"

$(OBJDIR)/%.o: $(BASEDIR)/%.cpp
	@mkdir -p $(@D)
	@echo -n Compiling $<...
	$(CXX) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@
	@echo " Done!"

$(BINPATH): $(OBJDIR) $(OBJECTS)
	@echo -n Linking...
	$(CXX) $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(LIBPATHS) $(LIBS) -o $@ 
	@echo " Done!"
	$(STRIP) $@

ifeq ($(BINPATH),$(PKGPATH))
all: $(BINPATH)
else
all: $(PKGPATH)
endif

clean:
	rm -rf $(OBJDIR)