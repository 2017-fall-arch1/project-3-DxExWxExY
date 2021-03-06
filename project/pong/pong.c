#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"
#include "move.h"
#include "pong.h"

static char sl = '0';
static char sr = '0';
static int s1 = 0;
static int s2 = 0;

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
    {4, (screenHeight/2)}, 
    {0,0}, {0,0},		
    COLOR_WHITE,
    0
};

Layer pr = { //Right Paddle 
    (AbShape *)&pR,
    {screenWidth-6, (screenHeight/2)},
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


/*LEFT PADDLE UP*/
MovLayer pLU = {&pl, {0,-4}, 0};
/*LEFT PADDLE DOWN*/
MovLayer pLD = {&pl, {0,4}, 0};
/*RIGHT PADDLE UP*/
MovLayer pRU = {&pr, {0,4}, 0};
/*RIGHT PADDLE DOWN*/
MovLayer pRD = {&pr, {0,-4}, 0};
/*BALL*/
MovLayer ml0 = {&layer0, {1,1}, 0};

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
                newPos.axes[axis] -= -4;
            }
            if (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) {
                newPos.axes[axis] += -3;
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
                (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])) {
                if (shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]) {
                    s2++;
                    buzzer_set_period(1000);
                }
                if (shapeBoundary.botRight.axes[0] > fence->botRight.axes[0]) {
                    s1++;
                    buzzer_set_period(1000);
                }
                int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
            newPos.axes[axis] += (2*velocity);
            break;
            
            
            
                }	/**< if outside of fence */
                /*PADDLE LEFT COLISION*/
                if ((shapeBoundary.topLeft.axes[0] <  pLFence.botRight.axes[0]) &&
                    (shapeBoundary.botRight.axes[1]-10 < pLFence.botRight.axes[1]) &&
                    (shapeBoundary.topLeft.axes[1]+10 > pLFence.topLeft.axes[1])) {
                    int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
                newPos.axes[axis] += (2*velocity);
                buzzer_set_period(4000);
                break;
                
                    }
                    /*PADDLE RIGHT COLISION*/
                    if ((shapeBoundary.botRight.axes[0] >  pRFence.topLeft.axes[0]) &&
                        (shapeBoundary.botRight.axes[1]-10 < pRFence.botRight.axes[1]) &&
                        (shapeBoundary.topLeft.axes[1]+10 > pRFence.topLeft.axes[1])) {
                        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
                    newPos.axes[axis] += (2*velocity);
                    buzzer_set_period(4000);
                    break;
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
    buzzer_init();
    
    startup();
    
    
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
	move(); //Assembly Function
	//moveC(); //C funtion
        score();
        __delay_cycles(200000);
        buzzer_set_period(0);
    }
}

/*STARTUP SCREEN WITH DIFICULTY OPTIONS*/
void startup() {
    clearScreen(COLOR_BLACK);
    drawString5x7(screenWidth/2 -11,screenHeight/2 -40, "PONG", COLOR_WHITE, COLOR_BLACK);
    drawString5x7(6,screenHeight/2 +40, "E:S1 M:S2 H:S3 X:S4", COLOR_WHITE, COLOR_BLACK);
    int dir1, dir2;
    dir1 = -1;
    dir2 = 1;
//     int random = rand();
//     if (random % 2 == 0) {
//         dir1 = 1;
//     }
//     else {
//         dir1 = 1;
//     }
//     random = rand();
//     if (random % 2 == 0) {
//         dir2 = 1;
//     }
//     else {
//         dir2 = 1;
//     }
    while (1) {
        if (!(P2IN & BIT0)) {
            ml0.velocity.axes[0] = 1*dir1;
            ml0.velocity.axes[1] = 1*dir2;
            goto out;
        }
        if (!(P2IN & BIT1)) {
            ml0.velocity.axes[0] = 2*dir1;
            ml0.velocity.axes[1] = 2*dir2;
            goto out;
        }
        if (!(P2IN & BIT2)) {
            ml0.velocity.axes[0] = 3*dir1;
            ml0.velocity.axes[1] = 3*dir2;
            goto out;
        }
        if (!(P2IN & BIT3)) {
            ml0.velocity.axes[0] = 4*dir1;
            ml0.velocity.axes[1] = 4*dir2;
            goto out;
        }
        if ((!(P2IN & BIT0)) && (!(P2IN & BIT3))) {
            WDTCTL = 0;  
        }   
    }
    out: ;
    clearScreen(COLOR_BLACK);
}

/*METHOD DISPLAYS THE SCORE OF THE PLAYERS
 * BY "CONVERTING" A INT TO CHAR*/
void score() {
    switch (s1) {
        case 1:
            sl = '1';
            break;
        case 2:
            sl = '2';
            break;
        case 3:
            sl = '3';
            break;
        case 4:
            sl = '4';
            break;
        case 5:
            sl = '5';
            break;
        case 6:
            sl = '6';
            break;
        case 7:
            sl = '7';
            break;
        case 8:
            sl = '8';
            break;
        case 9:
            sl = '9';
            break;
        case 0:
            sl = '0';
            break;
        default:
            drawString5x7(30, screenHeight/2, "PLAYER 1 WON", COLOR_WHITE, COLOR_BLACK);
            win();
            WDTCTL = 0; 
            break;
    }
    switch (s2) {
        case 1:
            sr = '1';
            break;
        case 2:
            sr = '2';
            break;
        case 3:
            sr = '3';
            break;
        case 4:
            sr = '4';
            break;
        case 5:
            sr = '5';
            break;
        case 6:
            sr = '6';
            break;
        case 7:
            sr = '7';
            break;
        case 8:
            sr = '8';
            break;
        case 9:
            sr = '9';
            break;
        case 0:
            sr = '0';
            break;
        default:
            drawString5x7(30, screenHeight/2, "PLAYER 2 WON", COLOR_WHITE, COLOR_BLACK);
            win();
            WDTCTL = 0; 
            break;
    }
    drawChar5x7(screenWidth/2 -30, 10, sl, COLOR_WHITE, COLOR_BLACK);
    drawChar5x7(screenWidth/2 +30, 10, sr, COLOR_WHITE, COLOR_BLACK);
}

/*AUXILIARY METHOD FOR THE ASSEMBLY FUNCTION*/
void lup() {
    movLayerDraw(&pLU, &pl);
    mlAdvance(&pLU,&fieldFence);
}
void ldw () {
    movLayerDraw(&pLD, &pl);
    mlAdvance(&pLD,&fieldFence);
}
void rup () {
    movLayerDraw(&pRU, &pr);
    mlAdvance(&pRU,&fieldFence);
}
void rdw () {
    movLayerDraw(&pRD, &pr);
    mlAdvance(&pRD,&fieldFence);
}
/*THIS METHOD DETECTS IF A BUTTON IS PRESSED
 TO MOVE A PADDLE UP OR DOWN RESPECTIVELY*/
void moveC() {
    if (!(P2IN & BIT0)) {
      movLayerDraw(&pLU, &pl);
      mlAdvance(&pLU,&fieldFence);
    }
    if (!(P2IN & BIT1)) {
        movLayerDraw(&pLD, &pl);
        mlAdvance(&pLD,&fieldFence);
    }
    if (!(P2IN & BIT2)) {
        movLayerDraw(&pRU, &pr);
        mlAdvance(&pRU,&fieldFence);
    }
    if (!(P2IN & BIT3)) {
        movLayerDraw(&pRD, &pr);
        mlAdvance(&pRD,&fieldFence);
    }
    if ((!(P2IN & BIT0)) && (!(P2IN & BIT2)) && (!(P2IN & BIT3))) {
        WDTCTL = 0;  
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
