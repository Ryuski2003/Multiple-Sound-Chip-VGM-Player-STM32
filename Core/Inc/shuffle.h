#ifndef _SHUFFLE_H_
#define _SHUFFLE_H_

/* On the switch's OFF->ON transition, builds the bag and places
   current_index into it; does nothing on ON->OFF or ON->ON. Called every
   iteration of the main loop with the value read from the switch pin. */
void Shuffle_SetEnabled(int on, int current_index);

int Shuffle_IsEnabled(void);

/* When a song is manually selected from the file list while shuffle is on,
   refreshes the bag so it continues from this new song. */
void Shuffle_Reseed(int current_index);

/* Returns the next/previous file index from the bag (reshuffles if the bag
   is exhausted). */
int Shuffle_Next(void);
int Shuffle_Prev(void);

#endif
