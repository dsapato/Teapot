#define GLEW_STATIC

#include <stdlib.h>
#include <math.h>
#include <glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include "SOIL.h"
#include "objParser.h"

#define NUM_VERTS 1202
#define NUM_FACES 2256

#define numKeysDefined 6
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define Qkey 4
#define Ekey 5
bool * keysDown;

float sealevel;

int res = 257;
glm::vec3 moveSpeed = glm::vec3(.0003f);

#define ADDR(i,j,k) (3*((j)*res + (i)) + (k))

//Manage orientation and location
glm::vec3 position;
glm::vec3 speed;
glm::vec3 direction;
glm::vec3 up;
glm::vec3 rightWing;
float angularAccel;

//Object Data
GLfloat *verts = 0;
GLfloat *norms = 0;
GLuint *faces = 0;

//Textures
GLuint * _skybox;
GLuint _grass;
GLuint _teapot;
GLuint _sphere;

bool sphereMap;

//Reads in file for shaders
std::string filetext;
std::string* loadFileToString(char const * const fname)
{
    std::ifstream ifile(fname);

    while( ifile.good() ) {
        std::string line;
        std::getline(ifile, line);
        filetext.append(line + "\n");
    }

    return &filetext;
}

/*
Tastefully borrowed from
http://zach.in.tu-clausthal.de/teaching/cg2_07/literatur/glsl_tutorial/index.html
for your convenience
*/
void printShaderInfoLog(GLuint obj)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
		free(infoLog);
	}
}

//Program/Shader Code
GLuint skyProgram;
GLuint grassProgram;
GLuint teapotProgram;

void init(void)
{
    //Sky Shader and program
	GLuint skyvs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(skyvs, 1, (const GLchar **) (loadFileToString("data/sky_vs")), NULL);
	glCompileShader(skyvs);
	printShaderInfoLog(skyvs);

	GLuint skyfs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(skyfs, 1, (const GLchar **) (loadFileToString("data/sky_fs")), NULL);
	glCompileShader(skyfs);
	printShaderInfoLog(skyfs);

	skyProgram = glCreateProgram();
	glAttachShader(skyProgram, skyvs);
	glAttachShader(skyProgram, skyfs);
	glLinkProgram(skyProgram);

	//Grass Shader and program
	GLuint grassvs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(grassvs, 1, (const GLchar **) (loadFileToString("data/grass_vs")), NULL);
	glCompileShader(grassvs);
	printShaderInfoLog(grassvs);

	GLuint grassfs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(grassfs, 1, (const GLchar **) (loadFileToString("data/grass_fs")), NULL);
	glCompileShader(grassfs);
	printShaderInfoLog(grassfs);

	grassProgram = glCreateProgram();
	glAttachShader(grassProgram, grassvs);
	glAttachShader(grassProgram, grassfs);
	glLinkProgram(grassProgram);

	//Teapot Shader and program
	GLuint teapotvs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(teapotvs, 1, (const GLchar **) (loadFileToString("data/grass_vs")), NULL);
	glCompileShader(teapotvs);
	printShaderInfoLog(teapotvs);

	GLuint teapotfs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(teapotfs, 1, (const GLchar **) (loadFileToString("data/sky_fs")), NULL);
	glCompileShader(teapotfs);
	printShaderInfoLog(teapotfs);

	teapotProgram = glCreateProgram();
	glAttachShader(teapotProgram, teapotvs);
	glAttachShader(teapotProgram, teapotfs);
	glLinkProgram(teapotProgram);

	GLfloat amb[] = {0.2,0.2,0.2};
	GLfloat diff[] = {1.0,1.0,1.0};
	GLfloat spec[] = {1.0,1.0,1.0};

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

	glClearColor (0.0, 0.75, 1.0, 0.0);	// sky
	glEnable(GL_DEPTH_TEST);

	sealevel = .25;

    keysDown = new bool[numKeysDefined];
    for(int i = 0; i < numKeysDefined; i++){
        keysDown[i] = false;
    }

    position.x = -5;
    position.y = 1;
    position.z = 1;

    direction.x = 1;
    direction.y = 0;
    direction.z = 0;

    up.x = 0;
    up.y = 0;
    up.z = 1;

    rightWing.x = 0;
    rightWing.y = -1;
    rightWing.z = 0;

    speed = glm::vec3(0.0f,0.0f,0.0f);
    angularAccel = 0.0f;

    //Create teapot
	verts = (GLfloat*)malloc(NUM_VERTS*3*sizeof(GLfloat));
	norms = (GLfloat*)malloc(NUM_VERTS*3*sizeof(GLfloat));
	faces = (GLuint*)malloc(NUM_FACES*3*sizeof(GLuint));
    objParser::parse(verts,norms,faces);

    //Use soil library to import sky textures
    _skybox = (GLuint*)(malloc(6*sizeof(GLuint)));
    for(int i = 0; i < 6; i++){


        switch(i){
        case 0: _skybox[i] = SOIL_load_OGL_texture(
                    "data/side4.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    ); break;
        case 1: _skybox[i] = SOIL_load_OGL_texture(
                    "data/side3.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    ); break;
        case 2: _skybox[i] = SOIL_load_OGL_texture(
                    "data/side2.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    ); break;
        case 3: _skybox[i] = SOIL_load_OGL_texture(
                    "data/side1.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    ); break;
        case 4: _skybox[i] = SOIL_load_OGL_texture(
                    "data/top.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    ); break;
        case 5: _skybox[i] = SOIL_load_OGL_texture(
                    "data/bottom.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    ); break;

        }


        if( 0 == _skybox[i] )
            {
                printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
            }

        //glGenTextures(1, &_skybox[i]);
    }

    //Get grass texture
    _grass = SOIL_load_OGL_texture(
                    "data/grass.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    );
    if(!_grass){
        std::cout << "failed to load grass" << std::endl;
    }

    //teapot texture
    _teapot = SOIL_load_OGL_texture(
                    "data/teapot.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    );
    if(!_teapot){
        std::cout << "failed to load teapot" << std::endl;
    }

    sphereMap = false;
    //sphereMap texture
    _sphere = SOIL_load_OGL_texture(
                    "data/spheremap.png",
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                    );
    if(!_sphere){
        std::cout << "failed to load spheremap" << std::endl;
    }
}


void drawSkybox(){
// Store the current matrix
   glPushMatrix();
   // Reset and transform the matrix.
   glLoadIdentity();
   gluLookAt(
       0,0,0,
       direction.x,direction.z,-direction.y,
       up.x,up.z,-up.y);

   // Enable/Disable features
   glPushAttrib(GL_ENABLE_BIT);
   glEnable(GL_TEXTURE_2D);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_LIGHTING);
   glDisable(GL_BLEND);

   // Just in case we set all vertices to white.
   glColor4f(1,1,1,1);
   // Render the front quad
   glBindTexture(GL_TEXTURE_2D, _skybox[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glBegin(GL_QUADS);
       glTexCoord2f(0, 0); glVertex3f(  0.5f, -0.5f, -0.5f );
       glTexCoord2f(1, 0); glVertex3f( -0.5f, -0.5f, -0.5f );
       glTexCoord2f(1, 1); glVertex3f( -0.5f,  0.5f, -0.5f );
       glTexCoord2f(0, 1); glVertex3f(  0.5f,  0.5f, -0.5f );
   glEnd();
   // Render the left quad
   glBindTexture(GL_TEXTURE_2D, _skybox[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glBegin(GL_QUADS);
       glTexCoord2f(0, 0); glVertex3f(  0.5f, -0.5f,  0.5f );
       glTexCoord2f(1, 0); glVertex3f(  0.5f, -0.5f, -0.5f );
       glTexCoord2f(1, 1); glVertex3f(  0.5f,  0.5f, -0.5f );
       glTexCoord2f(0, 1); glVertex3f(  0.5f,  0.5f,  0.5f );
   glEnd();
   // Render the back quad
   glBindTexture(GL_TEXTURE_2D, _skybox[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glBegin(GL_QUADS);
       glTexCoord2f(0, 0); glVertex3f( -0.5f, -0.5f,  0.5f );
       glTexCoord2f(1, 0); glVertex3f(  0.5f, -0.5f,  0.5f );
       glTexCoord2f(1, 1); glVertex3f(  0.5f,  0.5f,  0.5f );
       glTexCoord2f(0, 1); glVertex3f( -0.5f,  0.5f,  0.5f );
   glEnd();
   // Render the right quad
   glBindTexture(GL_TEXTURE_2D, _skybox[3]);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glBegin(GL_QUADS);
       glTexCoord2f(0, 0); glVertex3f( -0.5f, -0.5f, -0.5f );
       glTexCoord2f(1, 0); glVertex3f( -0.5f, -0.5f,  0.5f );
       glTexCoord2f(1, 1); glVertex3f( -0.5f,  0.5f,  0.5f );
       glTexCoord2f(0, 1); glVertex3f( -0.5f,  0.5f, -0.5f );
   glEnd();
   // Render the top quad
   glBindTexture(GL_TEXTURE_2D, _skybox[4]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glBegin(GL_QUADS);
       glTexCoord2f(0, 1); glVertex3f( -0.5f,  0.5f, -0.5f );
       glTexCoord2f(0, 0); glVertex3f( -0.5f,  0.5f,  0.5f );
       glTexCoord2f(1, 0); glVertex3f(  0.5f,  0.5f,  0.5f );
       glTexCoord2f(1, 1); glVertex3f(  0.5f,  0.5f, -0.5f );
   glEnd();
   // Render the bottom quad
   glBindTexture(GL_TEXTURE_2D, _skybox[5]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glBegin(GL_QUADS);
       glTexCoord2f(0, 0); glVertex3f( -0.5f, -0.5f, -0.5f );
       glTexCoord2f(0, 1); glVertex3f( -0.5f, -0.5f,  0.5f );
       glTexCoord2f(1, 1); glVertex3f(  0.5f, -0.5f,  0.5f );
       glTexCoord2f(1, 0); glVertex3f(  0.5f, -0.5f, -0.5f );
   glEnd();

   // Restore enable bits and matrix
   glPopAttrib();
   glPopMatrix();
}


void turnLeft(){
    angularAccel -= .003;
}

void turnRight(){
    angularAccel += .003;
}

void display(void)
{
    float accel = .006;
    glm::vec3 acceleration = glm::vec3(accel,accel,accel);
    //Check which keys are down, handle motion
    if(keysDown[UP]){speed += direction * acceleration;}
    if(keysDown[DOWN]){speed -= direction * acceleration;}
    if(keysDown[LEFT]){speed -= rightWing * acceleration;}
    if(keysDown[RIGHT]){speed += rightWing * acceleration;}
    if(keysDown[Qkey]){turnLeft();}
    if(keysDown[Ekey]){turnRight();}

    //Apply friction and move position
    float friction = 0.99f;
    speed *= glm::vec3(friction,friction,friction);
    position += speed;

    //Apply friction and turning
    angularAccel *= friction;
    direction = glm::normalize(direction * float(cos(angularAccel)) + rightWing * float(sin(angularAccel)));
    rightWing = glm::cross(direction, up);

	GLfloat tanamb[] =  {0.10, 0.50, 0.10, 1.0};
	GLfloat tandiff[] = {0.03, 0.15, 0.03, 1.0};
	GLfloat tanspec[] = {0.03, 0.05, 0.03, 1.0};	// grass

	GLfloat teaamb[] = {0.5,0.5,0.5,1.0};
	GLfloat teadiff[] = {0.5,0.5,0.5,1.0};
	GLfloat teaspec[] = {0.3,0.3,0.3,1.0};	// Teapot

	GLfloat lpos[] = {3.0,1.0,1.0,0.0};	// sun, high noon


	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(skyProgram);
    drawSkybox();


    glUseProgram(teapotProgram);

	glColor3f (1.0, 1.0, 1.0);
	glLoadIdentity ();             /* clear the matrix */

	/* viewing transformation, look at the origin  */
	gluLookAt (position.x, position.y, position.z,
               position.x + direction.x, position.y + direction.y, position.z + direction.z,
               up.x, up.y, up.z);

	// send the light position down as if it was a vertex in world coordinates
	glLightfv(GL_LIGHT0, GL_POSITION, lpos);



    glPushMatrix();
    glRotatef(90.0,1.0,0.0,0.0);
    glRotatef(90.0,0.0,1.0,0.0);

	// load teapot material
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, teaamb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, teadiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, teaspec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0);

    //Give the teapot texture
   if(!sphereMap){
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _teapot);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glEnable(GL_BLEND);
   }
   else{
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _sphere);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_S,GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
        glTexGeni(GL_T,GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
   }

    //Draw teapotGL_CLAMP_TO_EDGE
    glBegin(GL_TRIANGLES);
    for(int f = 0; f < NUM_FACES*3; f+=3)
    {
        glTexCoord2f(verts[faces[f]*3-3], verts[faces[f]*3-2]);
        glNormal3f(norms[faces[f]*3-3], norms[faces[f]*3-2], norms[faces[f]*3-1]);
        glVertex3f(verts[faces[f]*3-3], verts[faces[f]*3-2], verts[faces[f]*3-1]);

        glTexCoord2f(verts[faces[f+1]*3-3], verts[faces[f+1]*3-2]);
        glNormal3f(norms[faces[f+1]*3-3], norms[faces[f+1]*3-2], norms[faces[f+1]*3-1]);
        glVertex3f(verts[faces[f+1]*3-3], verts[faces[f+1]*3-2], verts[faces[f+1]*3-1]);

        glTexCoord2f(verts[faces[f+2]*3-3], verts[faces[f+2]*3-2]);
        glNormal3f(norms[faces[f+2]*3-3], norms[faces[f+2]*3-2], norms[faces[f+2]*3-1]);
        glVertex3f(verts[faces[f+2]*3-3], verts[faces[f+2]*3-2], verts[faces[f+2]*3-1]);
    }
    glEnd();

    if(!sphereMap){
        glDisable(GL_TEXTURE_2D);
    }
    else{
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }

    glPopMatrix();

    glUseProgram(grassProgram);
	// Ground plane material
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tanamb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tandiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tanspec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);

	// Ground is a single quad
	glNormal3f(0.0,0.0,1.0);
	glBegin(GL_QUADS);
		glVertex3f(-50,-50,sealevel);
		glVertex3f(50,-50,sealevel);
		glVertex3f(50,50,sealevel);
		glVertex3f(-50,50,sealevel);
	glEnd();

    //Reset keys
    for(int i = 0; i < numKeysDefined; i++){
        keysDown[i] = false;
    }

	glutSwapBuffers();
	glFlush ();

	glutPostRedisplay();
}

void reshape (int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0,(float)w/h,0.01,10.0);
	glMatrixMode (GL_MODELVIEW);
}

void cleanUp(){
    delete [] keysDown;
    delete _skybox;
}

void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
		case '-':
			sealevel -= 0.01;
			break;
		case '+':
		case '=':
			sealevel += 0.01;
			break;
        case ' ' : sphereMap = !sphereMap; break;

		case 'd' : keysDown[RIGHT] = true; break;
        case 'a' : keysDown[LEFT]  = true; break;
        case 'w' : keysDown[UP]    = true; break;
        case 's' : keysDown[DOWN]  = true; break;
        case 'q' : keysDown[Qkey]  = true; break;
        case 'e' : keysDown[Ekey]  = true; break;
		case 27:
		    cleanUp();
			exit(0);
			break;
   }
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize (500, 500);
   glutInitWindowPosition (100, 100);
   glutCreateWindow (argv[0]);
   glewInit();
   init ();
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMainLoop();
   return 0;
}
