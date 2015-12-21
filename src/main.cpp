#include <GL/freeglut.h>
#include <iostream>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// Flag telling us to keep executing the main loop
static int continue_in_main_loop = 1;

static void displayProc(void) {
  glClear(GL_COLOR_BUFFER_BIT);

  glPushMatrix();

  glColor4f(0.5f, 0.0, 0.0, 1.0f);
  glBegin(GL_TRIANGLES);
    glVertex3f(0.0, 1.0, 0.0);
    glVertex3f(1.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 0.0);
  glEnd();

  glPopMatrix();
  glutSwapBuffers();
}

static void keyProc(unsigned char key, int x, int y) {
  int need_redisplay = 1;

  switch (key) {
    case 27:  // Escape key
      continue_in_main_loop = 0;
      break;

    default:
      need_redisplay = 0;
      break;
  }
  if (need_redisplay)
    glutPostRedisplay();
}

static void reshapeProc(int width, int height) {
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, width, height);
  glutPostRedisplay();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  int argc = 0;
  char **argv = nullptr;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(200, 200);
  glutInitWindowPosition(140, 140);
  glutCreateWindow("filter");

  glutKeyboardFunc(keyProc);
  glutDisplayFunc(displayProc);
  glutReshapeFunc(reshapeProc);

  while (continue_in_main_loop)
    glutMainLoopEvent();

  return 0;
}