#ifndef pong_included
#define pong_included

#define GREEN_LED BIT6

typedef struct MovLayer_s {
    Layer *layer;
    Vec2 velocity;
    struct MovLayer_s *next;
} MovLayer;

extern MovLayer pLU, pLD, pRU, pRD;
extern Region fieldFecne;
extern Layer pl, pr;

void movLayerDraw(MovLayer *movLayers, Layer *layers);
void mlAdvance(MovLayer *ml, Region *fence);
void paddleAdvance(MovLayer *ml, MovLayer *pLU, MovLayer *pRU, Region *fence);
void startup();
void score();
void moveC();

#endif
