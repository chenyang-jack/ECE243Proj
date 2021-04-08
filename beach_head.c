//
// Created by lijip on 2021-04-07.
//
/* This files provides address values that exist in the system */

#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* VGA colors */
#define BLACK 0x0000
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00

#define ABS(x) (((x) > 0) ? (x) : -(x))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 2
#define NUM_BOXES 8

#define FALSE 0
#define TRUE 1

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int turretDx=0;
int last_turretDx=0;

volatile int pixel_buffer_start; // global variable

void clear_screen();
void draw_picture(int posX, int posY, int picWidth,int picLength, int pictureArray[]);
void draw_line(int x0, int y0, int x1, int y1, short int line_color);
void draw_box(int x, int y, int size, short int box_color);
void swap(int* x_ptr, int* y_ptr);
void plot_pixel(int x, int y, short int line_color);
void wait_for_vsync();

int main(void){
    int bullet[10][2];
    int turretPositionX =160;
    int turretPositionY =230;
    volatile int *pushButton=0xFF200050;
    int value;
    int left=0;
    int right=0;
    srand (time(NULL));
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the
    // front buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen();

    while(1){
        //Erase any boxes and lines that were drawn in the last iteration


        draw_box(turretPositionX-last_turretDx,turretPositionY,9,BLACK);

        // code for drawing the boxes and lines (not shown)
        // code for updating the locations of boxes (not shown)
        value=*pushButton;
        if(value==0x2){
            right=1;
            left=0;
        }else if(value==0x4){
            right=0;
            left=1;
        }else{
            right=0;
            left=0;
        }

        turretPositionX+=turretDx;
        last_turretDx=turretDx;
        if(right==1){
            turretDx=1;
        }else if(left==1){
            turretDx=-1;
        }else{
            turretDx=0;
        }
        if(turretPositionX>=309){
            if(left==1){
                turretDx=-1;
            }else{
                turretDx=0;
            }
        }
        if(turretPositionX<=10){
            if(right==1){
                turretDx=1;
            }else{
                turretDx=0;
            }
        }
        draw_box(turretPositionX,turretPositionY,9,WHITE);

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}
// code for subroutines (not shown)
void clear_screen(){
    volatile int * pixel_ctrl_ptr = (int *)PIXEL_BUF_CTRL_BASE;
    // get max_x max_y from the resolution register
    int max_x = (*(pixel_ctrl_ptr + 2)) & 0xFFFF;
    int max_y = (*(pixel_ctrl_ptr + 2)) >> 16 & 0xFFFF;

    for(int curX = 0; curX < max_x; curX++){
        for(int curY = 0; curY < max_y; curY++){
            plot_pixel(curX, curY, BLACK);
        }
    }
}
void draw_picture(int posX, int posY, int picLength,int picWidth, int pictureArray[][picWidth]){
    for(int i = 0; i < picLength; i++){
        for(int j = 0; j < picWidth; j++){
            plot_pixel(i + posX, j + posY, pictureArray[i][j]);
        }
    }
}
void draw_box(int x, int y, int size, short int box_color){
    for(int i = x-(size/2); i <= x + (size/2); i++){
        for(int j = y - (size/2); j <= y+(size/2); j++){
            if(i <= RESOLUTION_X && i>=0 && j <= RESOLUTION_Y && j>=0)
                plot_pixel(i, j, box_color);
        }
    }
}
void draw_line(int x0, int y0, int x1, int y1, short int line_color){
    if(x0 > RESOLUTION_X || x0 < 0) return;
    if(x1 > RESOLUTION_X || x1 < 0) return;
    if(y0 > RESOLUTION_Y || y0 < 0) return;
    if(y1 > RESOLUTION_Y || y1 < 0) return;

    int is_steep = abs(y1 - y0) > abs(x1 - x0);
    if (is_steep == TRUE){
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1){
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax / 2);

    int y = y0;
    int x;
    int y_step;

    if (y0 < y1) y_step = 1;
    else y_step = -1;

    for (x = x0 ; x < x1; x++){
        if (is_steep) plot_pixel(y, x, line_color);
        else plot_pixel(x, y, line_color);

        error = error + deltay;
        if (error >= 0){
            y = y + y_step;
            error = error - deltax;
        }
    }
}

void swap(int* x_ptr, int* y_ptr){
    (*x_ptr) = (*x_ptr) ^ (*y_ptr);
    (*y_ptr) = (*x_ptr) ^ (*y_ptr);
    (*x_ptr) = (*y_ptr) ^ (*x_ptr);
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}
void wait_for_vsync(){
    volatile int * pixel_ctrl_ptr = PIXEL_BUF_CTRL_BASE;
    register int status;

    *pixel_ctrl_ptr = 1;

    status = *(pixel_ctrl_ptr + 3);
    while((status & 0x01)!=0){
        status = *(pixel_ctrl_ptr + 3);
    }
}
typedef struct {
    int **array;
    size_t used;
    size_t size;
} Array;

void initArray(Array *a, size_t initialRowSize, size_t initialColSize) {
    a->array = (int**)malloc(initialRowSize * sizeof(int));
    for(int i=0;i<initialRowSize;i++){
        a->array[i]=(int*)malloc(initialColSize* sizeof(int));
    }
    a->used = 0;
    a->size = initialRowSize;
}

void insertArray(Array *a, int x, int y) {
    // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
    // Therefore a->used can go up to a->size
    if (a->used == a->size) {
        a->size *= 2;
        a->array = realloc(a->array, a->size * sizeof(int));
    }
    a->array[a->used++][0] = x;
    a->array[a->used++][1] = y;
}
void deleteArray(Array *a,int index){
    a->used--;
    for(int i=index;i<used;i++){
        int tempX=a->array[i+1][0];
        int tempY=a->array[i+1][1];
        a->array[i][0] = tempX;
        a->array[i][1] = tempY;
    }
}
void freeArray(Array *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}
