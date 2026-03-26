#define MINIAUDIO_IMPLEMENTATION
#include "../miniaudio/miniaudio.h"
#include "audio.hpp"
#include <iostream>
#include <unordered_map>

static ma_engine engine;
static bool audioInitialized = false;

static std::unordered_map<unsigned int, ma_sound*> sounds;
static std::unordered_map<unsigned int, ma_sound*> music;
static unsigned int nextID = 1;

static void EnsureAudioInitialized() {
    if (!audioInitialized) {
        InitAudio();
        atexit(ShutdownAudio);
    }
}

void InitAudio() {
    ma_result result = ma_engine_init(nullptr, &engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine" << std::endl;
        return;
    }
    audioInitialized = true;
}

void ShutdownAudio() {
    for (auto& [id, sound] : sounds) {
        ma_sound_uninit(sound);
        delete sound;
    }
    for (auto& [id, m] : music) {
        ma_sound_uninit(m);
        delete m;
    }
    sounds.clear();
    music.clear();
    ma_engine_uninit(&engine);
    audioInitialized = false;
}

// Sound
Sound LoadSound(const char* path) {
	EnsureAudioInitialized();

    ma_sound* s = new ma_sound();
    ma_result result = ma_sound_init_from_file(&engine, path, 0, nullptr, nullptr, s);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load sound: " << path << std::endl;
        delete s;
        return {0};
    }

    unsigned int id = nextID++;
    sounds[id] = s;
    return {id};
}

void UnloadSound(Sound& sound) {
    auto it = sounds.find(sound.id);
    if (it == sounds.end()) return;
    ma_sound_uninit(it->second);
    delete it->second;
    sounds.erase(it);
    sound.id = 0;
}

void PlaySound(Sound& sound) {
    auto it = sounds.find(sound.id);
    if (it == sounds.end()) return;
    ma_sound_seek_to_pcm_frame(it->second, 0);
    ma_sound_start(it->second);
}

void StopSound(Sound& sound) {
    auto it = sounds.find(sound.id);
    if (it == sounds.end()) return;
    ma_sound_stop(it->second);
}

void PauseSound(Sound& sound) {
    auto it = sounds.find(sound.id);
    if (it == sounds.end()) return;
    ma_sound_stop(it->second);
}

void ResumeSound(Sound& sound) {
    auto it = sounds.find(sound.id);
    if (it == sounds.end()) return;
    ma_sound_start(it->second);
}

bool IsSoundPlaying(Sound& sound) {
    auto it = sounds.find(sound.id);
    if (it == sounds.end()) return false;
    return ma_sound_is_playing(it->second);
}

void SetSoundVolume(Sound& sound, float volume) {
    auto it = sounds.find(sound.id);
    if (it == sounds.end()) return;
    sound.volume = volume;
    ma_sound_set_volume(it->second, volume);
}

// Music
Music LoadMusic(const char* path) {
	EnsureAudioInitialized();

    ma_sound* s = new ma_sound();
    ma_result result = ma_sound_init_from_file(&engine, path,
        MA_SOUND_FLAG_STREAM, nullptr, nullptr, s);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load music: " << path << std::endl;
        delete s;
        return {0};
    }

    ma_sound_set_looping(s, MA_TRUE);

    unsigned int id = nextID++;
    music[id] = s;
    return {id, 1.0f, true};
}

void UnloadMusic(Music& m) {
    auto it = music.find(m.id);
    if (it == music.end()) return;
    ma_sound_uninit(it->second);
    delete it->second;
    music.erase(it);
    m.id = 0;
}

void PlayMusic(Music& m) {
    auto it = music.find(m.id);
    if (it == music.end()) return;
    ma_sound_seek_to_pcm_frame(it->second, 0);
    ma_sound_start(it->second);
}

void StopMusic(Music& m) {
    auto it = music.find(m.id);
    if (it == music.end()) return;
    ma_sound_stop(it->second);
}

void PauseMusic(Music& m) {
    auto it = music.find(m.id);
    if (it == music.end()) return;
    ma_sound_stop(it->second);
}

void ResumeMusic(Music& m) {
    auto it = music.find(m.id);
    if (it == music.end()) return;
    ma_sound_start(it->second);
}

void UpdateMusic(Music& m) {
    // miniaudio streams automatically, nothing needed here
    // kept for API consistency
}

bool IsMusicPlaying(Music& m) {
    auto it = music.find(m.id);
    if (it == music.end()) return false;
    return ma_sound_is_playing(it->second);
}

void SetMusicVolume(Music& m, float volume) {
    auto it = music.find(m.id);
    if (it == music.end()) return;
    m.volume = volume;
    ma_sound_set_volume(it->second, volume);
}

void SetMusicLooping(Music& m, bool loop) {
    auto it = music.find(m.id);
    if (it == music.end()) return;
    m.looping = loop;
    ma_sound_set_looping(it->second, loop ? MA_TRUE : MA_FALSE);
}

void SetMasterVolume(float volume) {
    if (!audioInitialized) return;
    ma_engine_set_volume(&engine, volume);
}
