/*
 * This file is part of the FreeStreamer project,
 * (C)Copyright 2011-2015 Matias Muhonen <mmu@iki.fi>
 * See the file ''LICENSE'' for using the code.
 *
 * https://github.com/muhku/FreeStreamer
 */

#import <Foundation/Foundation.h>

/**
 * A playlist item. Each item has a title and url.
 */
@interface FSPlaylistItem : NSObject {
}

/**
 * The title of the playlist item.
 */
@property (nonatomic,copy) NSString *title;
/**
 * The URL of the playlist item.
 */
@property (nonatomic,copy) NSURL *url;
/**
 * The originating URL of the playlist item.
 */
@property (nonatomic,copy) NSURL *originatingUrl;

@end

NS_INLINE FSPlaylistItem* FSPlaylistItemMake(NSString* title, NSURL* url, NSURL* originalURL)
{
    FSPlaylistItem* item = [FSPlaylistItem new];
    item.title = title;
    item.url = url;
    item.originatingUrl = originalURL;
    return item;
}