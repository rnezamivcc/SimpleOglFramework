#include <stdlib.h>
#include <time.h>
//#include <math.h>
//#include <stddef.h>

#include <stdio.h>s
#include <string.h>s

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifndef MACOSX
#include <GL/gl.h>
#include <GL/glu.h>
#include<GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#pragma comment(lib, "legacy_stdio_definitions.lib")

#include "wcode/fssimplewindow.h"
#include "bitmapfont/ysglfontdata.h"

#include "vectors.h"

enum
{
	SPHERE = 1, CONE, LIGHT, LEFTWALL, FLOOR
};

enum {
	X, Y, Z, W
};
enum {
	A, B, C, D
};

/* Rendering shadows using projective shadows. */

/* Create a single component texture map */
std::shared_ptr<float> make_texture(int maxs, int maxt)
{
	int s, t;
	std::shared_ptr<float> text((new float[maxs * maxt * sizeof(GLfloat)]{}), [](float* p) {delete[] p; });
	float* texture{ text.get() };
	for (t = 0; t < maxt; t++) 
	{
		for (s = 0; s < maxs; s++) 
		{
			texture[s + maxs * t] = 1.f - (((s >> 4) & 0x1) ^ ((t >> 4) & 0x1)) *0.5f;
		}
	}
	return text;
}


/* create a matrix that will project the desired shadow */
void shadowmatrix(GLfloat shadowMat[4][4], float4D groundplane, float4D lightpos)
{
	/* find dot product between light position vector and ground plane normal */
	float dot = Dot(groundplane, lightpos);


	shadowMat[X][0] = dot - lightpos[X] * groundplane[X];
	shadowMat[1][0] = 0.f - lightpos[X] * groundplane[Y];
	shadowMat[2][0] = 0.f - lightpos[X] * groundplane[Z];
	shadowMat[3][0] = 0.f - lightpos[X] * groundplane[W];

	shadowMat[X][1] = 0.f - lightpos[Y] * groundplane[X];
	shadowMat[1][1] = dot - lightpos[Y] * groundplane[Y];
	shadowMat[2][1] = 0.f - lightpos[Y] * groundplane[Z];
	shadowMat[3][1] = 0.f - lightpos[Y] * groundplane[W];

	shadowMat[X][2] = 0.f - lightpos[Z] * groundplane[X];
	shadowMat[1][2] = 0.f - lightpos[Z] * groundplane[Y];
	shadowMat[2][2] = dot - lightpos[Z] * groundplane[Z];
	shadowMat[3][2] = 0.f - lightpos[Z] * groundplane[W];

	shadowMat[X][3] = 0.f - lightpos[W] * groundplane[X];
	shadowMat[1][3] = 0.f - lightpos[W] * groundplane[Y];
	shadowMat[2][3] = 0.f - lightpos[W] * groundplane[Z];
	shadowMat[3][3] = dot - lightpos[W] * groundplane[W];
}

/* find the plane equation given 3 points */
void findplane(float4D &plane, float3D v0, float3D v1, float3D v2)
{
	float3D vec0{ v1 - v0 };
	float3D vec1{ v2 - v0 };

	/* find cross product to get A, B, and C of plane equation */
	float3D normal = Cross(vec0, vec1);
	//normal = plane = Normal(normal);
	plane[D] = -Dot(normal, v0); 
}

GLfloat PosX = 60.f, PosY = -50.f, PosZ = -360.f;
void sphere(void)
{
	glPushMatrix();
//	glTranslatef(60.f, -50.f, -360.f);
	glTranslatef(PosX, PosY, PosZ);
	glCallList(SPHERE);
	glPopMatrix();
}

void cone(void)
{
	glPushMatrix();
	glTranslatef(-40.f, -40.f, -400.f);
	glCallList(CONE);
	glPopMatrix();
}

enum 
{
  NONE, SHADOW
};

int rendermode = SHADOW;

void menu(int mode)
{
	rendermode = mode;
	glutPostRedisplay();
}

GLfloat leftwallshadow[4][4];
GLfloat floorshadow[4][4];

float4D lightpos{50.f, 50.f, -320.f, 1.f};

GLfloat gameTime = 0;
const int timeSpan = 33; // milliseconds
double timeInc = (double)timeSpan * 0.001; // time increment in seconds
DWORD passedTime = 0;

GLfloat VelX = 0.f, VelY = 1.f, VelZ = 0.f;
void update(void)
{
	timeInc = (double)(passedTime)* 0.001;

	// update physics here
	PosX += VelX * timeInc;
	PosY += VelY * timeInc;
	PosZ += VelZ * timeInc;

}


void redraw(void)
{
	/* material properties for objects in scene */
	static GLfloat wall_mat[] = {1.f, 1.f, 1.f, 1.f};
	static GLfloat sphere_mat[] = {1.f, .5f, 0.f, 1.f};
	static GLfloat cone_mat[] = {0.f, .5f, 1.f, 1.f};

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	/* Note: wall verticies are ordered so they are all front facing this lets
		me do back face culling to speed things up.  */

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, wall_mat);

	/* floor 
	make the floor textured 
	Since we want to turn texturing on for floor only, we have to make floor 
		a separate glBegin()/glEnd() sequence. You can't turn texturing on and
		off between begin and end calls 
		*/
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
		glColor3f(1.f, 0.f, 0.f);
		glNormal3f(0.f, 1.f, 0.f);
		glTexCoord2i(0, 0);
		glVertex3f(-100.f, -100.f, -320.f);
		glTexCoord2i(1, 0);
		glVertex3f(100.f, -100.f, -320.f);
		glTexCoord2i(1, 1);
		glVertex3f(100.f, -100.f, -520.f);
		glTexCoord2i(0, 1);
		glVertex3f(-100.f, -100.f, -520.f);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	if (rendermode == SHADOW)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glColor3f(0.f, 0.f, 0.f);  /* shadow color */

		glPushMatrix();
		glMultMatrixf((GLfloat *) floorshadow);
		cone();
		glPopMatrix();

		glPushMatrix();
		glMultMatrixf((GLfloat*)floorshadow);
		sphere();
		glPopMatrix();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
	}

	/* walls */
	if (rendermode == SHADOW) 
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 1, 0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	/* left wall */
	glBegin(GL_QUADS);
	glNormal3f(1.f, 0.f, 0.f);
	glVertex3f(-100.f, -100.f, -320.f);
	glVertex3f(-100.f, -100.f, -520.f);
	glVertex3f(-100.f, 100.f, -520.f);
	glVertex3f(-100.f, 100.f, -320.f);
	glEnd();

	if (rendermode == SHADOW)
	{
		glStencilFunc(GL_EQUAL, 1, 1);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glColor3f(0.f, 0.f, 0.f);  /* shadow color */
		glDisable(GL_DEPTH_TEST);
		glPushMatrix();
		glMultMatrixf((GLfloat *) leftwallshadow);
		cone();
		glPopMatrix();
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
	}
	glBegin(GL_QUADS);
	/* right wall */
	glNormal3f(-1.f, 0.f, 0.f);
	glVertex3f(100.f, -100.f, -320.f);
	glVertex3f(100.f, 100.f, -320.f);
	glVertex3f(100.f, 100.f, -520.f);
	glVertex3f(100.f, -100.f, -520.f);

	/* ceiling */
	glNormal3f(0.f, -1.f, 0.f);
	glVertex3f(-100.f, 100.f, -320.f);
	glVertex3f(-100.f, 100.f, -520.f);
	glVertex3f(100.f, 100.f, -520.f);
	glVertex3f(100.f, 100.f, -320.f);

	/* back wall */
	glNormal3f(0.f, 0.f, 1.f);
	glVertex3f(-100.f, -100.f, -520.f);
	glVertex3f(100.f, -100.f, -520.f);
	glVertex3f(100.f, 100.f, -520.f);
	glVertex3f(-100.f, 100.f, -520.f);
	glEnd();

	glPushMatrix();
	glTranslatef(lightpos[X], lightpos[Y], lightpos[Z]);
	glDisable(GL_LIGHTING);
	glColor3f(1.f, 1.f, .7f);
	glCallList(LIGHT);
	glEnable(GL_LIGHTING);
	glPopMatrix();

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cone_mat);
	cone();

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
	sphere();

	glutSwapBuffers();    


	////// update time lapse /////////////////
	passedTime = FsPassedTime();
	int timediff = timeSpan - passedTime;
	//	printf("\ntimeInc=%f, passedTime=%d, timediff=%d", timeInc, passedTime, timediff);
	while (timediff >= timeSpan / 3)
	{
		FsSleep(5);
		passedTime = FsPassedTime();
		timediff = timeSpan - passedTime;
		//	printf("--passedTime=%d, timediff=%d", passedTime, timediff);
	}
	passedTime = FsPassedTime(true);

}

/* ARGSUSED1 */
void key(unsigned char key, int x, int y)
{
  if (key == '\033')
    exit(0);
}

const int TEXDIM = 256;
/* Parse arguments, and set up interface between OpenGL and window system */


int main(int argc, char *argv[])
{
	GLUquadricObj *sphereObj, *coneObj, *base;
//	GLfloat plane[4];
//	float3D v0, v1, v2;

	FsPassedTime(true);
	passedTime = 0;

	glutInit(&argc, argv);
	glutInitWindowSize(512, 512);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE);
	(void) glutCreateWindow("projection shadows");
	glutDisplayFunc(redraw);
	glutKeyboardFunc(key);
	glutIdleFunc(update);
	glutCreateMenu(menu);
	glutAddMenuEntry("No Shadows", NONE);
	glutAddMenuEntry("Shadows", SHADOW);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/* draw a perspective scene */
	glMatrixMode(GL_PROJECTION);
	glFrustum(-100., 100., -100., 100., 320., 640.);
	glMatrixMode(GL_MODELVIEW);

	/* turn on features */
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	/* make shadow matricies */

	/* 3 points on floor */
	float3D v0(-100.f, -100.f, -320.f);
	float3D v1(100.f, -100.f, -320.f);
	float3D v2(100.f, -100.f, -520.f);

	float4D plane;
	
	findplane(plane, v0, v1, v2);
	shadowmatrix(floorshadow, plane, lightpos);

	/* 3 points on left wall */
	v0.set(-100.f, -100.f, -320.f), v1.set(-100.f, -100.f, -520.f), v2.set(-100.f, 100.f, -520.f);

	findplane(plane, v0, v1, v2);
	shadowmatrix(leftwallshadow, plane, lightpos);

	float* pt = lightpos.get();
	/* place light 0 in the right place */
	glLightfv(GL_LIGHT0, GL_POSITION, pt);

	/* remove back faces to speed things up */
	glCullFace(GL_BACK);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	/* make display lists for sphere and cone; for efficiency */

	glNewList(SPHERE, GL_COMPILE);
	sphereObj = gluNewQuadric();
	gluSphere(sphereObj, 3.f, 10, 10);
	gluDeleteQuadric(sphereObj);
	glEndList();

	glNewList(LIGHT, GL_COMPILE);
	sphereObj = gluNewQuadric();
	gluSphere(sphereObj, 1.f, 10, 10);
	gluDeleteQuadric(sphereObj);
	glEndList();

	glNewList(CONE, GL_COMPILE);
	coneObj = gluNewQuadric();
	base = gluNewQuadric();
	glRotatef(-90.f, 1.f, 0.f, 0.f);
	gluDisk(base, 0., 20., 20, 1);
	gluCylinder(coneObj, 20., 0., 60., 20, 20);
	gluDeleteQuadric(coneObj);
	gluDeleteQuadric(base);
	glEndList();

	glNewList(FLOOR, GL_COMPILE);
	glEndList();

	glNewList(LEFTWALL, GL_COMPILE);
	glEndList();
	
	/* load pattern for current 2d texture */
	std::shared_ptr<float> tex = make_texture(TEXDIM, TEXDIM);
	glTexImage2D(GL_TEXTURE_2D, 0, 1, TEXDIM, TEXDIM, 0, GL_RED, GL_FLOAT, tex.get());

	glutMainLoop();

	return 0;             
}
