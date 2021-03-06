/*
 * This file is part of the FreeStreamer project,
 * (C)Copyright 2011-2015 Matias Muhonen <mmu@iki.fi>
 * See the file ''LICENSE'' for using the code.
 *
 * https://github.com/muhku/FreeStreamer
 */

#import "FSPlaylistItem.h"

@implementation FSPlaylistItem

- (BOOL)isEqual:(id)anObject
{
    FSPlaylistItem *otherObject = anObject;
    
    if ([otherObject.title isEqual:self.title] &&
        [otherObject.url isEqual:self.url]) {
        return YES;
    }
    
    return NO;
}

@end

