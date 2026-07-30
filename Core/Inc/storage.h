#ifndef _STORAGE_H_
#define _STORAGE_H_

/* Called once at boot: reads the last song index stored in flash
   (leaves the factory default if it's not valid). */
void Storage_Init(void);

/* The currently known last song index (from Storage_Init or the last Save call). */
int Storage_GetLastSongIndex(void);

/* Called when the song changes: writes to internal flash (the last sector of
   Bank2) if the index changed. Does not touch flash if the same index comes
   in again. */
void Storage_SaveLastSongIndex(int index);

#endif
