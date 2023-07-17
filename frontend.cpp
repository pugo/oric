// =========================================================================
//   Copyright (C) 2009-2023 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>
// =========================================================================

#include "frontend.hpp"

#include <SDL_image.h>


Frontend::Frontend(const Oric* oric) :
	oric(oric),
	sdl_window(NULL),
	sdl_surface(NULL),
	sdl_renderer(NULL),
	video_attrib(0),
	text_attrib(0),
    sound_frequency(440),
    sound_high(0),
    sound_samples_played(0)
{
	pixels = std::vector<uint8_t>(texture_width * texture_height * texture_bpp, 0);
}

Frontend::~Frontend()
{
    close_graphics();
    close_sound();
    close_sdl();
}

bool Frontend::init_graphics()
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
	}
	else {
		// Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
			std::cout <<  "Warning: Linear texture filtering not enabled!" << std::endl;
		}

		// Create window (240x224)
		sdl_window = SDL_CreateWindow("Pugo-Oric", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 600, SDL_WINDOW_SHOWN);
		if (sdl_window == NULL) {
			std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
		}
		else {
			// Create renderer for window
			sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
			if (sdl_renderer == NULL) {
				std::cout <<  "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
                return false;
			}
			else {
				SDL_RendererInfo info;
				SDL_GetRendererInfo(sdl_renderer, &info);
				for (uint32_t i = 0; i < info.num_texture_formats; i++) {
					std::cout << SDL_GetPixelFormatName( info.texture_formats[i] ) << std::endl;
				}

				sdl_texture = SDL_CreateTexture(sdl_renderer,
                                                SDL_PIXELFORMAT_ARGB8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                texture_width,
                                                texture_height);

				// Initialize renderer color
				SDL_SetRenderDrawColor(sdl_renderer, 0xff, 0xff, 0xff, 0xff);
				SDL_RenderClear(sdl_renderer);
			}
		}
	}

    return true;
}


void Frontend::close_graphics()
{
    //Destroy window
    SDL_DestroyWindow(sdl_window);
    sdl_window = NULL;
}


void audio_callback(void* user_data, uint8_t* raw_buffer, int len)
{
    std::cout << "audio_callback" << std::endl;

    Frontend* frontend = (Frontend*)user_data;
    Sint16* buffer = (Sint16*)raw_buffer;

    for(int i = 0; i < (len / 2); ++i, ++frontend->sound_samples_played)
    {
        if ((frontend->sound_samples_played % 100) == 0) {
            frontend->sound_high = !frontend->sound_high;
        }

//        double time = (double)frontend->sound_samples_played / 44100.0;
        buffer[i] = (Sint16)28000 * frontend->sound_high;
    }
}


bool Frontend::init_sound()
{
    std::cout << "Initializing sound.." << std::endl;

    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        std::cout << "Error: failed initializing SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_AudioSpec audio_spec_want, audio_spec;
    SDL_memset(&audio_spec_want, 0, sizeof(audio_spec_want));

    audio_spec_want.freq     = 44100;
    audio_spec_want.format   = AUDIO_S16SYS;
    audio_spec_want.channels = 1;
    audio_spec_want.samples  = 2048;
    audio_spec_want.callback = audio_callback;
    audio_spec_want.userdata = (void*) this;

    sound_audio_device_id = SDL_OpenAudioDevice(NULL,
                                                0,
                                                &audio_spec_want,
                                                &audio_spec,
                                                SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if(!sound_audio_device_id)
    {
        std::cout << "Error: creating SDL audio device: " << SDL_GetError() << std::endl;
        return false;
    }

    if (audio_spec_want.format != audio_spec.format) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
    }

//    SDL_Delay(100); // wait while sound is playing
//    SDL_PauseAudioDevice(sound_audio_device_id, 0);
//    SDL_Delay(2000); // wait while sound is playing
//    SDL_PauseAudioDevice(sound_audio_device_id, 1);

    return true;
}


void Frontend::close_sound()
{
    SDL_CloseAudioDevice(sound_audio_device_id);
}


void Frontend::close_sdl()
{
    SDL_Quit(); // Quit all SDL subsystems
}


// Return memory address corresponding to a raster line, for current video mode.
inline uint16_t calcRowAddr(uint8_t a_RasterLine, uint8_t a_VideoAttrib)
{
	if (a_VideoAttrib & Frontend::VideoAttribs::HIRES && a_RasterLine < 200) {		
		return 0xa000 + a_RasterLine * 40;			// Hires: return address for hires data for line.
	}
	return 0xbb80 + (a_RasterLine >> 3) * 40;		// Text (lores or > 200): return address to char data for line (>>3).
}

void Frontend::update_graphics(uint8_t raster_line, uint8_t* mem)
{
	uint8_t dx = 0;
	uint32_t bg_col = colors[0];
	uint32_t fg_col = colors[7];
	text_attrib = 0;

	uint32_t* texture_line = (uint32_t*)&pixels[raster_line * texture_width * texture_bpp];
	uint16_t row = calcRowAddr(raster_line, video_attrib);

	// 40 characters wide, regardless of lores or hires.
	for (uint16_t x = 0; x < 40; x++) {
		// get char code.
		uint8_t ch = mem[row + x];
		bool ctrl_char = false;
		
		if (!(ch & 0x60)) {
			ctrl_char = true;
			switch(ch & 0x18)
			{
			case 0x00:
				// Ink color.
				fg_col = colors[ch & 7];
				break;
			case 0x08:
				// Text attributes.
				text_attrib = ch & 7;
				break;
			case 0x10:
				// Paper color.
				bg_col = colors[ch & 7];
				break;
			case 0x18:
				// Video control attrs.
				video_attrib = ch & 0x07;
				row = calcRowAddr(raster_line, video_attrib);
				break;
			}
		}

		uint32_t _fg_col = fg_col;
		uint32_t _bg_col = bg_col;

		// Inverse colors if upper bit is set in char code.
		if (ch & 0x80) {
			_fg_col = _fg_col ^ 0x00ffffff;
			_bg_col = _bg_col ^ 0x00ffffff;
		}

		uint8_t chr_dat = 0;
		if (!ctrl_char) {
			if ((video_attrib & VideoAttribs::HIRES) && raster_line < 200) {
				chr_dat = ch;	// Hires, read byte directly.
			}
			else {
				// get char pixel data for read char code. If hires > 200, charmem is at 0x9800.
				uint8_t* char_mem = mem + ((video_attrib & VideoAttribs::HIRES) ? 0x9800 : 0xb400) +
										  ((text_attrib & TextAttribs::ALTERNATE_CHARSET) ? 128 * 8 : 0);
				chr_dat = char_mem[((ch & 0x7f) << 3) + (raster_line & 0x07)] & 0x3f;
			}
		}

		// Copy 6 pixels.
		for (uint8_t i = 0x20; i > 0; i >>= 1, dx++) {
			*texture_line++ = (chr_dat & i) ? _fg_col : _bg_col;
		}
	}
}

void Frontend::render_graphics()
{
	SDL_UpdateTexture(sdl_texture, NULL, &pixels[0], texture_width * texture_bpp);
	SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL );
	SDL_RenderPresent(sdl_renderer);
}

