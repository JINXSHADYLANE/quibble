// Ego - distilled essence of the Greed, C++ flavour.

/*

============
=== Docs ===
============

A few curious things about ego should be noticed:

0)  Your main function should have following signature: 
    int dgreed_main(int argc, const char** argv)
	Also, it must be inside 'extern "C" { ... } '
1)  You must poll Input::process in your main loop and stop it when return value
    is false.
2)  You must call Log::init with sensible arguments before anything else in your
    main function.
3)  Exceptions are not used. All serious errors stop execution of the program. 
    Read logs, use debugger, and don't cause serious errors.
4)  Memory manager only tracks internal memory usage of the library and won't 
    show your own memory leaks. Use C if you need real memory manager!
5) 	MML is great! However, it exploits some propierties of C strings (like
	keeping lots of null-terminated strings in a single memory buffer) and C++
	wrapper would lose efficiency. You can use C version in C++ without any
	problems.
6)  Textures must have power-of-2 dimensions. Width and height can differ tough, 
    as long as it's pow2.
7)  There are 3 officially supported asset formats - png for images, wav for 
    sounds and ogg for music. Some implementations accidentaly can support
    more (eg. SDL dgreed for PCs supports jpeg, tga, bmp...), but you shouldn't 
    depend on it.
8)  There are some arbitrary limits built into the engine. For example, your app
    will crash if you draw more than 512 rects in a single layer or try to load 
    more than 64 sounds. These should be interpreted as (rather strict) 
    guidelines for politeness to not-so-powerful platforms like iPhone.
9)  Use Ego file IO functions. They do some magic, like swap some bytes here and 
    there on big-endian architectures and perform rituals for a painless 
    contruction of Mac OS bundles from your apps.
10) Use texture atlasses! Piles of little images can kill performance.   

*/

// TODO: GUI Descs, particles

#ifndef EGO_HPP
#define EGO_HPP

#include <string>
#include <cstdarg>

namespace ego {
	const float pi = 3.14159265f;
	typedef unsigned int uint;
	typedef std::string string;

	// Two most important functions ever

	template <typename T>
	T lerp(const T& a, const T& b, float t) {
		return a + (b-a) * t;
	}

	template <typename T>
	T smoothstep(const T& a, const T& b, float t) {
		return lerp(a, b, t * t * (3.0f - 2.0f * t)); 
	}

	// Easy logging to a text file
	class Log {
	public:
		// Every app must initialize and close log! 
		static void init(const string& filename);
		static void close();

		// Log messages of variuos scaryness
		void info(const char* format, ...);
		void info(const string& msg);
		void warning(const char* format, ...);
		void warning(const string& msg);
		void error(const char* format, ...);
		void error(const string& msg);
	};

	// Read-only file
	class IFile {
	public:
		~IFile();

		uint size();
		// Seek position is relative to beginning
		void seek(uint pos);

		unsigned char readByte();
		unsigned short readUint16();
		unsigned int readUint32();
		float readFloat();
		void read(void* dest, uint size);
	private:	
		friend class Filesystem;
		IFile();
		size_t handle;	
	};
	
	// Write-only file
	class OFile {
	public:
		~OFile();

		void writeByte(unsigned char data);
		void writeUint16(unsigned short data);
		void writeUint32(unsigned int data);
		void writeFloat(float data);
		void write(void* data, uint size);
	private:
		friend class Filesystem;
		OFile();
		size_t handle;
	};	

	// Collection of FS-related stuff
	class Filesystem {
		// Returns true if such file exists
		static bool exists(const string& name);

		// Files are either readable or writable
		static IFile* open(const string& name);
		static OFile* create(const string& name);

		// If you're reading/writing text files, there is no need to use
		// IFile/OFile objects. Following functions simply read/write all text
		// in a file to/from a string.
		static string readText(const string& name);
		static void writeText(const string& name, const string& text);
	};

	#pragma pack(push, 1)

	// Two following classes are self-explanatory
	
	class Vector2 {
	public:
		Vector2();
		Vector2(const Vector2& v);
		Vector2(float x, float y);

		Vector2& operator=(const Vector2& v);
		
		Vector2 operator +(const Vector2& v) const;
		Vector2& operator +=(const Vector2& v);
		Vector2 operator -(const Vector2& v) const;
		Vector2& operator -=(const Vector2& v);
		Vector2 operator *(float s) const;
		Vector2& operator *=(float s);
		Vector2 operator /(float s) const;
		Vector2& operator /=(float s);
		Vector2 operator -() const;

		static float dot(const Vector2& v1, const Vector2& v2);
		
		float lengthSquared() const;
		float length() const;
		
		void normalize();
		Vector2 normalized() const;

		float* getPtr();
		const float* getConstPtr() const;

		float x, y;
	};	

	class RectF {
	public:
		RectF();
		RectF(const RectF& r);
		RectF(float l, float t, float r, float b);

		RectF& operator =(const RectF& r);

		float width();
		float height();
		
		bool collidePoint(const Vector2& point);
		bool collideCircle(const Vector2& center, float radius);

		float left, top, right, bottom;
	};
	#pragma pack(pop)

	// 32-bit RGBA color. Allows easy conversion to/from HSV!
	class Color {
	public:	
		Color();
		Color(const Color& c);
		Color(float r, float g, float b, float a = 1.0f);
		static Color hsv(float h, float s, float v, float a = 1.0f);

		static Color lerp(Color a, Color b, float t);

		void toRgba(float& r, float& g, float& b, float& a);
		void toHsva(float& h, float& s, float& v, float& a);

		uint value;

		static const Color white;
		static const Color black;
	};

	// Image, in a gpu-friendly form
	class Texture {
	public:
		~Texture();

		// Size in pixels
		uint width();
		uint height();
	private:
		friend class Video;
		Texture();
		uint handle, mwidth, mheight;
	};

	// Bitmap font, with all glyphs nicely packed in a texture
	class Font {
	public:
		~Font();

		// Width of a text line in pixels.
		float width(const string& text);
		// Height of a text line in pixels!
		float height();

		// Calculates exact bounding box of a centered piece of text. Newlines
		// are not handled automaticaly. 
		RectF bbox(const string& text, const Vector2& center, 
			float scale = 1.0f);

		// Draws a line of text on screen
		void draw(const string& text, uint layer, const Vector2& topleft, 
			Color tint, float scale = 1.0f);
	private:
		friend class Video;
		Font();
		uint handle;
	};

	// Immediate mode GUI for lazy people
	class GUI {
	public:
		// TODO: Support custom GuiDescs

		~GUI();
		// GUI needs gui.png, nova.bft and nova_00.png files. You must show
		// where they can be found in assets_prefix.
		static GUI* init(const string& assets_prefix);

		// Draws interactive widgets. Position values are used for some magic
		// calculations and you absolutely must not animate widget positions!
		void label(const Vector2& pos, const string& text);
		bool button(const Vector2& pos, const string& text);
		bool _switch(const Vector2& pos, const string& text);
		float slider(const Vector2& pos);

		// Allows to get/set switch and slider states by their positions.
		bool getSwitchState(const Vector2& pos);
		float getSliderState(const Vector2& pos);
		void setSwitchState(const Vector2& pos, bool state);
		void setSliderState(const Vector2& pos, float state);

	private:
		GUI();
	};

	// Pretty things are being drawn here
	class Video {
	public:	
		~Video();

		// Inits video. Virtual width and height is resolution in which all
		// coordinates and calculations are done in your app. Just before
		// drawing everything to screen it is rescaled to real resolution. Eg.
		// you can pretend you are drawing to 480x320 screen while real
		// resolution can be 960x480, 1920x960 or anything else. Fullscreen
		// and title means nothing on most devices, except PCs.
		static Video* init(uint width, uint height, const string& title, 
				uint v_width = 0, uint v_height = 0, bool fullscreen = false); 

		// Nothing appears on screen until present is called. Usually indicates
		// end of a frame.
		void present();
		// Number of frames drawn.
		uint getFrame();

		// Loads texture from a png file. Must have pow2 size!
		Texture* loadTexture(const string& filename);

		// Loads font from bft file. Currently, bfts are made with fnt2bft.py
		// tool, from AngelCode Bitmap Font Generator xml fnt files. Expect a
		// proper tool in future. Scale allows to artificially change font size.
		Font* loadFont(const string& filename, float scale = 1.0f);

		// Various ways to draw a rectangle. Layer must be in range [0, 15],
		// things with smaller layer value are always drawn in front of things
		// with higher layer. All coordinates are in pixels and in CG 
		// (positive y goes down) coordinate space. Rotation angle is in radians.
		void drawRect(Texture* tex, uint layer, const Vector2& dest);
		void drawRect(Texture* tex, uint layer, const RectF& source,
				const Vector2& dest, float rotation = 0.0f, 
				Color tint = Color::white);
		void drawRect(Texture* tex, uint layer, const RectF& source,
				const RectF& dest, float rotation = 0.0f, 
				Color tint = Color::white);
		void drawCenteredRect(Texture* tex, uint layer, const RectF& source,
				const Vector2& dest, float rotation = 0.0f, float scale = 1.0f,
				Color tint = Color::white);

		// Draw a pixel-wide line segment.		
		void drawLine(uint layer, const Vector2& start, const Vector2& end,
				Color color);

	private:
		Video();
	};

	// Represents specific stream or sample
	class Sound {
	public:
		~Sound();

		// Plays sound. Samples are played once, streams loop.
		void play();

		// Stopping and changing volume doesn't work yet!
		void stop();
		float getVolume();
		void setVolume(float value);
	
	private:
		friend class Audio;
		Sound();
		uint handle;
	};

	// Sounds are born here
	class Audio {
	public:	
		~Audio();
		static Audio* init();

		// Must be called frequently. Once a frame is usually good enough.
		void update();

		// Loads sound assets. "Stream" is read from ogg vorbis file and
		// streamed from disk using very small amount of memory, "Sample"
		// is read once from wav file and lives in memory all the time.
		Sound* loadStream(const string& filename);
		Sound* loadSample(const string& filename);

	private:
		Audio();
	};

	// Keyboard, mouse, touchscreen ...
	class Input {
	public:	
		// Some devices don't have keyboards! Still, a few generic keys are
		// provided for you. On PCs - A is 'z', B is 'x' and pause is 'p',
		// others shouldn't too difficult to figure out. Count is not a key!
		enum Key {
			KEY_UP = 0,
			KEY_DOWN,
			KEY_LEFT,
			KEY_RIGHT,
			KEY_A,
			KEY_B,
			KEY_PAUSE,
			KEY_QUIT,
			KEY_COUNT
		};

		enum MouseButton {
			MBNT_LEFT = 0,
			MBTN_RIGHT,
			MBTN_MIDDLE,
			MBTN_OTHER
		};

		~Input();
		static Input* init();

		// This should be called before every frame.
		// Program must exit if return value is false!
		bool process();

		// 'Pressed' - is currently held down,
		// 'Down' - was just pressed down,
		// 'Up' - was just released.
		bool keyPressed(Key key);
		bool keyDown(Key key);
		bool keyUp(Key key);
		bool mousePressed(MouseButton button);
		bool mouseDown(MouseButton button);
		bool mouseUp(MouseButton button);

		// Coordinates are in virtual pixels. See Video::init.
		void mousePos(int& x, int& y);
		Vector2 mousePos();

	private:
		Input();
	};

	// Currently, apps are locked to 60 fps fixed time-step.
	// This simplifies a lot of things for some devices and 
	// allows you to do painless and stable physics simulation. 
	// On the other hand, if your app doesn't fit into 60 fps
	// it will be in slo-mo.
	class Time {
	public:
		// Miliseconds since application start
		static float ms();
		// Miliseconds since last frame
		static float delta();
		// Current frames per second
		static uint fps();
	};	

	// Random numbers. You must seed randomizer by calling init!
	class Random {
	public:
		static void init(uint seed);

		static uint _uint();
		static int _int(int min, int max);
		static float _float();
		static float _float(float min, float max);
	};
}	

#endif
