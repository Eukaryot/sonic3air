# This Makefile is primarily used for building the Switch version.

.DEFAULT_GOAL := all

NAME		=  sonic3air
BUILD_STRING	= `grep BUILD_STRING ../sonic3air/version.inc | cut -d'"' -f2`
BUILD_VARIANT	= `grep BUILD_VARIANT ../sonic3air/version.inc | cut -d'"' -f2`
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
				-I$(BASEDIR)/Oxygen/soncthrickles/source \
				-I$(BASEDIR)/librmx/source

INCLUDES += $(LIBS)

# Main Sources
SOURCES	+=	\
			Oxygen/lemonscript/source/lemon/compiler/Compiler \
			Oxygen/lemonscript/source/lemon/compiler/FunctionCompiler \
			Oxygen/lemonscript/source/lemon/compiler/Node \
			Oxygen/lemonscript/source/lemon/compiler/Parser \
			Oxygen/lemonscript/source/lemon/compiler/ParserHelper \
			Oxygen/lemonscript/source/lemon/compiler/Preprocessor \
			Oxygen/lemonscript/source/lemon/compiler/Token \
			Oxygen/lemonscript/source/lemon/compiler/TokenProcessing \
			Oxygen/lemonscript/source/lemon/compiler/Utility \
			Oxygen/lemonscript/source/lemon/pch \
			Oxygen/lemonscript/source/lemon/program/DataType \
			Oxygen/lemonscript/source/lemon/program/Function \
			Oxygen/lemonscript/source/lemon/program/FunctionWrapper \
			Oxygen/lemonscript/source/lemon/program/GlobalsLookup \
			Oxygen/lemonscript/source/lemon/program/Module \
			Oxygen/lemonscript/source/lemon/program/Opcode \
			Oxygen/lemonscript/source/lemon/program/Program \
			Oxygen/lemonscript/source/lemon/program/StoredString \
			Oxygen/lemonscript/source/lemon/runtime/provider/DefaultOpcodeProvider \
			Oxygen/lemonscript/source/lemon/runtime/provider/NativizedOpcodeProvider \
			Oxygen/lemonscript/source/lemon/runtime/provider/OptimizedOpcodeProvider \
			Oxygen/lemonscript/source/lemon/runtime/ControlFlow \
			Oxygen/lemonscript/source/lemon/runtime/OpcodeProcessor \
			Oxygen/lemonscript/source/lemon/runtime/Runtime \
			Oxygen/lemonscript/source/lemon/runtime/RuntimeFunction \
			Oxygen/lemonscript/source/lemon/runtime/StandardLibrary \
			Oxygen/lemonscript/source/lemon/translator/Nativizer \
			Oxygen/lemonscript/source/lemon/translator/SourceCodeWriter \
			Oxygen/lemonscript/source/lemon/translator/Translator \
			Oxygen/oxygenengine/source/oxygen/application/Application \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioCollection \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioOutBase \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioPlayer \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioSourceBase \
			Oxygen/oxygenengine/source/oxygen/application/audio/AudioSourceManager \
			Oxygen/oxygenengine/source/oxygen/application/audio/EmulationAudioSource \
			Oxygen/oxygenengine/source/oxygen/application/audio/OggAudioSource \
			Oxygen/oxygenengine/source/oxygen/application/Configuration \
			Oxygen/oxygenengine/source/oxygen/application/EngineMain \
			Oxygen/oxygenengine/source/oxygen/application/GameLoader \
			Oxygen/oxygenengine/source/oxygen/application/GameProfile \
			Oxygen/oxygenengine/source/oxygen/application/input/ControlsIn \
			Oxygen/oxygenengine/source/oxygen/application/input/InputManager \
			Oxygen/oxygenengine/source/oxygen/application/input/InputRecorder \
			Oxygen/oxygenengine/source/oxygen/application/mainview/GameView \
			Oxygen/oxygenengine/source/oxygen/application/modding/Mod \
			Oxygen/oxygenengine/source/oxygen/application/modding/ModManager \
			Oxygen/oxygenengine/source/oxygen/application/overlays/DebugLogView \
			Oxygen/oxygenengine/source/oxygen/application/overlays/DebugSidePanel \
			Oxygen/oxygenengine/source/oxygen/application/overlays/DebugSidePanelCategory \
			Oxygen/oxygenengine/source/oxygen/application/overlays/MemoryHexView \
			Oxygen/oxygenengine/source/oxygen/application/overlays/ProfilingView \
			Oxygen/oxygenengine/source/oxygen/application/overlays/SaveStateMenu \
			Oxygen/oxygenengine/source/oxygen/application/overlays/TouchControlsOverlay \
			Oxygen/oxygenengine/source/oxygen/application/video/VideoOut \
			Oxygen/oxygenengine/source/oxygen/base/CrashHandler \
			Oxygen/oxygenengine/source/oxygen/base/PlatformFunctions \
			Oxygen/oxygenengine/source/oxygen/drawing/DrawCollection \
			Oxygen/oxygenengine/source/oxygen/drawing/DrawCommand \
			Oxygen/oxygenengine/source/oxygen/drawing/Drawer \
			Oxygen/oxygenengine/source/oxygen/drawing/DrawerTexture \
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
			Oxygen/oxygenengine/source/oxygen/helper/Log \
			Oxygen/oxygenengine/source/oxygen/helper/PackageFileCrawler \
			Oxygen/oxygenengine/source/oxygen/helper/Profiling \
			Oxygen/oxygenengine/source/oxygen/helper/Transform2D \
			Oxygen/oxygenengine/source/oxygen/helper/Utils \
			Oxygen/oxygenengine/source/oxygen/pch \
			Oxygen/oxygenengine/source/oxygen/platform/AndroidJavaInterface \
			Oxygen/oxygenengine/source/oxygen/rendering/Geometry \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/OverlayManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/PaletteManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/PatternManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/PlaneManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/RenderParts \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/ScrollOffsetsManager \
			Oxygen/oxygenengine/source/oxygen/rendering/parts/SpriteManager \
			Oxygen/oxygenengine/source/oxygen/rendering/RenderResources \
			Oxygen/oxygenengine/source/oxygen/rendering/software/SoftwareRenderer \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/BufferTexture \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/ComponentSprite \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/Kosinski \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/PaletteBitmap \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/PaletteSprite \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/RenderUtils \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/SpriteBase \
			Oxygen/oxygenengine/source/oxygen/rendering/utils/SpriteDump \
			Oxygen/oxygenengine/source/oxygen/resources/ResourcesCache \
			Oxygen/oxygenengine/source/oxygen/resources/SpriteCache \
			Oxygen/oxygenengine/source/oxygen/simulation/analyse/ROMDataAnalyser \
			Oxygen/oxygenengine/source/oxygen/simulation/CodeExec \
			Oxygen/oxygenengine/source/oxygen/simulation/EmulatorInterface \
			Oxygen/oxygenengine/source/oxygen/simulation/GameRecorder \
			Oxygen/oxygenengine/source/oxygen/simulation/LemonScriptBindings \
			Oxygen/oxygenengine/source/oxygen/simulation/LemonScriptRuntime \
			Oxygen/oxygenengine/source/oxygen/simulation/LogDisplay \
			Oxygen/oxygenengine/source/oxygen/simulation/PersistentData \
			Oxygen/oxygenengine/source/oxygen/simulation/SaveStateSerializer \
			Oxygen/oxygenengine/source/oxygen/simulation/Simulation \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/blip_buf \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/sn76489 \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/SoundDriver \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/SoundEmulation \
			Oxygen/oxygenengine/source/oxygen/simulation/sound/ym2612 \
			Oxygen/soncthrickles/source/sonic3air/audio/AudioOut \
			Oxygen/soncthrickles/source/sonic3air/audio/CustomAudioMixer \
			Oxygen/soncthrickles/source/sonic3air/ConfigurationImpl \
			Oxygen/soncthrickles/source/sonic3air/data/PlayerProgress \
			Oxygen/soncthrickles/source/sonic3air/data/PlayerRecorder \
			Oxygen/soncthrickles/source/sonic3air/data/SharedDatabase \
			Oxygen/soncthrickles/source/sonic3air/data/TimeAttackData \
			Oxygen/soncthrickles/source/sonic3air/debug/DebugSidePanelAdditions \
			Oxygen/soncthrickles/source/sonic3air/EngineDelegate \
			Oxygen/soncthrickles/source/sonic3air/Game \
			Oxygen/soncthrickles/source/sonic3air/generator/ResourceScriptGenerator \
			Oxygen/soncthrickles/source/sonic3air/helper/BlueSpheresRendering \
			Oxygen/soncthrickles/source/sonic3air/helper/GameUtils \
			Oxygen/soncthrickles/source/sonic3air/helper/DiscordIntegration \
			Oxygen/soncthrickles/source/sonic3air/main \
			Oxygen/soncthrickles/source/sonic3air/menu/ActSelectMenu \
			Oxygen/soncthrickles/source/sonic3air/menu/context/ApplicationContextMenu \
			Oxygen/soncthrickles/source/sonic3air/menu/ExtrasMenu \
			Oxygen/soncthrickles/source/sonic3air/menu/GameApp \
			Oxygen/soncthrickles/source/sonic3air/menu/GameMenuBase \
			Oxygen/soncthrickles/source/sonic3air/menu/GameMenuManager \
			Oxygen/soncthrickles/source/sonic3air/menu/MainMenu \
			Oxygen/soncthrickles/source/sonic3air/menu/MenuBackground \
			Oxygen/soncthrickles/source/sonic3air/menu/ModsMenu \
			Oxygen/soncthrickles/source/sonic3air/menu/options/OptionsEntry \
			Oxygen/soncthrickles/source/sonic3air/menu/options/OptionsMenu \
			Oxygen/soncthrickles/source/sonic3air/menu/overlays/SecretUnlockedWindow \
			Oxygen/soncthrickles/source/sonic3air/menu/overlays/SkippableCutsceneWindow \
			Oxygen/soncthrickles/source/sonic3air/menu/PauseMenu \
			Oxygen/soncthrickles/source/sonic3air/menu/SharedResources \
			Oxygen/soncthrickles/source/sonic3air/menu/TimeAttackMenu \
			Oxygen/soncthrickles/source/sonic3air/menu/TimeAttackResultsMenu \
			Oxygen/soncthrickles/source/sonic3air/scriptimpl/ScriptImplementations \
			Oxygen/soncthrickles/source/sonic3air/_nativized/NativizedCode \
			librmx/source/rmxbase/Basics \
			librmx/source/rmxbase/BinarySerializer \
			librmx/source/rmxbase/Bitmap \
			librmx/source/rmxbase/BitmapCodecBMP \
			librmx/source/rmxbase/BitmapCodecICO \
			librmx/source/rmxbase/BitmapCodecJPG \
			librmx/source/rmxbase/BitmapCodecPNG \
			librmx/source/rmxbase/Color \
			librmx/source/rmxbase/ErrorHandler \
			librmx/source/rmxbase/FileCrawler \
			librmx/source/rmxbase/FileHandle \
			librmx/source/rmxbase/FileIO \
			librmx/source/rmxbase/FileProvider \
			librmx/source/rmxbase/FileSystem \
			librmx/source/rmxbase/InputStream \
			librmx/source/rmxbase/jsoncpp/json_reader \
			librmx/source/rmxbase/jsoncpp/json_value \
			librmx/source/rmxbase/jsoncpp/json_writer \
			librmx/source/rmxbase/JsonHelper \
			librmx/source/rmxbase/Math \
			librmx/source/rmxbase/OutputStream \
			librmx/source/rmxbase/RC4Encryption \
			librmx/source/rmxbase/rmxbase \
			librmx/source/rmxbase/RmxDeflate \
			librmx/source/rmxbase/String \
			librmx/source/rmxbase/Tools \
			librmx/source/rmxbase/VectorBinarySerializer \
			librmx/source/rmxbase/ZlibDeflate \
			librmx/source/rmxext_oggtheora/OggLoaderTheora \
			librmx/source/rmxext_oggvorbis/OggLoader \
			librmx/source/rmxext_oggvorbis/rmxext_oggvorbis \
			librmx/source/rmxmedia/AppFramework \
			librmx/source/rmxmedia/AudioBuffer \
			librmx/source/rmxmedia/AudioManager \
			librmx/source/rmxmedia/AudioMixer \
			librmx/source/rmxmedia/AudioReference \
			librmx/source/rmxmedia/Camera \
			librmx/source/rmxmedia/FileInputStreamSDL \
			librmx/source/rmxmedia/FileProviderSDL \
			librmx/source/rmxmedia/Font \
			librmx/source/rmxmedia/FontOutput \
			librmx/source/rmxmedia/FontSource \
			librmx/source/rmxmedia/Framebuffer \
			librmx/source/rmxmedia/FTX_System \
			librmx/source/rmxmedia/GLTools \
			librmx/source/rmxmedia/GuiBase \
			librmx/source/rmxmedia/GuiButton \
			librmx/source/rmxmedia/GuiImage \
			librmx/source/rmxmedia/GuiInputField \
			librmx/source/rmxmedia/GuiLabel \
			librmx/source/rmxmedia/JobManager \
			librmx/source/rmxmedia/Painter \
			librmx/source/rmxmedia/rmxmedia \
			librmx/source/rmxmedia/Shader \
			librmx/source/rmxmedia/SpriteAtlas \
			librmx/source/rmxmedia/StdResources \
			librmx/source/rmxmedia/Texture \
			librmx/source/rmxmedia/Thread \
			librmx/source/rmxmedia/VertexArrayObject \
			librmx/source/rmxmedia/VideoBuffer \
			Oxygen/oxygenengine/source/oxygen/rendering/hardware/HardwareRenderer \
			Oxygen/oxygenengine/source/oxygen/rendering/hardware/HardwareRenderResources \
			Oxygen/oxygenengine/source/oxygen/rendering/hardware/shaders/DebugDrawPlaneShader \
			Oxygen/oxygenengine/source/oxygen/rendering/hardware/shaders/RenderComponentSpriteShader \
			Oxygen/oxygenengine/source/oxygen/rendering/hardware/shaders/RenderPaletteSpriteShader \
			Oxygen/oxygenengine/source/oxygen/rendering/hardware/shaders/RenderPlaneShader \
			Oxygen/oxygenengine/source/oxygen/rendering/hardware/shaders/RenderVdpSpriteShader \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLDrawer \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLDrawerResources \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLDrawerTexture \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/OpenGLTexture \
			Oxygen/oxygenengine/source/oxygen/drawing/opengl/Upscaler \

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