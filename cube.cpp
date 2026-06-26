#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>

float A, B, C;                 // rotation angles (X, Y, Z axes)
int   width = 80, height = 80; // screen size in characters
float zBuffer[80 * 80];        // depth buffer (closest point wins)
char  buffer[80 * 80];         // character frame buffer
int   distanceFromCam = 100;
float K1 = 40;                 // projection scale
float incrementSpeed = 0.4;    // surface sampling step (smaller = denser)
float horizontalOffset;

void getTerminalSize() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        width = 80;   // fallback if ioctl fails
        height = 80;
        return;
    }
    width  = ws.ws_col;
    height = ws.ws_row;

    if (width  > 80) width  = 80;
    if (height > 80) height = 80;
}

/* 3D rotation matrices, multiplied out for a point (i, j, k). */
float calculateX(float i, float j, float k) {
    return  j * sin(A) * sin(B) * cos(C) - k * cos(A) * sin(B) * cos(C)
          + j * cos(A) * sin(C) + k * sin(A) * sin(C) + i * cos(B) * cos(C);
}
float calculateY(float i, float j, float k) {
    return  j * cos(A) * cos(C) + k * sin(A) * cos(C)
          - j * sin(A) * sin(B) * sin(C) + k * cos(A) * sin(B) * sin(C)
          - i * cos(B) * sin(C);
}
float calculateZ(float i, float j, float k) {
    return k * cos(A) * cos(B) - j * sin(A) * cos(B) + i * sin(B);
}

/* Rotate one surface point, project it to 2D, and plot it if it's the
   nearest thing along that line of sight. */
void surface(float cx, float cy, float cz, char ch) {
    float x = calculateX(cx, cy, cz);
    float y = calculateY(cx, cy, cz);
    float z = calculateZ(cx, cy, cz) + distanceFromCam;
    float ooz = 1 / z;                                   // "one over z"

    int xp = (int)(width  / 2 + horizontalOffset + K1 * ooz * x * 2);
    int yp = (int)(height / 2 + K1 * ooz * y);
    int idx = xp + yp * width;

    if (idx >= 0 && idx < width * height && ooz > zBuffer[idx]) {
        zBuffer[idx] = ooz;
        buffer[idx]  = ch;
    }
}

int main(void) {
    float cubeWidth = 20;
    printf("\x1b[?1049h");                   // enable alternate screen buffer
    printf("\x1b[2J");                       // clear screen once

    while (1) {
        getTerminalSize();
        memset(buffer, ' ', width * height); // blank the frame
        memset(zBuffer, 0, sizeof(zBuffer)); // reset depths
        horizontalOffset = 0;

        // Sweep both surface coordinates and draw all six faces.
        for (float cx = -cubeWidth; cx < cubeWidth; cx += incrementSpeed)
            for (float cy = -cubeWidth; cy < cubeWidth; cy += incrementSpeed) {
                surface( cx, cy, -cubeWidth, '@');
                surface( cubeWidth, cy, cx, '$');
                surface(-cubeWidth, cy, -cx, '~');
                surface(-cx, cy,  cubeWidth, '#');
                surface( cx, -cubeWidth, -cy, ';');
                surface( cx,  cubeWidth, cy, '+');
            }

        printf("\x1b[H");                    // cursor home, redraw in place
        for (int k = 0; k < width * height; k++)
            putchar(k % width ? buffer[k] : '\n');
        fflush(stdout);

        A += 0.05;                           // advance rotation
        B += 0.05;
        C += 0.01;
        usleep(16000);                       // ~60 fps
    }
    printf("\x1b[?1049l");                   // disable alternate screen buffer
    return 0;
}

