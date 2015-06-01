#include<stdio.h>
#include"nm_graphics.h"
main()
{
	initialize();
	rect(0,0,99,24);// draw a rectangle
	fill_rect(2,2,80,15); // fill the rectangle
	line(3,20,90,10,0);
	display();
}
