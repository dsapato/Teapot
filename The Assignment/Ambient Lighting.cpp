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

int sphereMap;

//Reads in file for shaders
std::string filetext;
std::string* loadFileToString(const char * fname)
{
    std::ifstream ifile(fname);

    if(ifile == NULL){
        std::cout << "File " << fname << " not loaded." << std::endl;
    }

    while( ifile.good() ) {
        std::string line;
        std::getline(ifile, line);
        filetext.append(line,0,line.size()-1);
        filetext.push_back(' ');
    }

    filetext.push_back('/n');

    return &filetext;
}

char* sky_vs = "\
    varying vec2 texture_coordinate;\
	void main(void){\
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\
		texture_coordinate = vec2(gl_MultiTexCoord0);\
	}";
char* sky_fs = "\
    varying vec2 texture_coordinate; uniform sampler2D my_color_texture;\
	void main(void) {\
	    gl_FragColor = texture2D(my_color_texture, texture_coordinate);\
	}";

char* grass_vs = "\
    varying vec4 position;\
    varying vec3 normal;\
    varying vec3 vertex_to_light_vector;\
    void main()\
    {\
        position = gl_ModelViewProjectionMatrix * gl_Vertex;\
        gl_Position = position;\
        normal = gl_NormalMatrix * gl_Normal;\
        vec4 vertex_in_modelview_space = gl_ModelViewMatrix * gl_Vertex;\
        vertex_to_light_vector = vec3(gl_LightSource[0].position - vertex_in_modelview_space);\
    }";

char* grass_fs = "\
    varying vec4 position;\
    varying vec3 normal;\
    varying vec3 vertex_to_light_vector;\
    uniform sampler2D my_color_texture;\
    void main()\
    {\
        const vec4 AmbientColor = vec4(0.0, 0.05, 0.0, 1.0);\
        const vec4 DiffuseColor = vec4(0.0, 0.1, 0.0, 1.0);\
        vec3 normalized_normal = normalize(normal);\
        vec3 normalized_vertex_to_light_vector = normalize(vertex_to_light_vector);\
        float DiffuseTerm = clamp(dot(normal, vertex_to_light_vector), 0.0, 1.0);\
        gl_FragColor = AmbientColor + DiffuseColor * DiffuseTerm + texture2D(my_color_texture, vec2(position.x - floor(position.x), position.y - floor(position.y)));\
	}";

char* teapot_vs = "\
    varying vec3 position;\
    varying vec3 normal;\
    varying vec3 lightPos;\
	void main(void){\
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\
		normal = gl_Normal;\
	}";

char* teapot_fs = "\
    varying vec3 position;\
    varying vec3 normal;\
    varying vec3 lightPos;\
    uniform sampler2D my_color_texture;\
    void main(void) {\
        const vec4 AmbientColor = vec4(0.1, 0.1, 0.1, 1.0);\
        gl_FragColor = AmbientColor + texture2D(my_color_texture, vec2(normal.x,normal.y));\
    }";

char* lightingProgram_vs = "\
    varying vec4 position;\
    varying vec3 normal;\
    varying vec3 vertex_to_light_vector;\
    void main()\
    {\
        position = gl_ModelViewProjectionMatrix * gl_Vertex;\
        gl_Position = position;\
        normal = gl_ProjectionMatrix * vec4(gl_NormalMatrix * gl_Normal, 0.0);\
        vec4 vertex_in_modelview_space = gl_ModelViewProjectionMatrix * gl_Vertex;\
        vertex_to_light_vector = vec3(gl_LightSource[0].position - vertex_in_modelview_space);\
    }";

char* lightingProgram_fs = "\
    varying vec4 position;\
    varying vec3 normal;\
    varying vec3 vertex_to_light_vector;\
    uniform sampler2D my_color_texture;\
    \
    float CalcAttenuation(in vec3 cameraSpacePosition, out vec3 lightDirection)\
    {\
        vec3 lightDifference =  vertex_to_light_vector;\
        float lightDistanceSqr = dot(lightDifference, lightDifference);\
        lightDirection = lightDifference * inversesqrt(lightDistanceSqr);\
        \
        return (1 / ( 2.2 * sqrt(lightDistanceSqr)));\
    }\
    \
    void main()\
    {\
        const vec4 AmbientColor = vec4(0.2, 0.2, 0.2, 1.0);\
        const vec4 DiffuseColor = vec4(0.2, 0.2, 0.2, 1.0);\
        const vec4 specularColor = vec4(0.25, 0.25, 0.25, 1.0);\
        vec3 normalized_normal = normalize(normal);\
        vec3 normalized_vertex_to_light_vector = normalize(vertex_to_light_vector);\
        float DiffuseTerm = clamp(dot(normal, vertex_to_light_vector), 0.0, 1.0);\
        \
        \
        \
        vec4 cameraSpacePosition = gl_ModelViewProjectionMatrix * vec4(position.x,position.y,position.z, 1.0);\
        vec3 lightDir = vec3(0.0);\
        float atten = CalcAttenuation(cameraSpacePosition, lightDir);\
        vec4 attenIntensity = atten * vec4(0.8, 0.8, 0.8, 1.0);\
        \
        vec3 surfaceNormal = normalize(normal);\
        \
        vec3 viewDirection = normalize(-cameraSpacePosition);\
        \
        vec3 halfAngle = normalize(lightDir + viewDirection);\
        float blinnTerm = dot(surfaceNormal, halfAngle);\
        blinnTerm = clamp(blinnTerm, 0, 1);\
        blinnTerm = dot(surfaceNormal, lightDir) >= 0.0 ? blinnTerm : 0.0;\
        blinnTerm = pow(blinnTerm, 50);\
        \
        \
        \
        gl_FragColor = AmbientColor + blinnTerm;\
	}";

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
GLuint lightingProgram;

//Terrain
float randomValue[50][50];

void init(void)
{
    //Sky Shader and program
	GLuint skyvs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(skyvs, 1, (const GLchar **) &(sky_vs), NULL);
	glCompileShader(skyvs);
	printShaderInfoLog(skyvs);

	GLuint skyfs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(skyfs, 1, (const GLchar **) &(sky_fs), NULL);
	glCompileShader(skyfs);
	printShaderInfoLog(skyfs);

	skyProgram = glCreateProgram();
	glAttachShader(skyProgram, skyvs);
	glAttachShader(skyProgram, skyfs);
	glLinkProgram(skyProgram);

    //std::cout << "GRASS" << std::endl << std::endl;

	//Grass Shader and program
	GLuint grassvs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(grassvs, 1, (const GLchar **) &(grass_vs), NULL);
	glCompileShader(grassvs);
	printShaderInfoLog(grassvs);

	GLuint grassfs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(grassfs, 1, (const GLchar **) &(grass_fs), NULL);
	glCompileShader(grassfs);
	printShaderInfoLog(grassfs);

	grassProgram = glCreateProgram();
	glAttachShader(grassProgram, grassvs);
	glAttachShader(grassProgram, grassfs);
	glLinkProgram(grassProgram);

    //std::cout << "TEAPOT" << std::endl << std::endl;

	//Teapot Shader and program
	GLuint teapotvs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(teapotvs, 1, (const GLchar **) &(teapot_vs), NULL);
	glCompileShader(teapotvs);
	printShaderInfoLog(teapotvs);

	GLuint teapotfs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(teapotfs, 1, (const GLchar **) &(teapot_fs), NULL);
	glCompileShader(teapotfs);
	printShaderInfoLog(teapotfs);

	teapotProgram = glCreateProgram();
	glAttachShader(teapotProgram, teapotvs);
	glAttachShader(teapotProgram, teapotfs);
	glLinkProgram(teapotProgram);

	//lighting Shader and program
	GLuint lightingvs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(lightingvs, 1, (const GLchar **) &(lightingProgram_vs), NULL);
	glCompileShader(lightingvs);
	printShaderInfoLog(lightingvs);

	GLuint lightingfs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(lightingfs, 1, (const GLchar **) &(lightingProgram_fs), NULL);
	glCompileShader(lightingfs);
	printShaderInfoLog(lightingfs);

	lightingProgram = glCreateProgram();
	glAttachShader(lightingProgram, lightingvs);
	glAttachShader(lightingProgram, lightingfs);
	glLinkProgram(lightingProgram);

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

	sealevel = 5.25;

    keysDown = new bool[numKeysDefined];
    for(int i = 0; i < numKeysDefined; i++){
        keysDown[i] = false;
    }

    position.x = -5;
    position.y = 1;
    position.z = 6;

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

    //Terrain
	for(int x = 0; x < 50; x++){
        for(int y = 0; y < 50; y++){
            if((x > 20 && x < 26) && (y > 18 && y < 30))
                randomValue[x][y] = 0.0f;
            else
                randomValue[x][y] = rand() % 50 * .01;
        }
	}

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

    sphereMap = 0;
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
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHTING);

   glPopAttrib();
   glPopMatrix();
}


void turnLeft(){
    angularAccel -= .003;
}

void turnRight(){
    angularAccel += .003;
}

void drawTeapot(){
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

    //Calculate z position
    if(position.x < 25 && position.x > -25 && position.y > -25 && position.y < 25){
        position.z = 6 + (randomValue[(int)position.x+24][(int)position.y+24]
                       +  randomValue[(int)position.x+25][(int)position.y+24]
                       +  randomValue[(int)position.x+24][(int)position.y+25]
                       +  randomValue[(int)position.x+25][(int)position.y+25])/4;
    }
    else{
        position.z = 6;
    }

	GLfloat tanamb[] =  {0.05, 0.25, 0.05, 1.0};
	GLfloat tandiff[] = {0.03, 0.15, 0.03, 1.0};
	GLfloat tanspec[] = {0.03, 0.05, 0.03, 1.0};	// grass

	GLfloat teaamb[] = {0.5,0.5,0.5,1.0};
	GLfloat teadiff[] = {0.5,0.5,0.5,1.0};
	GLfloat teaspec[] = {0.3,0.3,0.3,1.0};	// Teapot

	GLfloat lpos[] = {-3.0,3.0,3.0,0.0};	// sun, high noon


	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(skyProgram);
    drawSkybox();

	glColor3f (1.0, 1.0, 1.0);
	glLoadIdentity ();             /* clear the matrix */

	/* viewing transformation, look at the origin  */
	gluLookAt (position.x, position.y, position.z,
               position.x + direction.x, position.y + direction.y, position.z + direction.z,
               up.x, up.y, up.z);

	// send the light position down as if it was a vertex in world coordinates
	glLightfv(GL_LIGHT0, GL_POSITION, lpos);

	// load teapot material
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, teaamb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, teadiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, teaspec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);

    //Small teapots
    glPushMatrix(); //Default opengl
    glUseProgram(0);
    glTranslatef(-1.0f,6.0f,5.2f);
    glRotatef(90.0,1.0,0.0,0.0);
    glRotatef(90.0,0.0,1.0,0.0);
    glScalef(.3,.3,.3);
    drawTeapot();
    glPopMatrix();

    glPushMatrix(); //Texture
    glUseProgram(0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _teapot);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTranslatef(-1.0f,4.0f,5.2f);
    glRotatef(90.0,1.0,0.0,0.0);
    glRotatef(90.0,0.0,1.0,0.0);
    glScalef(.3,.3,.3);
    drawTeapot();
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    glPushMatrix(); //Sphere Map
    glUseProgram(0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _sphere);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTranslatef(-1.0f,-4.0f,5.2f);
    glRotatef(90.0,1.0,0.0,0.0);
    glRotatef(90.0,0.0,1.0,0.0);
    glScalef(.3,.3,.3);
    drawTeapot();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glPopMatrix();

    glPushMatrix(); //Blinn Lighting
    glUseProgram(lightingProgram);
    glTranslatef(-1.0f,-6.0f,5.2f);
    glRotatef(90.0,1.0,0.0,0.0);
    glRotatef(90.0,0.0,1.0,0.0);
    glScalef(.3,.3,.3);
    drawTeapot();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f,0.0f,5.0f);
    glRotatef(90.0,1.0,0.0,0.0);
    glRotatef(90.0,0.0,1.0,0.0);

    //Give the teapot texture
   if(sphereMap==1){
        glUseProgram(0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _teapot);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   }
   else if(sphereMap == 2){
        glUseProgram(0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _sphere);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
   }
   else if(sphereMap == 3){
        glUseProgram(lightingProgram);
   }
   else{
        glUseProgram(0);
   }

    //Draw main teapot
    drawTeapot();

    if(sphereMap == 1){
        glDisable(GL_TEXTURE_2D);
    }
    else if(sphereMap == 2){
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
    }

    glPopMatrix();

    glUseProgram(0);
	// Ground plane material
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, tanamb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, tandiff);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tanspec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _grass);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Ground is a single quad
	glBegin(GL_QUADS);
	for(int x = -25; x < 25; x++){
        for(int y = -25; y < 25; y++){
            glm::vec3 dx = glm::vec3(randomValue[x+25] - randomValue[x+24], 0, 1);
            glm::vec3 dy = glm::vec3(0, randomValue[y+25] - randomValue[y+24], 1);
            glm::vec3 cross = glm::cross(dx, dy);
            glNormal3f(cross.x, cross.y , cross.z);

            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(x,y,sealevel + randomValue[x+24][y+24]);

            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(x+1,y,sealevel + randomValue[x+25][y+24]);

            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(x+1,y+1,sealevel + randomValue[x+25][y+25]);

            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(x,y+1,sealevel + randomValue[x+24][y+25]);
        }
	}
	glEnd();

	glDisable(GL_TEXTURE_2D);

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
	gluPerspective(90.0,(float)w/h,0.01,20.0);
	glMatrixMode (GL_MODELVIEW);
}

void cleanUp(){
    delete [] keysDown;
    delete _skybox;
}

void keyboard(unsigned char key, int x, int y)
{
   switch (key) {

        case ' ' : sphereMap = (sphereMap+1)%4; break;

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
