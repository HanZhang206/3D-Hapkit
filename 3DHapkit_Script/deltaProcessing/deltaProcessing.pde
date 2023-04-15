import processing.serial.*;
Serial myPort;         
int valr;            

 
 // Press 'w' to start wiggling, space to restore
// original positions.

int zWallThick = 10;
int[][] nCirleCenter = new int[2][2];  
float dWllPos = 130;
float cubeSize = 320;
int circleRad = 20; 
int circleBig = 20;  
int circleRes = 40;
float noiseMag = 1;
float x,y,z;
int[] serialInArray = new int[14];    // Where we'll put what we receive
int serialCount = 0;                 // A count of how many bytes we receive
int xpos, ypos,zpos;                 // Starting position of the ball
boolean firstContact = false;        // Whether we've heard from the microcontroller
boolean locked = false;  
float xOffset = 0.0; 
float yOffset = 0.0; 
float bx;
float by;
boolean wiggling = false;

int[] origin = new int[3];
PShape face2; 
PShape planet2;
PImage surftex2;
PShape cube;

void setup() {

  size(1424, 800, P3D);    
  myPort = new Serial(this, "com5", 115200);
  nCirleCenter[0][0] = -20; 
  nCirleCenter[0][1] = -20; 
  nCirleCenter[1][0] = 20; 
  nCirleCenter[1][1] = 20; 
  
  createCube();  
  stroke(0,255,0);
  planet2 = createShape(SPHERE, circleRad/2); 
  x = y = z = 0;
  //face2 = createShape();   
  //createFaceWithHole2(face2);
}

void draw() {
  
  background(0);
  translate(width/2, height/2);
 
  rotateX(50 * 0.01f);
  rotateY(-10 * 0.01f);
  
  rotateY(xOffset/180*3.1415);
  rotateX(yOffset/180*3.1415);
  
   noStroke();
   //sphereDetail(10);
   
   stroke(255, 0, 0);
   
  origin[0] = -170; 
  origin[1] = -170;
  origin[2] = -170;
  
  int lineLength = 450;
   
   line( origin[0],  origin[1],  origin[2], origin[0]+lineLength, origin[1],  origin[2]);
   line( origin[0],  origin[1],  origin[2], origin[0], origin[1]+lineLength,  origin[2]);
   line( origin[0],  origin[1],  origin[2], origin[0],  origin[1], origin[2]+lineLength);
   
   stroke(0, 255, 0);
   textSize(36);
   text("X",origin[0]+lineLength, origin[1],  origin[2]); //<>//
   text("Y", origin[0], origin[1]+lineLength,  origin[2]);
   text("Z",  origin[2],origin[1], origin[2]+lineLength);
 
 
  //shape(face2);
  shape(cube);
  
  fill(255, 0, 0); 
 
  pushMatrix();
 // rotateY(PI * frameCount / 500);
  translate(x,y,z);  
  shape(planet2);
 // planet2.stroke(255,255, 0);
  popMatrix();  
}

int dCCCC = (circleRad+circleRad/2)*(circleRad+circleRad/2); 

int getDisCircle2Circle(float x,float y)
{
    for(int i = 0; i < 2; i++)
    {        
      float distace =  (x-nCirleCenter[i][0])*(x-nCirleCenter[i][0])+ (y-nCirleCenter[i][1])*(y-nCirleCenter[i][1]);
      if( distace <  (circleRad/2)*(circleRad/2))
       {
           return int(sqrt(distace));
       }
    }
    return 1000;
}

void serialEvent(Serial myPort) {
  // read a byte from the serial port:
  int inByte = myPort.read();
  // if this is the first byte received, and it's an A,
  // clear the serial buffer and note that you've
  // had first contact from the microcontroller. 
  // Otherwise, add the incoming byte to the array:
  if (firstContact == false) {
    if (inByte == 'A') { 
      myPort.clear();          // clear the serial port buffer
      firstContact = true;     // you've had first contact from the microcontroller
      myPort.write('A');       // ask for more
    } 
  } 
  else 
  {
    // Add the latest byte from the serial port to array:
    serialInArray[serialCount] = inByte;
    serialCount++;
   
    // If we have 12 bytes:
    if (serialCount > 13 ) {
      xpos=ypos=zpos=0;
      xpos = serialInArray[0]; xpos |= (serialInArray[1]<<8); xpos |= (serialInArray[2]<<16); xpos |= (serialInArray[3]<<24);
      ypos = serialInArray[4]; ypos |= (serialInArray[5]<<8); ypos |= (serialInArray[6]<<16); ypos |= (serialInArray[7]<<24);
      zpos = serialInArray[8]; zpos |= (serialInArray[9]<<8); zpos |= (serialInArray[10]<<16);zpos |= (serialInArray[11]<<24);          
      //x = -x;
      x = xpos/10.0;
      y = ypos/10.0;
      z = zpos/10.0;
      if(serialInArray[12] == 0) x = -x;
      if(serialInArray[13] == 0) y = -y;
      
      println(x + "\t" + y + "\t" + z);
      
      int distanceCC = getDisCircle2Circle(x,y);
      int bInCircle = 0;              
      if(distanceCC <= dCCCC) bInCircle = 1; 
      if(bInCircle == 0)
      {
         if(z <= dWllPos+circleRad/2 &&  z >= dWllPos+zWallThick-circleRad/2)
         // if(z <= 140 &&  z >= 125)
          {
                z = dWllPos+circleRad/2;
          }
          else if(z >= dWllPos-zWallThick-circleRad/2 &&  z <= dWllPos-circleRad/2)
          //else if(z >= 110 &&  z <= 125)
          {
                z = dWllPos-zWallThick-circleRad/2;
          }
      } 
      //println(bInCircle + "\t" + z + "\t" );
     
      // print the values (for debugging purposes only):
     // println(x + "\t" + y + "\t" + z);

      // Send a capital A to request new sensor readings:
      myPort.write('A');
      // Reset serialCount:
      serialCount = 0;
    }
  }
}

void createFaceWithHole2(PShape face) {
  face.beginShape(POLYGON);
  face.stroke(0, 0, 255);
  face.fill(125);

  // Draw main shape Clockwise
  face.vertex(-cubeSize/2, -cubeSize/2, dWllPos);
  face.vertex(+cubeSize/2, -cubeSize/2, dWllPos);
  face.vertex(+cubeSize/2, +cubeSize/2, dWllPos);
  face.vertex(-cubeSize / 2, +cubeSize / 2, dWllPos);

  // Draw contour (hole) Counter-Clockwise
  face.beginContour();
  for (int i = 0; i < circleRes; i++) {
    float angle = TWO_PI * i / circleRes;
    float x = nCirleCenter[0][0]+circleRad * sin(angle);
    float y = nCirleCenter[0][1]+circleRad * cos(angle);
    float z = dWllPos;
    face.vertex(x, y, z);
  }
  face.endContour();
  
  face.beginContour();
  for (int i = 0; i < circleRes; i++) {
    float angle = TWO_PI * i / circleRes;
    float x = nCirleCenter[1][0]+circleRad * sin(angle);
    float y = nCirleCenter[1][0]+circleRad * cos(angle);
    float z = dWllPos;
    face.vertex(x, y, z);
  }
  face.endContour();
  
  face.endShape(CLOSE);
}
 
void createCube() 
{  
  cube = createShape(GROUP);  
  PShape face;
  // Create all faces at front position
  for (int i = 0; i < 6; i++) {
    face = createShape();
    createFaceWithHole(face,i);
    cube.addChild(face);
  }

  // Rotate all the faces to their positions

  // Front face - already correct
  face = cube.getChild(0);

  // Back face
  face = cube.getChild(1); 
  face.translate(0,0,-zWallThick); 
  
  // Right face
  face = cube.getChild(2);  
  face.translate(0,0,0);
  
  // Left face
  face = cube.getChild(3);   
  face.translate(cubeSize,0,0);
  
  // Top face
  face = cube.getChild(4);
  
  // Bottom face
  face = cube.getChild(5);
  face.translate(0,cubeSize,0);
}

void createFaceWithHole(PShape face,int nIndex)
{
  face.beginShape(POLYGON); 
  face.stroke(0, 0, 255);
  face.fill(125);
  
  if(nIndex == 0 || nIndex == 1)
  {
      face.vertex(-cubeSize/2, -cubeSize/2, dWllPos);
      face.vertex(+cubeSize/2, -cubeSize/2, dWllPos);
      face.vertex(+cubeSize/2, +cubeSize/2, dWllPos);
      face.vertex(-cubeSize / 2, +cubeSize / 2, dWllPos);
    
      // Draw contour (hole) Counter-Clockwise
      face.beginContour();
      
      for (int i = 0; i < circleRes; i++) {
        float angle = TWO_PI * i / circleRes;
        float x = nCirleCenter[0][0]+circleBig * sin(angle);
        float y = nCirleCenter[0][1]+circleBig * cos(angle);
        float z = dWllPos;
        face.vertex(x, y, z);
      }
      face.endContour();
      
      face.beginContour();
      for (int i = 0; i < circleRes; i++) {
        float angle = TWO_PI * i / circleRes;
        float x = nCirleCenter[1][0]+circleBig * sin(angle);
        float y = nCirleCenter[1][1]+circleBig * cos(angle);
        float z = dWllPos;
        face.vertex(x, y, z);
      }
      face.endContour();
  } 
  else if(nIndex == 2 || nIndex == 3 )
  {
      face.vertex(-cubeSize/2, -cubeSize/2, dWllPos); //1 
      face.vertex(-cubeSize / 2, +cubeSize / 2, dWllPos);  //4    
      face.vertex(-cubeSize / 2, +cubeSize / 2, dWllPos-zWallThick); //3
      face.vertex(-cubeSize/2, -cubeSize/2, dWllPos-zWallThick); //2      
  } 
   else if(nIndex == 4 || nIndex == 5)
  {
      face.vertex(-cubeSize/2, -cubeSize/2, dWllPos);
      face.vertex(+cubeSize/2, -cubeSize/2, dWllPos);
      face.vertex(+cubeSize/2, -cubeSize/2, dWllPos-zWallThick);//4
      face.vertex(-cubeSize/2, -cubeSize/2, dWllPos-zWallThick); //2
  } 
  face.endShape(CLOSE);
}

void mousePressed() {
  locked = true;
  bx = mouseX; 
  by = mouseY; 

}

void mouseDragged() {
   
    if(locked == true)
    {
      xOffset = mouseX-bx; 
      yOffset = mouseY-by;       
     // println(xOffset + "\t" + yOffset);
    }  
}

void mouseReleased() {
  locked = false;
}
