#include <nds.h>

#include <stdio.h>
#include <math.h>

//console is 24x32

typedef struct {
	float x;
	float y;
	float param;
} Coord;

typedef struct {
	Coord a;
	Coord b;
} Ray;

typedef struct {
	Coord a, b, c, d;
} Square;

Coord nullCoord = {-1, -1};
Ray lineRay;

bool isCoordNull(Coord coord) {
	if (coord.x == -1 and coord.y == -1) return true;
	return false;
};

Square segments[] = { // the objects on screen
	{{0.5, 0.9}, {0.4, 0.9}, {0.4, 0.5}, {0.5, 0.5}},

	{{-1.0, -1.0}, {-1.0, -1,0}, {-1.0, 1.0}, {-1.0, 1.0}}, // left wall
	{{-1.0, -1.0}, {1.0, -1,0}, {1.0, -1.0}, {-1.0, -1.0}}, // bottom wall
	{{1.0, -1.0}, {1.0, -1,0}, {1.0, 1.0}, {1.0, 1.0}}, // right wall
	{{1.0, 1.0}, {1.0, 1,0}, {-1.0, 1.0}, {-1.0, 1.0}}, // top wall
};

Coord convertNDSCoordsToGL(Coord ndsCoord) {
	double x = (ndsCoord.x / (double) SCREEN_WIDTH) * 2 /*sum of -1 and 1*/ + -1;
	double y = (1 - ndsCoord.y / (double) SCREEN_HEIGHT) * 2 /*sum of -1 and 1*/ -1;

	return {x, y};
};

Coord getIntersection(Ray ray, Ray segment_line) {
	// RAY in parametric: Point + Direction*T1
	float r_px = ray.a.x;
	float r_py = ray.a.y;
	float r_dx = ray.b.x-ray.a.x;
	float r_dy = ray.b.y-ray.a.y;

	// SEGMENT in parametric: Point + Direction*T2
	float s_px = segment_line.a.x;
	float s_py = segment_line.a.y;
	float s_dx = segment_line.b.x-segment_line.a.x;
	float s_dy = segment_line.b.y-segment_line.a.y;

	// Are they parallel? If so, no intersect (Pythagorean)
	float r_mag = sqrt(r_dx*r_dx+r_dy*r_dy);
	float s_mag = sqrt(s_dx*s_dx+s_dy*s_dy);

	if(r_dx/r_mag==s_dx/s_mag && r_dy/r_mag==s_dy/s_mag){ // Directions are the same.
		return nullCoord;
	}
	// SOLVE FOR T1 & T2
	float T2 = (r_dx*(s_py-r_py) + r_dy*(r_px-s_px))/(s_dx*r_dy - s_dy*r_dx);
	float T1 = (s_px+s_dx*T2-r_px)/r_dx;

	// Must be within parametic whatevers for RAY/SEGMENT
	if(T1<0) return nullCoord;
	if(T2<0 || T2>1) return nullCoord;

	Coord raycast = {r_px+r_dx*T1, r_py+r_dy*T1, T1};
	return raycast;
}

// Segment related functions
void renderSegments() {
	for (int i = 0; i < sizeof(segments)/sizeof(Square); i++) {
		glPushMatrix();
		glBegin(GL_QUADS);

		glColor3b(255, 255, 255);
		glVertex3v16(floattov16(segments[i].a.x),floattov16(segments[i].a.y), 0); // A
		glColor3b(255, 255, 255);
		glVertex3v16(floattov16(segments[i].b.x),floattov16(segments[i].b.y), 0); // B
		glColor3b(255, 255, 255);
		glVertex3v16(floattov16(segments[i].c.x),floattov16(segments[i].c.y), 0); // C
		glColor3b(255, 255, 255);
		glVertex3v16(floattov16(segments[i].d.x),floattov16(segments[i].d.y), 0); // D

		glEnd();
		glPopMatrix(1);
	}
}

// the redline related functions
void renderLine(Coord coord) {
	glPushMatrix();
	glBegin(GL_QUADS);
		//Coord converted = convertNDSCoordsToGL(line_ray.b);

		glColor3b(255, 0, 0);
		glVertex3v16(floattov16(lineRay.a.x),floattov16(lineRay.a.y), 0); // A
		glColor3b(255, 0, 0);
		glVertex3v16(floattov16(lineRay.a.x), floattov16(lineRay.a.y), 0); // B
		glColor3b(255, 0, 0);
		glVertex3v16(floattov16(coord.x), floattov16(coord.y), 0); // C
		glColor3b(255, 0, 0);
		glVertex3v16(floattov16(coord.x), floattov16(coord.y), 0); // D

	glEnd();
	glPopMatrix(1);
}
touchPosition touch;

volatile int frame = 0;

//---------------------------------------------------------------------------------
void Vblank() {
//---------------------------------------------------------------------------------
	frame++;
}
	
//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	irqSet(IRQ_VBLANK, Vblank);

	videoSetMode(MODE_0_3D);
	consoleDemoInit();

	glInit();
	glClearColor(0, 0, 0, 31);
	glClearDepth(GL_MAX_DEPTH);

	glViewport(0, 0, 255, 191);

	gluPerspective(70.0, 256.0 / 192.0, 0.1, 40.0);
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);

	gluLookAt(
		0.0, 0.0, 1.0, //cam pos
		0.0, 0.0, 0.0, //look at
		0.0, 1.0, 0.0  //up
	);
	Coord tmp = {0, 0};
	lineRay.a = tmp;

	iprintf("NDS RT demo\n");
 
	while(1) {
		scanKeys();
		int keys = keysDown();
		if (keys & KEY_TOUCH) touchRead(&touch);

		// print at using ansi escape sequence \x1b[line;columnH 
		iprintf("\x1b[2;0HFrame = %d",frame);
		iprintf("\x1b[23;31H@");

		Coord tmp = {touch.px, touch.py};
		lineRay.b = convertNDSCoordsToGL(tmp);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		Coord closestIntersect = nullCoord;

		for (int i = 0; i < sizeof(segments) / sizeof(Square); i++)
		{
			Ray segmentRays[] = {
				{segments[i].a, segments[i].b}, // A to B
				{segments[i].b, segments[i].c}, // B to C
				{segments[i].c, segments[i].d}, // C to D
				{segments[i].d, segments[i].a}, // D to A
			};
			for (int t = 0; t < sizeof(segmentRays)/sizeof(Ray); t++)
			{
				Coord intersect = getIntersection(lineRay, segmentRays[t]);
				if (isCoordNull(intersect))
					continue;			
				if (isCoordNull(closestIntersect))
					closestIntersect = intersect;
				if(intersect.param < closestIntersect.param){
					closestIntersect = intersect;
				}
			}
		}

		iprintf("\x1b[3;0HIntersect %f, %f", closestIntersect.x, closestIntersect.y);
		renderLine(closestIntersect);

		renderSegments();

		glFlush(0);
		swiWaitForVBlank();
	}

	return 0;
}
