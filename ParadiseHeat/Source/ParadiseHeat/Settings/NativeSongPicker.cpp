#include "NativeSongPicker.h"
#if !PLATFORM_MAC
void OpenNativeSongPicker(TFunction<void(FString)> Completion){Completion(FString());}
#endif
