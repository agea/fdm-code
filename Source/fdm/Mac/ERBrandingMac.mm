#include "CoreMinimal.h"
#include "Mac/MacSystemIncludes.h"

void ERSetMacGameIcon(const FString& IconPath)
{
    // AppKit updates belong on the main thread. The block owns its path copy.
    const FString Path = IconPath;
    dispatch_async(dispatch_get_main_queue(), ^{
        NSImage* Icon = [[NSImage alloc] initWithContentsOfFile:
            [NSString stringWithUTF8String:TCHAR_TO_UTF8(*Path)]];
        if (Icon)
        {
            [NSApp setApplicationIconImage:Icon];
            [Icon release];
        }
    });
}
