// Copyright (C) 2009-2016 by Anders Piniesjö <pugo@pugo.org>

#ifndef FRONTEND_H
#define FRONTEND_H

#include <memory>
#include <iostream>
#include <ostream>
#include <vector>
#include <SDL.h>

class Oric;
class Memory;


class Frontend
{
public:
	enum VideoAttribs
	{
		HZ_50 = 0x02,
		HIRES = 0x04
	};

	enum TextAttribs
	{
		ALTERNATE_CHARSET = 0x01,
		DOUBLE_SIZE = 0x02,
		BLINKING = 0x04,
	};

	Frontend(const Oric* oric);
	~Frontend();

	void init_graphics();
	void init_audio();
	void close_graphics();
	void update_graphics(uint8_t raster_line, uint8_t* mem);
	void render_graphics();

protected:
	uint32_t colors[8] {0xff000000, 0xffff0000, 0xff00ff00, 0xffffff00, 0xff0000ff, 0xffff00ff, 0xff00ffff, 0xffffffff};
	const uint8_t texture_width = 240;
	const uint8_t texture_height = 224;
	const uint8_t texture_bpp = 4;

	const Oric* oric;

	SDL_Window* sdl_window;
	SDL_Surface* sdl_surface;
	SDL_Renderer* sdl_renderer;
	SDL_Texture* sdl_texture;
	SDL_AudioDeviceID audio_device;

	std::vector<uint8_t> pixels;
	uint8_t video_attrib;
	uint8_t text_attrib;
};



#endif // FRONTEND_H
