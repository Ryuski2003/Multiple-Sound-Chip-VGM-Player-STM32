#include "notes.h"


#define BPM 160
#define silence 0.2

float melodyNotes[][2] = {
		{NOTE_C4,EIGHTHNOTE}, {NOTE_G4,EIGHTHNOTE}, {NOTE_C4,EIGHTHNOTE}, {NOTE_G4,EIGHTHNOTE}, {NOTE_C4,EIGHTHNOTE}, {NOTE_G4,EIGHTHNOTE}, {NOTE_C4,EIGHTHNOTE}, {NOTE_G4,EIGHTHNOTE},
		{NOTE_G3,EIGHTHNOTE}, {NOTE_D4,EIGHTHNOTE}, {NOTE_G3,EIGHTHNOTE}, {NOTE_D4,EIGHTHNOTE}, {NOTE_G3,EIGHTHNOTE}, {NOTE_D4,EIGHTHNOTE}, {NOTE_G3,EIGHTHNOTE}, {NOTE_D4,EIGHTHNOTE},
		{NOTE_A3,EIGHTHNOTE}, {NOTE_E4,EIGHTHNOTE}, {NOTE_A3,EIGHTHNOTE}, {NOTE_E4,EIGHTHNOTE}, {NOTE_A3,EIGHTHNOTE}, {NOTE_E4,EIGHTHNOTE}, {NOTE_A3,EIGHTHNOTE}, {NOTE_E4,EIGHTHNOTE},
		{NOTE_F3,EIGHTHNOTE}, {NOTE_C4,EIGHTHNOTE}, {NOTE_F3,EIGHTHNOTE}, {NOTE_C4,EIGHTHNOTE}, {NOTE_F3,EIGHTHNOTE}, {NOTE_C4,EIGHTHNOTE}, {NOTE_F3,EIGHTHNOTE}, {NOTE_C4,EIGHTHNOTE}
};
float bassNotes[][2] = {
		{NOTE_C3,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_C3,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_C3,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_C3,QUARTERNOTE-silence}, {0.0, (float)silence},
		{NOTE_G2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_G2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_G2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_G2,QUARTERNOTE-silence}, {0.0, (float)silence},
		{NOTE_A2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_A2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_A2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_A2,QUARTERNOTE-silence}, {0.0, (float)silence},
		{NOTE_F2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_F2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_F2,QUARTERNOTE-silence}, {0.0, (float)silence}, {NOTE_F2,QUARTERNOTE-silence}, {0.0, (float)silence}
};
