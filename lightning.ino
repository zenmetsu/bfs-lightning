#include <SSD1320_OLED.h>

/* Initialize the display with the follow pin connections
   SSD1320 flexibleOLED(_cs, _rst, sclk, sd); */   
SSD1320 flexibleOLED(A5, A4, 24, 23);

/* display parameters */
const uint16_t rows = 32;
const uint16_t cols = 160;

/* lightning parameters */
uint8_t strobe = 13;        /* roughly how many bolt flashes will be drawn per strike */
uint8_t volts = 15;         /* brightness of strike, 0-15                             */
uint8_t elmo = 8;           /* brightness of pre-strike corona / pathwalking, 0-15    */
uint8_t vWallThresh = 200;  /* increases resistance to left/right travel, 0-255       */
uint8_t hWallThresh = 54;   /* increases resistance to vertical travel, 0-255         */
uint8_t maxIter = 255;      /* maximum iterations per attempt, 0-255                  */

/* global arrays */
uint8_t vis[rows][cols]; /* graphics buffer to push to display                        */
uint8_t pth[rows][cols]; /* array to hold backwalk path                               */
bool vWalls[rows][cols]; /* array to hold vertical walls                              */
bool hWalls[rows][cols]; /* array to hold horizontal walls                            */

/* graphical representation of what is habbening
 *
 *         x-1 x   x+1 x+2 x+3      current pos ( ) is at x,y
 *     y-2 O   O   O   O   O
 *         | 2   3 |                wall # due to vWalls[x][y] = 1;
 *     y-1 O - O   O - O   O
 *             #(4)    X            wall @ due to hWalls[x][y] = 1;
 *     y   O   O @ O   O   O
 *                                  wall X due to vWalls[x+2][y] = 1;
 *     y+1 O & O   O   O   O
 *                                  wall & due to hWalls[x-1][y+1] = 1; 
 *         O   O   O   O   O                         
 *                                  numbers represent breadth of visited cells
 *                                  zeros currently omitted for clarity
 */           

/*  current breadth = 4, next visited cells will be 5
 *  breadth values stored in vis[rows][cols] array
 *  
 *  
 *  current breadth is read in: vis[x][y] = 4 
 *  if current breadth is non-zero, the following takes place:
 *  
 *      UP is checked: vis[x][y-1] = 3 
 *      UP is non-zero, ignored
 *  
 *      LEFT is checked: vis[x-1][y] = 0 
 *      LEFT is blocked because vWalls[x][y] = 1;
 *  
 *      RIGHT is checked: vis[x+1][y] = 0
 *      RIGHT is open because vWalls[x+1][y] = 0;
 *      RIGHT is set to 5 : vis[x+1][y]=(breadth + 1)
 *  
 *      DOWN is checked: vis[x][y+1] = 0
 *      DOWN is blocked because hWalls[x][y] = 1;
 *  
 *      vis[rows][cols] is re-scanned and updated
 *        until [y] = height of display (bottom) 
 *        or maximum iterations is reached 
 */

/* global vars */
unsigned long lastMillis;
unsigned long frameCount;
float framesPerSecond;

uint8_t iter = 0;         /* for tracking iteration                                      */
uint8_t maxDepth = 0;     /* for tracking previous maximum downward distance             */
uint8_t maxBreadth = 0;   /* for tracking maximum breadth attained                       */
uint8_t lastChange = 0;   /* for tracking iteration count where last change was observed */



void setup()
{
  /* enable serial peripheral for debugging */
  Serial.begin(115200);
  
  /* initialize display */
  flexibleOLED.begin(cols, rows);

  /* randomize seed from analog */
  randomSeed(analogRead(A0) * analogRead(A1));
}



void loop()
{
  /* generate a new atmosphere and re-initialize global vars */
  generateMaze();

  /* perform walk until bottom or maxIter is reached */
  while ((maxDepth < cols-1) && (iter < maxIter))
  {
    iter++;
    walk();
    drawWalk();
  }

  /* if bottom was reached, backwalk and draw bolt */
  if (maxDepth >= cols-1)
  {
    backwalk();
    drawStrike();
  }
}



void generateMaze()
{
  /* local vars to hold col/row iteration counts */
  uint16_t i, j;

  /* reset global vars */
  iter=0;
  maxDepth=0;
  lastChange=0;
  maxBreadth=0;
  
  /* iterate through rows and columns to create walls and initialize arrays */
  for ( i=0; i<rows; ++i ) {
    for ( j=0; j<cols; ++j ) {
      (random(255) < vWallThresh)? vWalls[i][j] = 1 : vWalls[i][j] = 0;
      (random(255) < hWallThresh)? hWalls[i][j] = 1 : hWalls[i][j] = 0;
      vis[i][j]=0;
      pth[i][j]=0; 
    }
  }

  /* set breadth of starting point, middle of top row, to one */
  vis[rows/2][0]=1;
}



void drawWalk()
{
  /* iterate vis array over rows and columns, and draw contents */ 
  for (int irows = 0 ; irows < 32 ; irows++)
  {
    for (int icolumns = 0 ; icolumns < 80 ; icolumns++)
    {
      /* this is a 4bpp display, so each byte represents two pixels, (y,x) & (y,x+1) */
      byte sparks = elmo*(vis[irows][2*icolumns])/(2*maxBreadth) + 16*(elmo*(vis[irows][2*icolumns + 1])/(2*maxBreadth));
      /* draw the pixel pair */
      flexibleOLED.data(sparks);
    }
  }
}



void drawStrike()
{
  uint8_t flashCount;
  uint8_t mainFlash;
  uint8_t currentFlash;

  flashCount = 2 + random(strobe);
  mainFlash = random(flashCount);

  Serial.println(flashCount);

  for (int irows = 0 ; irows < 32 ; irows++)
  {
    for (int icolumns = 0 ; icolumns < 80 ; icolumns++)
    {
      byte sparks = (pth[irows][2*icolumns]) + 16*(pth[irows][2*icolumns + 1]);
      flexibleOLED.data(sparks);
    }
  }

  /* iterate for number of flashes */
  for (currentFlash = 0; currentFlash <= flashCount; currentFlash++)
  { 
    flexibleOLED.setContrast(0xFF);   
    (currentFlash == mainFlash)? delay(400 + random(1200)) : delay(random(60));
    flexibleOLED.setContrast(0x00);
    delay(random(70));
  }  
  flexibleOLED.setContrast(0xFF);
  flexibleOLED.clearDisplay(); 
}



void walk(){
  uint8_t breadth = 1;
  for (uint8_t horizon = 0; horizon < rows; horizon++)
  {
    for (uint8_t elevation = 0;elevation <cols;elevation++)
    {
      if (vis[horizon][elevation]>0)
      {
        breadth=vis[horizon][elevation];
        if (elevation > maxDepth) maxDepth = elevation;
        {
          if(horizon > 0)
          {
            if ((vWalls[horizon][elevation] == 0)&&(vis[horizon-1][elevation]==0))
            {
              vis[horizon-1][elevation] = breadth+1;
              lastChange=iter;
              if ((breadth + 1) > maxBreadth) maxBreadth = breadth+1;
            }         
          }
          if(horizon < rows)
          {
            if ((vWalls[horizon+1][elevation] == 0)&&(vis[horizon+1][elevation]==0))
            {
              vis[horizon+1][elevation] = breadth+1;
              lastChange=iter;
              if ((breadth + 1) > maxBreadth) maxBreadth = breadth+1;
            }
          }
          if(elevation > 0)
          {
            if ((hWalls[horizon][elevation-1] == 0)&&(vis[horizon][elevation-1]==0))
            {
              vis[horizon][elevation-1] = breadth+1;
              lastChange=iter;
              if ((breadth + 1) > maxBreadth) maxBreadth = breadth+1;
            }
          }
          if(elevation < cols)
          {
            if ((hWalls[horizon][elevation] == 0)&&(vis[horizon][elevation+1]==0))
            {
              vis[horizon][elevation+1] = breadth+1;
              lastChange=iter;
              if ((breadth + 1) > maxBreadth) maxBreadth = breadth+1;
            }
          }
        }
      }
    }
  }
  if ((lastChange + 2) < iter)
  {
    //Serial.println(iter);
    iter = maxIter;
  }
}



void backwalk(){
  /* initialize local vars */
  uint8_t breadth = 0;
  uint8_t localMin = 255;
  uint8_t strikePos = 0;
  uint8_t elevation = 159;
  uint8_t horizon;

  /* need to comment this code and optimize it */
  for (horizon = 0; horizon < rows; horizon++)
  {
    breadth = vis[horizon][elevation];
    if ((breadth > 0)&&(breadth < localMin))
    {
      strikePos = horizon;
      localMin = breadth;
    }    
  }

  horizon = strikePos;
  pth[horizon][elevation]=10;
  breadth=vis[horizon][elevation];
  
  while (breadth > 2)
  {
    if (horizon > 0)
    {
      if (vis[horizon-1][elevation]==breadth - 1)
      {
        horizon = horizon - 1;
        breadth = breadth - 1;
        pth[horizon-1][elevation] = volts;
      }
    }
    if(horizon < rows)
    {
      if (vis[horizon+1][elevation]==breadth - 1)
      {
        horizon = horizon + 1;
        breadth = breadth - 1;
        pth[horizon+1][elevation] = volts;
      }
    }
    if(elevation > 0)
    {
      if (vis[horizon][elevation-1]==breadth - 1)
      {
        elevation = elevation - 1;
        breadth = breadth - 1;
        pth[horizon][elevation-1] = volts;
      }
    }
    if(elevation < cols)
    {
      if (vis[horizon][elevation+1]==breadth - 1)
      {
        elevation = elevation + 1;
        breadth = breadth - 1;
        pth[horizon][elevation+1] = volts;
      }
    }
  }
}



void fps(int seconds){
  frameCount ++;
  if ((millis() - lastMillis) > (seconds * 1000)) {
    framesPerSecond = (float)frameCount / (float)seconds;
    Serial.print(framesPerSecond, 2);
    Serial.println(" FPS");
    frameCount = 0;
    lastMillis = millis();
  }
}
