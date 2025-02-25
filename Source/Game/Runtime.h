#ifndef __RUNTIME_H__
#define __RUNTIME_H__

extern SyncroTimer global_time;	// детерминированный таймер
extern SyncroTimer frame_time; // недетерминированный таймер
extern SyncroTimer scale_time; // недетерминированный ускоряемый таймер

//--------------------------------------

void app_event_poll();
void setLogicFp();

void PlayMusic(const char *str);
void SetVolumeMusic(float f);
void MusicEnable(int enable);
void InitSound(bool sound, bool music, bool firstTime = true);
void SoundQuant();
void FinitSound();
void request_application_restart(std::vector<std::string>* args = nullptr);

//--------------------------------------
extern class cVisGeneric* terVisGeneric;
extern class cInterfaceRenderDevice* terRenderDevice;
extern class cLogicGeneric* terLogicGeneric;

extern class cScene* terScene;
extern class cUnkLight* terLight;
extern class cTileMap* terMapPoint;

#ifdef GPX
extern const int terFullScreen;
#else
extern int terFullScreen;
#endif
extern int terScreenSizeX;
extern int terScreenSizeY;
extern int terBitPerPixel;
extern int terScreenRefresh;
extern int terScreenIndex;
extern int terGrabInput;

extern int terMapReflection;
extern int terObjectReflection;

extern int terSoundEnable;		// 0,1
extern int terMusicEnable;		// 0,1
extern float terSoundVolume;	// 0..1
extern float terMusicVolume;	// 0..1

extern float terGraphicsGamma;	// 0.5..2.5

extern int terShadowType;
extern int terDrawMeshShadow;
extern bool terEnableBumpChaos;
void SetShadowType(int shadow_map,int shadow_size,bool update);

extern const char* currentVersion;
extern const char* currentShortVersion;
extern uint16_t currentVersionNumbers[];

//-------------------------------------------------
bool openFileDialog(std::string& filename, const char* initialDir, const char* extention, const char* title);
bool saveFileDialog(std::string& filename, const char* initialDir, const char* extention, const char* title);
const char* popupMenu(std::vector<const char*> items); // returns zero if cancel
int popupMenuIndex(std::vector<const char*> items); // returns -1 if cancel
const char* editText(const char* defaultValue);
const char* editTextMultiLine(const char* defaultValue, void* hwnd);

#endif //__RUNTIME_H__