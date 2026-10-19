// macOS-specific: Dynamic dock icon switching based on theme
#include "appcore.h"

#ifdef Q_OS_MACOS

#include <AppKit/AppKit.h>
#include <QString>

void AppCore::updateApplicationIcon(bool)
{
    // Загружаем Trackly.icns из bundle
    NSString *iconPath = [[NSBundle mainBundle] pathForResource:@"Trackly" ofType:@"icns"];
    if (iconPath) {
        NSImage *icon = [[NSImage alloc] initWithContentsOfFile:iconPath];
        if (icon) {
            // явно задаём стандартный размер как у системных иконок (128pt)
            NSSize targetSize = NSMakeSize(128, 128);
            NSImage *resized = [[NSImage alloc] initWithSize:targetSize];
            [resized lockFocus];
            [icon drawInRect:NSMakeRect(0, 0, targetSize.width, targetSize.height)
                    fromRect:NSZeroRect
                   operation:NSCompositingOperationCopy
                    fraction:1.0];
            [resized unlockFocus];
            [[NSApplication sharedApplication] setApplicationIconImage:resized];
            [resized release];
            [icon release];
        }
    }
}

#else

void AppCore::updateApplicationIcon(bool /*dark*/)
{
    // No-op on non-macOS platforms
}

#endif
