//Copyright (C) 2010-2012 by Jason L. McKesson
//This file is licensed under the MIT License.


#include <string>
#include <vector>
#include <stack>
#include <math.h>
#include <stdio.h>
#include <glload/gl_3_3.h>
#include <glutil/glutil.h>
#include <GL/freeglut.h>
#include "../framework/framework.h"
#include "../framework/Mesh.h"
#include "../framework/MousePole.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define ARRAY_COUNT( array ) (sizeof( array ) / (sizeof( array[0] ) * (sizeof( array ) != sizeof(void*) || sizeof( array[0] ) <= sizeof(void*))))

struct ProgramData
{
	GLuint theProgram;

	GLuint dirToLightUnif;
	GLuint lightIntensityUnif;

	GLuint modelToCameraMatrixUnif;
	GLuint normalModelToCameraMatrixUnif;
};

float g_fzNear = 1.0f;
float g_fzFar = 1000.0f;

ProgramData g_WhiteDiffuseColor;
ProgramData g_VertexDiffuseColor;

const int g_projectionBlockIndex = 2;

ProgramData LoadProgram(const std::string &strVertexShader, const std::string &strFragmentShader)
{
	std::vector<GLuint> shaderList;

	shaderList.push_back(Framework::LoadShader(GL_VERTEX_SHADER, strVertexShader));
	shaderList.push_back(Framework::LoadShader(GL_FRAGMENT_SHADER, strFragmentShader));

	ProgramData data;
	data.theProgram = Framework::CreateProgram(shaderList);
	data.modelToCameraMatrixUnif = glGetUniformLocation(data.theProgram, "modelToCameraMatrix");
	data.normalModelToCameraMatrixUnif = glGetUniformLocation(data.theProgram, "normalModelToCameraMatrix");
	data.dirToLightUnif = glGetUniformLocation(data.theProgram, "dirToLight");
	data.lightIntensityUnif = glGetUniformLocation(data.theProgram, "lightIntensity");

	GLuint projectionBlock = glGetUniformBlockIndex(data.theProgram, "Projection");
	glUniformBlockBinding(data.theProgram, projectionBlock, g_projectionBlockIndex);

	return data;
}

void InitializeProgram()
{
	g_WhiteDiffuseColor = LoadProgram("DirVertexLighting_PN.vert", "ColorPassthrough.frag");
	g_VertexDiffuseColor = LoadProgram("DirVertexLighting_PCN.vert", "ColorPassthrough.frag");
}

Framework::Mesh *g_pCylinderMesh = NULL;
Framework::Mesh *g_pPlaneMesh = NULL;



//Called after the window and OpenGL are initialized. Called exactly once, before the main loop.
void init()
{
	InitializeProgram();

	try
	{
		g_pCylinderMesh = new Framework::Mesh("UnitCylinder.xml");
		g_pPlaneMesh = new Framework::Mesh("LargePlane.xml");
	}
	catch(std::exception &except)
	{
		printf("%s\n", except.what());
		throw;
	}

	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseMotion);
	glutMouseWheelFunc(MouseWheel);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0.0f, 1.0f);
	glEnable(GL_DEPTH_CLAMP);

	glGenBuffers(1, &g_projectionUniformBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, g_projectionUniformBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionBlock), NULL, GL_DYNAMIC_DRAW);

	//Bind the static buffers.
	glBindBufferRange(GL_UNIFORM_BUFFER, g_projectionBlockIndex, g_projectionUniformBuffer,
		0, sizeof(ProjectionBlock));

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

glm::vec4 g_lightDirection(0.866f, 0.5f, 0.0f, 0.0f);

static bool g_bDrawColoredCyl = true;

//Called to update the display.
//You should call glutSwapBuffers after all of your rendering to display what you rendered.
//If you need continuous updates of the screen, call glutPostRedisplay() at the end of the function.
void display()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(g_pPlaneMesh && g_pCylinderMesh)
	{
		glutil::MatrixStack modelMatrix;
		modelMatrix.SetMatrix(g_viewPole.CalcMatrix());

		glm::vec4 lightDirCameraSpace = modelMatrix.Top() * g_lightDirection;

		glUseProgram(g_WhiteDiffuseColor.theProgram);
		glUniform3fv(g_WhiteDiffuseColor.dirToLightUnif, 1, glm::value_ptr(lightDirCameraSpace));
		glUseProgram(g_VertexDiffuseColor.theProgram);
		glUniform3fv(g_VertexDiffuseColor.dirToLightUnif, 1, glm::value_ptr(lightDirCameraSpace));
		glUseProgram(0);

		{
			glutil::PushStack push(modelMatrix);

			//Render the ground plane.
			{
				glutil::PushStack push(modelMatrix);

				glUseProgram(g_WhiteDiffuseColor.theProgram);
				glUniformMatrix4fv(g_WhiteDiffuseColor.modelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
				glm::mat3 normMatrix(modelMatrix.Top());
				glUniformMatrix3fv(g_WhiteDiffuseColor.normalModelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(normMatrix));
				glUniform4f(g_WhiteDiffuseColor.lightIntensityUnif, 1.0f, 1.0f, 1.0f, 1.0f);
				g_pPlaneMesh->Render();
				glUseProgram(0);
			}

			//Render the Cylinder
			{
				glutil::PushStack push(modelMatrix);

				modelMatrix.ApplyMatrix(g_objtPole.CalcMatrix());

				if(g_bDrawColoredCyl)
				{
					glUseProgram(g_VertexDiffuseColor.theProgram);
					glUniformMatrix4fv(g_VertexDiffuseColor.modelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
					glm::mat3 normMatrix(modelMatrix.Top());
					glUniformMatrix3fv(g_VertexDiffuseColor.normalModelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(normMatrix));
					glUniform4f(g_VertexDiffuseColor.lightIntensityUnif, 1.0f, 1.0f, 1.0f, 1.0f);
					g_pCylinderMesh->Render("lit-color");
				}
				else
				{
					glUseProgram(g_WhiteDiffuseColor.theProgram);
					glUniformMatrix4fv(g_WhiteDiffuseColor.modelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
					glm::mat3 normMatrix(modelMatrix.Top());
					glUniformMatrix3fv(g_WhiteDiffuseColor.normalModelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(normMatrix));
					glUniform4f(g_WhiteDiffuseColor.lightIntensityUnif, 1.0f, 1.0f, 1.0f, 1.0f);
					g_pCylinderMesh->Render("lit");
				}
				glUseProgram(0);
			}
		}
	}

	glutSwapBuffers();
}

//Called whenever the window is resized. The new window size is given, in pixels.
//This is an opportunity to call glViewport or glScissor to keep up with the change in size.
void reshape (int w, int h)
{
	glutil::MatrixStack persMatrix;
	persMatrix.Perspective(45.0f, (w / (float)h), g_fzNear, g_fzFar);

	ProjectionBlock projData;
	projData.cameraToClipMatrix = persMatrix.Top();

	glBindBuffer(GL_UNIFORM_BUFFER, g_projectionUniformBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ProjectionBlock), &projData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glutPostRedisplay();
}

//Called whenever a key on the keyboard was pressed.
//The key is given by the ''key'' parameter, which is in ASCII.
//It's often a good idea to have the escape key (ASCII value 27) call glutLeaveMainLoop() to
//exit the program.
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		delete g_pPlaneMesh;
		delete g_pCylinderMesh;
		glutLeaveMainLoop();
		return;

	case 32:
		g_bDrawColoredCyl = !g_bDrawColoredCyl;
		break;
	}

	glutPostRedisplay();
}


unsigned int defaults(unsigned int displayMode, int &width, int &height) {return displayMode;}
