//
//  Customview.m
//  gameTest
//
//  Created by syro Fullerton on 03/01/2025.
//

#include "CustomView.h"
#import <CoreText/CoreText.h>

@interface CustomView ()

@property NSInteger selection;
@property NSInteger oldSelection;
@property id trackingTouchIdentity;

@end

@implementation CustomView

bool isRunning = false;
unsigned long frameCounter = 0;

int tileValue       [23];
bool isTileFlagged  [23];
bool isTileVisible  [23];
int numTiles =       23;
int numMines = 6;
int tileIndex = -1;

bool didWin = false;

bool tapped = false;
bool isTappedTileMine = false;
bool released = false;

bool isSelected = false;

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)touchesBeganWithEvent:(NSEvent *)event
{
    
//    NSLog(@"tapped");
    tapped = true;
    // You're already tracking a touch, so this must be a new touch.
    // What should you do? Cancel or ignore.
    //
    if (self.trackingTouchIdentity == nil)
    {
        NSSet<NSTouch *> *touches = [event touchesMatchingPhase:NSTouchPhaseBegan inView:self];
        // Note: Touches may contain zero, one, or more touches.
        // What to do if there is more than one touch?
        // In this example, randomly pick a touch to track and ignore the other one.
        
        NSTouch *touch = touches.anyObject;
    
        if (touch != nil)
        {
            if (touch.type == NSTouchTypeDirect)
            {
                _trackingTouchIdentity = touch.identity;
                
                // Remember the selection value at the start of tracking in case you need to cancel.
                _oldSelection = self.selection;
                
                NSPoint location = [touch locationInView:self];
                self.fingerPos = location.x;
            }
        }
    }
    
    
    [super touchesBeganWithEvent:event];
}

- (void)touchesMovedWithEvent:(NSEvent *)event
{
    if (self.trackingTouchIdentity)
    {
        for (NSTouch *touch in [event touchesMatchingPhase:NSTouchPhaseMoved inView:self])
        {
            if (touch.type == NSTouchTypeDirect && [_trackingTouchIdentity isEqual:touch.identity])
            {
                NSPoint location = [touch locationInView:self];
                _fingerPos = location.x;
                break;
            }
        }
    }
    
    [super touchesMovedWithEvent:event];
}

- (void)touchesEndedWithEvent:(NSEvent *)event
{
    if (self.trackingTouchIdentity)
    {
        for (NSTouch *touch in [event touchesMatchingPhase:NSTouchPhaseEnded inView:self])
        {
            if (touch.type == NSTouchTypeDirect && [_trackingTouchIdentity isEqual:touch.identity])
            {
                // Finshed tracking successfully.
                _trackingTouchIdentity = nil;
                
                NSPoint location = [touch locationInView:self];
                _fingerPos = location.x;
                released = true;
                break;
            }
        }
    }

    [super touchesEndedWithEvent:event];
}

- (void)touchesCancelledWithEvent:(NSEvent *)event
{
    if (self.trackingTouchIdentity)
    {
        for (NSTouch *touch in [event touchesMatchingPhase:NSTouchPhaseMoved inView:self])
        {
            if (touch.type == NSTouchTypeDirect && [self.trackingTouchIdentity isEqual:touch.identity])
            {
                // CANCEL
                // This can happen for a number of reasons.
                // # A gesture recognizer started recognizing a touch.
                // # The underlying touch context changed (the user pressed Command-Tab while interacting with this view).
                // # The hardware canceled the touch.
                // Whatever the reason, put things back the way they were. In this example, reset the selection.
                //
                _trackingTouchIdentity = nil;
                
                self.selection = self.oldSelection;
                
                NSPoint location = [touch locationInView:self];
                _fingerPos = location.x;
            }
        }
    }
    
    
    [super touchesCancelledWithEvent:event];
}

int numCorrectFlags = 0;
- (void)initMines{
    
    didWin = false;
    numCorrectFlags = 0;
    
    for (int i = 0; i < numTiles; i++) {
        tileValue[i] = -1;
        isTileVisible[i] = false;
        isTileFlagged[i] = false;
    }
    
    int minePos = 0;
    int tempNumMines = numMines;
    for(int i = 0; i < tempNumMines; i++){
        
        minePos = rand()%numTiles;
        
        if (tileValue[minePos] == 0) {
            tempNumMines++;
        }else{
            tileValue[minePos] = 0; // mines
        }
    }
    
    
    for (int i = 0; i < (numTiles - 1); i++) {
        int tileVal = 0;
        if (tileValue[i+1] == 0) {
            if(tileValue[i] != 0){
                tileVal += 1;
                tileValue[i] = tileVal;
            }
        }
        
        if(i){
            if (tileValue[i-1] == 0) {
                if(tileValue[i] != 0){
                    tileVal += 1;
                    tileValue[i] = tileVal;
                }
            }
        }
        
        
    }
    
    
    
}

- (void)startAnimating { // init stuff
    self.timer = [NSTimer scheduledTimerWithTimeInterval:0.1
                                                 target:self
                                               selector:@selector(timerFired:)
                                               userInfo:nil
                                                repeats:YES];
    isRunning = true;

    [self initMines];
    
}

- (void)stopAnimating {
    [self.timer invalidate];
    self.timer = nil;
    [self setNeedsDisplay:YES]; // Force a redraw to clear the screen
}

- (void)timerFired:(NSTimer *)timer {
    frameCounter++;
    [self setNeedsDisplay:YES]; // Invalidate the current display and schedule a redraw
}

bool flagButtonPressed = false;
bool rstButtonPressed = false;
-   (void)drawBackground:(CGContextRef)context{
    
    
    NSRect rect = NSMakeRect(0, 0, 685, 30);
    
    CGContextSetRGBFillColor(context, .8f, .8f, .8f, 1.f);
    CGContextFillRect(context, rect);
    
    NSRect rect2 = NSMakeRect(2, 2, 685, 26);
    
    CGContextSetRGBFillColor(context, .6f, .6f, .6f, 1.f);
    CGContextFillRect(context, rect2);
    
    NSRect flagButton = NSMakeRect(685 - 4 - 22, 4, 22, 22);
    
    if(flagButtonPressed){
        CGContextSetRGBFillColor(context, .5f, 0.f, 0.f, 1.f);
    }else{
        CGContextSetRGBFillColor(context, 1.f, 0.f, 0.f, 1.f);
    }
    
    
    CGContextFillRect(context, flagButton);
    
    NSRect rstButton = NSMakeRect(685 - 4 - 22 - 30, 4, 22, 22);
    
    if(rstButtonPressed){
        CGContextSetRGBFillColor(context, .0f, .5f, 0.f, 1.f);
    }else{
        CGContextSetRGBFillColor(context, .0f, 1.f, 0.f, 1.f);
    }
    
    CGContextFillRect(context, rstButton);
    
    NSRect mineDisplay = NSMakeRect(685 - 4 - 80, 4, 24, 22);
    CGContextSetRGBFillColor(context, 0.1, 0.1, 0.1, 1.f);
    CGContextFillRect(context, mineDisplay);
    
    
    NSDictionary *attributes = @{
           NSFontAttributeName: [NSFont systemFontOfSize:20.0],
           NSForegroundColorAttributeName: [NSColor redColor]
       };

    NSString *text;
    text = [NSString stringWithFormat:@"%i",abs(numCorrectFlags - numMines)];
    [text drawAtPoint:NSMakePoint(685 - 4 - 23 - 50, 3) withAttributes:attributes];
       
}

-   (void)drawSkewmorphicTile:(CGContextRef)context atX:(int)pos isUp:(bool)isUp{
    
    CGMutablePathRef triPath = CGPathCreateMutable();
    CGPathMoveToPoint(triPath, NULL,4 + pos, 4);
    CGPathAddLineToPoint(triPath, NULL, 30 - 4 + pos, 30-4);
    CGPathAddLineToPoint(triPath, NULL, 30 - 4 + pos, 4);
    CGPathCloseSubpath(triPath);
    
    if(isUp){
        CGContextSetRGBFillColor(context, 0.7f, 0.7f, 0.7, 1.f);
    }else{
        CGContextSetRGBFillColor(context, .9f, .9f, .9f, 1.f);
    }
    CGContextAddPath(context, triPath);
    CGContextDrawPath(context, kCGPathFill);
    
    CGMutablePathRef rotPath = CGPathCreateMutable();
    
    CGPathMoveToPoint   (rotPath, NULL, 4 + pos, 30-4);
    CGPathAddLineToPoint(rotPath, NULL, 30 - 4 + pos, 30-4);
    CGPathAddLineToPoint(rotPath, NULL, 4 + pos, 4);
    CGPathCloseSubpath(rotPath);

    if(isUp){
        CGContextSetRGBFillColor(context, .9f, .9f, .9f, 1.f);
    }else{
        CGContextSetRGBFillColor(context, 0.7f, 0.7f, 0.7, 1.f);
    }
    CGContextAddPath(context, rotPath);
    
    CGContextDrawPath(context, kCGPathFill);
}

-   (void)draw1:(CGContextRef)context atX:(int)pos{
    
    [self drawSkewmorphicTile:context atX:pos isUp:false];
    
    NSRect inner = NSMakeRect(6 + pos, 6, 30 - 12, 30 - 12);
    CGContextSetRGBFillColor(context, 0.8f, 0.8f, 0.8, 1.f);
    CGContextFillRect(context, inner);
    
    NSDictionary *attributes = @{
           NSFontAttributeName: [NSFont systemFontOfSize:20.0],
           NSForegroundColorAttributeName: [NSColor blueColor]
       };

    NSString *text = @"1";
    
    [text drawAtPoint:NSMakePoint(pos + 10, 3) withAttributes:attributes];
       
}

-   (void)draw2:(CGContextRef)context atX:(int)pos{
    
    [self drawSkewmorphicTile:context atX:pos isUp:false];
    
    NSRect inner = NSMakeRect(6 + pos, 6, 30 - 12, 30 - 12);
    CGContextSetRGBFillColor(context, 0.75f, 0.75f, 0.75, 1.f);
    CGContextFillRect(context, inner);
    
    NSDictionary *attributes = @{
           NSFontAttributeName: [NSFont systemFontOfSize:20.0],
           NSForegroundColorAttributeName: [NSColor redColor]
       };

    NSString *text = @"2";
    
    [text drawAtPoint:NSMakePoint(pos + 10, 3) withAttributes:attributes];
}

-   (void)drawFlag:(CGContextRef)context atX:(int)pos{
    
    CGMutablePathRef flag = CGPathCreateMutable();
    NSRect pole = NSMakeRect(pos + 15 - 2, 4, 3, 22);
    
    CGPathMoveToPoint   (flag, NULL, pos + 15       , 26);
    CGPathAddLineToPoint(flag, NULL, pos + 15 - 10  , 20);
    CGPathAddLineToPoint(flag, NULL, pos + 15       , 14);
    CGPathCloseSubpath(flag);
    
    
    CGContextSetRGBFillColor(context, 1.f, 0.f, 0.f, 1.f);
    CGContextAddPath(context, flag);
    CGContextDrawPath(context, kCGPathFill);
    
    CGContextSetRGBFillColor(context, 0.f, 0.f, 0.f, 1.f);
    CGContextFillRect(context, pole);
}


-   (void)drawEmptyTile:(CGContextRef)context atX:(int)pos isSelected:(bool)isSel{

    [self drawSkewmorphicTile:context atX:pos isUp:!true];
    
    NSRect inner = NSMakeRect(6 + pos, 6, 30 - 12, 30 - 12);
    CGContextSetRGBFillColor(context, 0.8f, 0.8f, 0.8, 1.f);
    CGContextFillRect(context, inner);
    
}

-   (void)drawMine:(CGContextRef)context atX:(int)pos{
    
    NSRect bombBounds = NSMakeRect(pos + 7, 7, 15, 15);
    CGMutablePathRef bomb = CGPathCreateMutable();
    CGPathAddEllipseInRect(bomb, NULL, bombBounds);
    CGContextSetRGBFillColor(context, 0.f, 0.f, 0.f, 1.f);
    CGContextAddPath(context, bomb);
    CGContextDrawPath(context, kCGPathFill);
    
}

-   (void)  drawTile:(int)index
            context:(CGContextRef)context
           tileValue:(int)tileVal
       isTileVisible:(bool)isVis
          tappedTile:(int)tappedTile{
    
    if(!isVis){
        [self drawEmptyTile:context atX:(index * 26) isSelected:released];
    }else if(isTileFlagged[index]){
        [self drawFlag:context atX:(index * 26)];
    }else{
        switch (tileVal) {
            case 0:
                [self drawMine:context atX:(index * 26)];
                break;
                
            case 1:
                [self draw1:context atX:(index * 26)];
                break;
                
            case 2:
                [self draw2:context atX:(index * 26)];
                break;
                
                
            default:
                break;
        }
        
    }
    
}

-   (void)recurseUncover:(int)tileIndex{
    
    int tileIncr = tileIndex;
    int tileDecr = tileIndex;
    
    while(true){
        tileIncr ++;
        tileIncr = tileIncr % numTiles;
        
        if(tileValue[tileIncr] == -1){      isTileVisible[tileIncr] = true;}
        else if (tileValue[tileIncr] == 1){ isTileVisible[tileIncr] = true;}
        else{break;}
    }
    
    while(true){
        tileDecr --;
        tileDecr = tileDecr % numTiles;
        
        if(tileValue[tileDecr] == -1){      isTileVisible[tileDecr] = true;}
        else if (tileValue[tileDecr] == 1){ isTileVisible[tileDecr] = true;}
        else{break;}
    }
    
}

-   (void)viewMines{
    
    for (int i = 0; i < numTiles; i++) {
        isTileVisible[i] = true;
    }
    
}


-   (void)winScreen:(CGContextRef)context{
    
    NSRect faceBound = NSMakeRect((685.f / 2.f) - 2, 2, 26, 26);
    CGMutablePathRef face = CGPathCreateMutable();
    CGPathAddEllipseInRect(face, NULL, faceBound);
    
    CGContextSetRGBFillColor(context, 1.f, 1.f, 0.f, 1.f);
    CGContextAddPath(context, face);
    CGContextDrawPath(context, kCGPathFill);
    
    NSRect ELB = NSMakeRect((685.f / 2.f) + 3, 14, 6, 6);
    CGMutablePathRef EL = CGPathCreateMutable();
    CGPathAddEllipseInRect(EL, NULL, ELB);
    
    CGContextSetRGBFillColor(context, 0.f, 0.f, 0.f, 1.f);
    CGContextAddPath(context, EL);
    
    NSRect ERB = NSMakeRect((685.f / 2.f) + 13, 14, 6, 6);
    CGMutablePathRef ER = CGPathCreateMutable();
    CGPathAddEllipseInRect(ER, NULL, ERB);
    
    CGContextSetRGBFillColor(context, 0.f, 0.f, 0.f, 1.f);
    CGContextAddPath(context, ER);
    
    CGMutablePathRef smile = CGPathCreateMutable();
    CGContextDrawPath(context, kCGPathFill);
    
    CGPathAddArc(smile, NULL, (685.f / 2.f) + 11, 10, 5, 0, M_PI, true);
    CGContextSetRGBStrokeColor(context, 0.f, 0.f, 0.f, 1.f);
    CGContextAddPath(context, smile);
    CGContextSetLineWidth(context, 2.);
    CGContextDrawPath(context, kCGPathStroke);

    
    
}

bool flipFlop = true;
-   (void)gamePlay:(CGContextRef)context{
    
    if (tapped) {
        released = false;
        tileIndex = (self.fingerPos / ( (26 * numTiles) + 4) ) * numTiles;
        
        if(tileIndex == 25){
            
            flagButtonPressed = !flagButtonPressed;
            tapped = false;
            return;
        }

        
        if(tileIndex == 24){
            
            rstButtonPressed = false;
            [self initMines];
            tapped = false;
            return;
        }
        
        numCorrectFlags = 0;
        for (int i = 0; i < numTiles; i++) {
            if( isTileFlagged[i] && (tileValue[i] == 0)){
                numCorrectFlags ++;
            }
        }
        if(numCorrectFlags == numMines){
            didWin = true;
        }
        
        
        switch (tileValue[tileIndex]) {
            case -1:
                isTileVisible[tileIndex] = true;
                
                [self recurseUncover:tileIndex];
                
                break;
                
            case 1:
                isTileVisible[tileIndex] = true;
                break;
                
            case 2:
                isTileVisible[tileIndex] = true;
                break;
                
            case 0:
                if(flagButtonPressed){
                    isTileFlagged[tileIndex] = true;
                    isTileVisible[tileIndex] = true;
                    
                }else{
                    [self viewMines];
                }
                
            default:
                break;
        }
        
        
        isTappedTileMine = (tileValue[tileIndex] == 0);
        
        tapped = false;
    }
    
    
}

-   (void)update{
    
    numCorrectFlags = 0;
    for (int i = 0; i < numTiles; i++) {
        if( isTileFlagged[i] && (tileValue[i] == 0)){
            numCorrectFlags ++;
        }
    }
    if(numCorrectFlags == numMines){
        didWin = true;
    }
    
    isSelected =  (tapped && !released);
    isSelected ? NSLog(@"selected") : NULL;
    
}

    - (void)drawRect:(NSRect)dirtyRect{
        
        if(!isRunning){
            [self startAnimating];
        }
        
        CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
        
        [self update];
        
        
        [self gamePlay:context];
        [self drawBackground:context];
        for (int i = 0; i < numTiles; i++) {
            [self drawTile:i context:context tileValue:tileValue[i] isTileVisible:isTileVisible[i] tappedTile:tileIndex];
        }
        
        if(didWin){
            [self viewMines];
            [self winScreen:context];
        }
//        rectTest(context); 
        
        isTappedTileMine = false;
        
    }





@end


