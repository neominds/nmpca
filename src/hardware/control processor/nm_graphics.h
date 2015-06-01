/****************

Graphical library functions

*********************/

#ifndef NM_GRAPHICS_H
#define NM_GRAPHICS_H
#define DISP_Y 25
#define DISP_X 100 
char memory[DISP_Y][DISP_X];
void  setpixel(int x,int y);
void line(int x0,int y0,int x1,int y1,int color);
void verti_lineT(int x0,int y0,int x1,int y1,int color);
void verti_lineD(int x0,int y0,int x1,int y1,int color);  //vertical down
void hori_linel(int x0,int y0,int x1,int y1,int color); //hori left
void hori_liner(int x0,int y0,int x1,int y1,int color); //hori right
void  rect(int x0,int y0,int x1,int y1);
void fill_rect(int x0,int y0,int x1,int y1);
void initialize();
void cleardisplay();
void display();

#endif
