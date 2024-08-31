#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

class Audio {

protected:
	Mix_Chunk* checkpointSound = nullptr;
	Mix_Chunk* lapSound = nullptr;
	Mix_Chunk* clappingSound = nullptr;

public:
	bool InitAudio() {
		if (SDL_Init(SDL_INIT_AUDIO) < 0) {
			return false;
		}

		if (Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3) == 0) {
			SDL_Quit();
			return false;
		}

		if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
			Mix_Quit();
			SDL_Quit();
			return false;
		}

		return true;
	}

	void CleanupAudio() {
		Mix_CloseAudio();
		Mix_Quit();
		SDL_Quit();
	}

	bool LoadSounds() {
		checkpointSound = Mix_LoadWAV("audio/checkpoint.wav");
		if (!checkpointSound) {
			std::cerr << "Failed to load checkpoint sound" << std::endl;
			return false;
		}
		lapSound = Mix_LoadWAV("audio/lap.wav");
		if (!checkpointSound) {
			std::cerr << "Failed to load checkpoint sound" << std::endl;
			return false;
		}
		clappingSound = Mix_LoadWAV("audio/clapping.wav");
		if (!checkpointSound) {
			std::cerr << "Failed to load checkpoint sound" << std::endl;
			return false;
		}
		return true;
	}

	void PlayCheckpointSound() {
		if (checkpointSound) {
			Mix_PlayChannel(-1, checkpointSound, 0);
		}
	}
	
	void PlayLapSound() {
		if (lapSound) {
			Mix_PlayChannel(-1, lapSound, 0);
		}
	}
	
	void ClappingSound() {
		if (lapSound) {
			Mix_PlayChannel(-1, lapSound, 0);
		}
	}

	void AudioCleanup() {
		Mix_FreeChunk(checkpointSound);
		checkpointSound = nullptr;
		lapSound = nullptr;
		CleanupAudio();
	}

};