#include <cstdio>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#ifdef _arch_dreamcast
#include <kos.h>
float avgfps = -1;
#endif

#include "GL/gl.h"
#include "GL/glkos.h"
#include "GL/glu.h"
#include "GL/glext.h"

#define PI 3.14159265358979323846264338327950288f
#define RAD_TO_DEG 57.295779513082320876798154814105f
#define MAX_CUBES 350

size_t numCubes = 0;

float timeElapsed = 0.0f;
const float dt = 1.0f / 60.0f;

float angleDegrees = 0;
const float invAngle360 = 1.0f / 360.0f;
const float cameraDistance = 3.0f;

bool isDrawingArrays = false;
bool isBlendingEnabled = true;
bool isRunning = true;

typedef struct
{
	GLubyte r;
	GLubyte g;
	GLubyte b;
	GLubyte a;
} Color;

Color colors[] =
{
	{255, 0, 0, 128},
	{0, 255, 0, 128},
	{0, 0, 255, 128},
	{255, 255, 0, 128},
	{255, 0, 255, 128},
	{0, 255, 255, 128}
};
Color faceColors[24];

float cubeVertices[] =
{
	// Front face
	-1.0f, -1.0f, +1.0f, // vertex 0
	+1.0f, -1.0f, +1.0f, // vertex 1
	+1.0f, +1.0f, +1.0f, // vertex 2
	-1.0f, +1.0f, +1.0f, // vertex 3

	// Back face
	-1.0f, -1.0f, -1.0f, // vertex 4
	+1.0f, -1.0f, -1.0f, // vertex 5
	+1.0f, +1.0f, -1.0f, // vertex 6
	-1.0f, +1.0f, -1.0f, // vertex 7

	// Top face
	-1.0f, +1.0f, +1.0f, // vertex 8
	+1.0f, +1.0f, +1.0f, // vertex 9
	+1.0f, +1.0f, -1.0f, // vertex 10
	-1.0f, +1.0f, -1.0f, // vertex 11

	// Bottom face
	-1.0f, -1.0f, +1.0f, // vertex 12
	+1.0f, -1.0f, +1.0f, // vertex 13
	+1.0f, -1.0f, -1.0f, // vertex 14
	-1.0f, -1.0f, -1.0f, // vertex 15

	// Right face
	+1.0f, -1.0f, +1.0f, // vertex 16
	+1.0f, -1.0f, -1.0f, // vertex 17
	+1.0f, +1.0f, -1.0f, // vertex 18
	+1.0f, +1.0f, +1.0f, // vertex 19

	// Left face
	-1.0f, -1.0f, +1.0f, // vertex 20
	-1.0f, -1.0f, -1.0f, // vertex 21
	-1.0f, +1.0f, -1.0f, // vertex 22
	-1.0f, +1.0f, +1.0f  // vertex 23
};

// Set up indices array
unsigned int cubeIndices[] =
{
	// Front face
	0, 1, 2, 3,

	// Back face
	4, 5, 6, 7,

	// Top face
	8, 9, 10, 11,

	// Bottom face
	12, 13, 14, 15,

	// Right face
	16, 17, 18, 19,

	// Left face
	20, 21, 22, 23
};

typedef struct
{
	float r;
	float x, y, z;
	float vx, vy, vz;
} Cube;

Cube cubes[MAX_CUBES];
float lutScaleFactors[MAX_CUBES];


void debugLog(const char* msg) 
{
#ifdef _arch_dreamcast
	dbglog(DBG_KDEBUG, "%s\n", msg);
#else
	printf("%s\n", msg);
#endif
}


void runningStats() 
{
#ifdef _arch_dreamcast
	pvr_stats_t stats;
	pvr_get_stats(&stats);

	if (avgfps != -1)
		avgfps = (avgfps + stats.frame_rate) * 0.5f;
	else
		avgfps = stats.frame_rate;
#endif
}


void avgStats() 
{
#ifdef _arch_dreamcast
	dbglog(DBG_DEBUG, "Average frame rate: ~%f fps\n", avgfps);
#endif
}


void stats() 
{
#ifdef _arch_dreamcast
	pvr_stats_t stats;

	pvr_get_stats(&stats);
	dbglog(DBG_DEBUG, "3D Stats: %d VBLs, current frame rate ~%f fps\n", stats.vbl_count, stats.frame_rate);
	avgStats();
#endif
}


void addCube(const float r, const float x, const float y, const float z, const float vx, const float vy, const float vz)
{
	if (numCubes < MAX_CUBES) 
	{
		cubes[numCubes].r = r;
		cubes[numCubes].x = x;
		cubes[numCubes].y = y;
		cubes[numCubes].z = z;
		cubes[numCubes].vx = vx;
		cubes[numCubes].vy = vy;
		cubes[numCubes].vz = vz;
		numCubes++;
	}
}


void updateCubes(float dt)
{
	for (size_t i = 0; i < MAX_CUBES; i++)
	{
		Cube* pCube = &cubes[i];
		pCube->x += pCube->vx * dt;
		pCube->y += pCube->vy * dt;
		pCube->z += pCube->vz * dt;

		if (pCube->x < -3 || pCube->x > +3) { pCube->vx *= -1; }
		if (pCube->y < -3 || pCube->y > +3) { pCube->vy *= -1; }
		if (pCube->z < -3 || pCube->z > +3) { pCube->vz *= -1; }
	}
}


void renderUnitCube()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, cubeVertices);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, faceColors);

	isDrawingArrays ? glDrawArrays(GL_QUADS, 0, 24) : glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, cubeIndices);

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void renderCubes(const float angleRotation)
{
	for (size_t i = 0; i < MAX_CUBES; i++) 
	{
		const float scaleFactor = lutScaleFactors[i];
		Cube* pCube = &cubes[i];

		glPushMatrix(); // Save previous camera state
		glMatrixMode(GL_MODELVIEW);

		glTranslatef(pCube->x, pCube->y, pCube->z);
		glRotatef(angleRotation, 1, 1, 1); // Rotate camera / object

		glScalef(scaleFactor, scaleFactor, scaleFactor); // Apply scale factor

		renderUnitCube();
		glPopMatrix(); // Restore previous camera state
	}
}


float rnd(float valueMin, float valueMax)
{
	return (valueMax - valueMin) * (float)rand() / (float)RAND_MAX + valueMin;
}


void initialize()
{
	debugLog("Initialize video output");
	glKosInit();

	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

	if (isBlendingEnabled)
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);

	glViewport(0, 0, 640, 480);
	glClearColor(0.0f, 0.0f, 0.3f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Set up colors (each face has a different color)
	for (size_t i = 0; i < 6; i++)
	{
		faceColors[i * 4] = colors[i];
		faceColors[i * 4 + 1] = colors[i];
		faceColors[i * 4 + 2] = colors[i];
		faceColors[i * 4 + 3] = colors[i];
	}

	// Precalculate scale factors and store them in a LUT
	const float incFactor = 0.35f / (float)MAX_CUBES;
	for (size_t i = 0; i < MAX_CUBES; ++i) 
	{
		lutScaleFactors[i] = 0.05f + (i * incFactor);
	}
}


void updateTimer()
{
	timeElapsed += dt;

	if (timeElapsed > 10.0f)
	{
		stats();
		timeElapsed = 0.0f;
	}
}


void updateLogic()
{
	updateTimer();

	const int fullRot = (int)(angleDegrees * invAngle360);
	angleDegrees -= fullRot * 360.0f;
	angleDegrees += 50.0f * dt;

	const float zoomVal = __builtin_sinf(timeElapsed) * 5.0f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set up the camera position and orientation
	float cameraPos[] = { 0.0f, 0.0f, cameraDistance };
	float cameraTarget[] = { 0.0f, 0.0f, 0.0f };
	float cameraUp[] = { 0.0f, 1.0f, 0.0f };

	// Move the camera
	gluLookAt(cameraPos[0], cameraPos[1], cameraPos[2],
		cameraTarget[0], cameraTarget[1], cameraTarget[2],
		cameraUp[0], cameraUp[1], cameraUp[2]);

	glTranslatef(0.0f, 0.0f, -cameraDistance + zoomVal);

	// Apply cube transformation (identity matrix)
	glLoadIdentity();

	updateCubes(dt);

	renderCubes(angleDegrees);

	// Reset ModelView matrix to remove camera transformation
	float matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
	matrix[12] = 0.0f;
	matrix[13] = 0.0f;
	matrix[14] = 0.0f;

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(matrix);
}


void updateInput()
{
#ifdef _arch_dreamcast
	static uint8_t prevButtons = 0;
	maple_device_t* cont;
	cont_state_t* state;

	cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

	if (cont)
	{
		state = (cont_state_t*)maple_dev_status(cont);

		if (state && (state->buttons & CONT_START) && !(prevButtons & CONT_START))
		{
			isRunning = false;
		}

		if (state && (state->buttons & CONT_A) && !(prevButtons & CONT_A))
		{
			isDrawingArrays = !isDrawingArrays;

			isDrawingArrays ? glClearColor(0.3f, 0.0f, 0.3f, 1.0f) : glClearColor(0.0f, 0.0f, 0.3f, 1.0f);
		}

		if (state && (state->buttons & CONT_B) && !(prevButtons & CONT_B))
		{
			isBlendingEnabled = !isBlendingEnabled;

			isBlendingEnabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
		}

		prevButtons = state->buttons;
	}
#endif
}


void swapBuffers()
{
#ifdef _arch_dreamcast
	glKosSwapBuffers();
#endif
}


int main(int argc, char* argv[])
{
	initialize();

	// Setup camera frustum
	const float aspectRatio = 640.0f / 480.0f;
	const float fov = 60;
	const float zNear = 0.1f;
	const float zFar = 1000.0f;

	gluPerspective(fov, aspectRatio, zNear, zFar);

	for (size_t i = 0; i < MAX_CUBES; i++)
	{
		const float r = rnd(0.1f, 0.5f);
		const float x = rnd(-3.0f, 3.0f);
		const float y = rnd(-3.0f, 3.0f);
		const float z = rnd(-3.0f, 3.0f);
		const float vx = rnd(-2.0f, 2.0f);
		const float vy = rnd(-2.0f, 2.0f);
		const float vz = rnd(-2.0f, 2.0f);

		addCube(r, x, y, z, vx, vy, vz);
	}

	while (isRunning)
	{
		updateLogic();
		updateInput();
		swapBuffers();
		runningStats();
	}

	avgStats();

	return 0;
}
