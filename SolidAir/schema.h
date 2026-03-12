#pragma once

typedef enum {

} Cards;

typedef struct {
	int numCardsOnPile;
	Cards pile[13];
} TargetPile;

typedef struct {
	int numCardsOnPile;
	Cards pile[100];
} DagoPile;

typedef struct {
	TargetPile targetPiles[4];
	DagoPile dagoPiles[13];
} GameState;