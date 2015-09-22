#include <stdlib.h>
#include <math.h>
#include <GLUT/glut.h>  /* Header File For The GLut Library*/

/* OpenGL.   This first example shows the use of the OpenGl library
for some simple graphics. */

/*
gcc -o glb gl.c -framework GLUT -framework OpenGL \
-L"/System/Library/Frameworks/OpenGL.framework/Libraries" \
-lGL -lGLU -lm
*/

void display(void)
{
	int i; double angle;
	glClear(GL_COLOR_BUFFER_BIT);
	for (i=0; i<360; i+=4) {
		glColor3f((float)i/360.0,1.0,1.0);
		glBegin(GL_LINES);
		glVertex2d(cos(i/57.25779),sin(i/57.25779));
		glVertex2d(cos((i+90)/57.25779),sin((i+90)/57.25779));
		glEnd();

		glColor3f(1.0, (float)i/360.0,1.0);
		glBegin(GL_LINES);
		glVertex2d(cos(i/57.25779),sin(i/57.25779));
		glVertex2d(cos((i*2)/57.25779),sin((i+90)/57.25779));
		glEnd();
	}
	glLoadIdentity();
	glutSwapBuffers();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize (300,300);
	glutInitWindowPosition (100, 100);
	glutCreateWindow ("OpenGL / C Example - Well House");

	glutDisplayFunc(display);

	glutMainLoop();

	return 0;
}
