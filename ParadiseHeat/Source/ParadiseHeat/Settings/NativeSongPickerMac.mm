#include "NativeSongPicker.h"
#if PLATFORM_MAC
#include "Async/Async.h"
#include "Mac/CocoaThread.h"
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
void OpenNativeSongPicker(TFunction<void(FString)> Completion)
{
 auto Callback=MakeShared<TFunction<void(FString)>,ESPMode::ThreadSafe>(MoveTemp(Completion));
 MainThreadCall(^{
  NSOpenPanel* Panel=[NSOpenPanel openPanel];
  Panel.title=@"Choose background music";
  Panel.canChooseFiles=YES;Panel.canChooseDirectories=NO;Panel.allowsMultipleSelection=NO;
  Panel.allowedContentTypes=@[[UTType typeWithFilenameExtension:@"mp3"],[UTType typeWithFilenameExtension:@"wav"],[UTType typeWithFilenameExtension:@"m4a"],[UTType typeWithFilenameExtension:@"aac"]];
  [Panel beginWithCompletionHandler:^(NSModalResponse Result){
   const FString Path=Result==NSModalResponseOK?FString(UTF8_TO_TCHAR(Panel.URL.path.UTF8String)):FString();
   AsyncTask(ENamedThreads::GameThread,[Callback,Path](){(*Callback)(Path);});
  }];
 },false);
}
#endif
