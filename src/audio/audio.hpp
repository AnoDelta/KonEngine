#pragma once

// Windows defines PlaySound and PlayMusic as macros via mmsystem.h/winmm.
// Undefine them so our functions with the same names are not affected.
#ifdef _WIN32
  #ifdef PlaySound
    #undef PlaySound
  #endif
  #ifdef PlayMusic
    #undef PlayMusic
  #endif
#endif

struct Sound {
    unsigned int id;
    float volume = 1.0f;
};

struct Music {
    unsigned int id;
    float volume = 1.0f;
    bool looping = true;
};

// Init/shutdown (called internally)
void InitAudio();
void ShutdownAudio();

// Sound
Sound LoadSound(const char* path);
void UnloadSound(Sound& sound);
void PlaySound(Sound& sound);
void StopSound(Sound& sound);
void PauseSound(Sound& sound);
void ResumeSound(Sound& sound);
bool IsSoundPlaying(Sound& sound);
void SetSoundVolume(Sound& sound, float volume);

// Music
Music LoadMusic(const char* path);
void UnloadMusic(Music& music);
void PlayMusic(Music& music);
void StopMusic(Music& music);
void PauseMusic(Music& music);
void ResumeMusic(Music& music);
void UpdateMusic(Music& music);
bool IsMusicPlaying(Music& music);
void SetMusicVolume(Music& music, float volume);
void SetMusicLooping(Music& music, bool loop);

// Master
void SetMasterVolume(float volume);
