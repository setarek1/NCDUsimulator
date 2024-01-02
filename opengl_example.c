#include <stdio.h>
#include <GLUT/glut.h>
#include <OpenGL/gl.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400

typedef struct {
    float x, y, width, height;
    const char* text;
    int clicked;
    const char* message;
} Button;

Button buttons[3] = {
    {100, 100, 100, 50, "Button 1", 0, "Button 1 Clicked!"},
    {250, 100, 100, 50, "Button 2", 0, "Button 2 Clicked!"},
    {400, 100, 100, 50, "Button 3", 0, "Button 3 Clicked!"}
};

void drawText(const char* text, float x, float y) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text++);
    }
}

void drawButtons() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < 3; ++i) {
        glColor3f(0.5f, 0.5f, 0.5f); // Gray color for button
        glBegin(GL_QUADS);
            glVertex2f(buttons[i].x, buttons[i].y);
            glVertex2f(buttons[i].x + buttons[i].width, buttons[i].y);
            glVertex2f(buttons[i].x + buttons[i].width, buttons[i].y + buttons[i].height);
            glVertex2f(buttons[i].x, buttons[i].y + buttons[i].height);
        glEnd();

        drawText(buttons[i].text, buttons[i].x + 10, buttons[i].y + 20);
    }

    for (int i = 0; i < 3; ++i) {
        if (buttons[i].clicked) {
            glColor3f(0.0f, 0.0f, 0.0f); // Black text color
            drawText(buttons[i].message, 10, 30);
        }
    }

    glFlush();
}

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        y = WINDOW_HEIGHT - y; // Invert y-coordinate to match OpenGL space

        for (int i = 0; i < 3; ++i) {
            if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].width &&
                y >= buttons[i].y && y <= buttons[i].y + buttons[i].height) {
                buttons[i].clicked = 1;
                printf("%s clicked!\n", buttons[i].text);
            } else {
                buttons[i].clicked = 0;
            }
        }
        glutPostRedisplay();
    }
}

void display() {
    drawButtons();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("OpenGL Buttons Example");

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Set clear color to white
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);

    glutDisplayFunc(display);
    glutMouseFunc(mouseClick);

    glutMainLoop();
    return 0;
}
