#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6

/*SHAPE DEFINITION SECTION {width, height}  */
AbRect ball = {abRectGetBounds, abRectCheck, {4,4}};
AbRect pL = {abRectGetBounds, abRectCheck, {1,11}};
AbRect pR = {abRectGetBounds, abRectCheck, {1,11}};

/*OUTLINE DEFINITION SECTION*/
AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2-1, screenHeight/2-1}
};

/*LAYER DEFINITION SECTION*/
Layer pl = { //Left Paddle
  (AbShape *)&pL,
  {5, (screenHeight/2)}, 
  {0,0}, {0,0},		
  COLOR_WHITE,
  0
};
  
Layer pr = { //Right Paddle 
  (AbShape *)&pR,
  {screenWidth-7, (screenHeight/2)},
  {0,0}, {0,0},	      
  COLOR_WHITE,
  &pl,
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2-1, screenHeight/2},          /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  &pr
};

Layer layer0 = { //Ball
  (AbShape *)&ball,
  {(screenWidth/2), (screenHeight/2)}, 
  {0,0}, {0,0},	  
  COLOR_WHITE,
  &fieldLayer,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;


/*LEFT PADDLE UP*/
MovLayer pLU = {&pl, {0,-1}, 0};
/*LEFT PADDLE DOWN*/
MovLayer pLD = {&pl, {0,1}, 0};
/*RIGHT PADDLE UP*/
MovLayer pRU = {&pr, {0,1}, 0};
/*RIGHT PADDLE DOWN*/
MovLayer pRD = {&pr, {0,-1}, 0};
/*BALL*/
MovLayer ml0 = {&layer0, {-1,0}, 0};

/*REGION DEFINITIONS*/
Region fieldFence;		/**< fence around playing field  */
Region pLFence;
Region pRFence;


void movLayerDraw(MovLayer *movLayers, Layer *layers) {
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence) {
  Vec2 newPos;
  Vec2 paddleL;
  Vec2 paddleR;
  u_char axis;
  int velocity;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity); // fence
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if (shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) {
	newPos.axes[axis] += screenHeight-23;
      }
      if (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) {
	newPos.axes[axis] -= screenHeight-23;
      }
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}

void paddleAdvance(MovLayer *ml, MovLayer *pLU, MovLayer *pRU, Region *fence) {
  Vec2 newPos;
  Vec2 paddleL;
  Vec2 paddleR;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity); // BALL
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    
    vec2Add(&paddleL, &pLU->layer->posNext, &pLU->velocity); //Paddle left
    abShapeGetBounds(pLU->layer->abShape, &paddleL, &pLFence);

    vec2Add(&paddleR, &pRU->layer->posNext, &pRU->velocity); //Paddle right
    abShapeGetBounds(pRU->layer->abShape, &paddleR, &pRFence);
    
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	
      }	/**< if outside of fence */
      /*PADDLE LEFT COLISION*/
      if ((shapeBoundary.topLeft.axes[0] <  pLFence.botRight.axes[0]) &&
      	  (shapeBoundary.botRight.axes[1]-10 < pLFence.botRight.axes[1]) &&
      	  (shapeBoundary.topLeft.axes[1]+10 > pLFence.topLeft.axes[1])) {
      	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
      	newPos.axes[axis] += (2*velocity);
      }
      /*PADDLE RIGHT COLISION*/
      if ((shapeBoundary.botRight.axes[0] >  pRFence.topLeft.axes[0]) &&
      	  (shapeBoundary.botRight.axes[1]-10 < pRFence.botRight.axes[1]) &&
      	  (shapeBoundary.topLeft.axes[1]+10 > pRFence.topLeft.axes[1])) {
      	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
      	newPos.axes[axis] += (2*velocity);
      }
    } /*< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}


u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */




/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen */
void main() {
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(1);
  shapeInit();

  /*WELCOME SCREEN*/
  clearScreen(COLOR_BLACK);
  drawString5x7(screenWidth/2,screenHeight/2, "PONG", COLOR_WHITE, COLOR_BLACK);
  __delay_cycles(80000000);
  clearScreen(COLOR_BLACK);
  
  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
    movPaddle();
   }
}

/*THIS METHOD DETECTS IF A BUTTON IS PRESSED*/
void movPaddle() {
  int sw1, sw2, sw3, sw4;
  sw1 = (P2IN & BIT0)? 0 : 1;
  sw2 = (P2IN & BIT1)? 0 : 1;
  sw3 = (P2IN & BIT2)? 0 : 1;
  sw4 = (P2IN & BIT3)? 0 : 1;
  if (sw1) {
    movLayerDraw(&pLU, &pl);
    mlAdvance(&pLU,&fieldFence);
  }
  if (sw2) {
    movLayerDraw(&pLD, &pl);
    mlAdvance(&pLD,&fieldFence);
  }
  if (sw3) {
    movLayerDraw(&pRU, &pr);
    mlAdvance(&pRU,&fieldFence);
  }
  if (sw4) {
    movLayerDraw(&pRD, &pr);
    mlAdvance(&pRD,&fieldFence);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 5) {
    paddleAdvance(&ml0,&pLD,&pRD,&fieldFence);
    redrawScreen = 1;
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
