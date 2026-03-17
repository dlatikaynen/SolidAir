#pragma once

#include <windows.h>

typedef enum {
	None = -1,
	Laub = 0,
	Karo = 1,
	Herz = 2,
	Pik = 3
} Suite;

typedef enum {
	Empty = -1,

	// ♣
	LaubAsslikum = 0,
	Laub2 = 1,
	Laub3 = 2,
	Laub4 = 3,
	Laub5 = 4,
	Laub6 = 5,
	Laub7 = 6,
	Laub8 = 7,
	Laub9 = 8,
	Laub10 = 9,
	LaubBublikum = 10,
	LaubDamlikum = 11,
	LaubKinigl = 12,

	// ♦
	KaroAsslikum = 13,
	Karo2 = 14,
	Karo3 = 15,
	Karo4 = 16,
	Karo5 = 17,
	Karo6 = 18,
	Karo7 = 19,
	Karo8 = 20,
	Karo9 = 21,
	Karo10 = 22,
	KaroBublikum = 23,
	KaroDamlikum = 24,
	KaroKinigl = 25,

	// ♥
	HerzAsslikum = 26,
	Herz2 = 27,
	Herz3 = 28,
	Herz4 = 29,
	Herz5 = 30,
	Herz6 = 31,
	Herz7 = 32,
	Herz8 = 33,
	Herz9 = 34,
	Herz10 = 35,
	HerzBublikum = 36,
	HerzDamlikum = 37,
	HerzKinigl = 38,

	// ♠
	PikAsslikum = 39,
	Pik2 = 40,
	Pik3 = 41,
	Pik4 = 42,
	Pik5 = 43,
	Pik6 = 44,
	Pik7 = 45,
	Pik8 = 46,
	Pik9 = 47,
	Pik10 = 48,
	PikBublikum = 49,
	PikDamlikum = 50,
	PikKinigl = 51,
	
	Joker = 52,

	BackBloodot = 54,
	BackCat = 55,
	BackColorful = 56,
	BackCubert = 57,
	BackDivine = 58,
	BackPaper = 59,
	BackParkett = 60,
	BackRed = 61,
	BackRocks = 62,
	BackSky = 63,
	BackSpace = 64,
	BackIce = 65,
	BackMaple = 66,
	BackPine = 67,
	BackSafety = 68
} Cards;

typedef enum {
	Default = -1,
	Green = 0,
	Black,
	DarkGray,
	DarkBlue,
	Pink
} BackgroundColors;

typedef enum {
	unknown = -1,
	en = 0,
	de = 1,
	fi = 2,
	ua = 3
} FrontendLanguage;

typedef enum {
	Ignore,
	MustDifferInColor,
	MustMatchSuite
} CardColorMatchMode;

typedef struct {
	int left;
	int top;
	int right;
	int bottom;
} CardRect;

typedef struct {
	int numCardsOnPile;
	Cards pile[52];
	CardRect pos;
	int uncovered; // -1 means not uncovered
} StockPile;

typedef struct {
	int numCardsOnPile;
	Cards pile[13];
	CardRect pos;
} TargetPile;

typedef struct {
	int numCardsOnPile;
	int uncoveredFrom; // index
	Cards pile[100];
	CardRect pos;
} DagoPile;

typedef struct {
	COLORREF bkgColor;
	Cards backside;
	StockPile stockpile;
	TargetPile targetPiles[4];
	DagoPile dagoPiles[7];
} GameState;

typedef struct {
	bool fromStockpile;
	DagoPile* pile; // nullptr if from stockpile
	int index;
} CardsBeingHit;

#pragma pack (1)
typedef struct SettingsStruct
{
	char Preamble0 = '\4';
	char Preamble1 = 'L';
	char Preamble2 = 'S';
	char Preamble3 = 'L';
	char Preamble4 = '\3';
	char Preamble5 = 'J';
	char Preamble6 = 'M';
	char Preamble7 = 'L';
	char Preamble8 = '\5';
	char Preamble9 = '\6'; // bloodot has 0 here
	BackgroundColors backgroundColor;
	Cards cardBackside;
	FrontendLanguage language;
	bool sfxOn;
	bool musicOn;
	bool commentaryOn;
} SettingsStruct;
