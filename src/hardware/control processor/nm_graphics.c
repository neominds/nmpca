/* 
		Graphics source code
*/

#include<stdio.h>
#include"nm_graphics.h"

void 
setpixel(int x,int y)
{
	int i,j;
	for(i=0;i<DISP_Y;i++)
	{
		for(j=0;j<DISP_X;j++)
		{
			if(i==x && j==y)
			{
				memory[i][j]='#';
				break;
			}
			
		}
	}
}
void
line(int x0,int y0,int x1,int y1,int color)
{
	int i,j;
	for(i=x0;i<x1;i++)
	{
		setpixel(y0,i);// vertical line
		
	}
}
void
verti_lineT(int x0,int y0,int x1,int y1,int color)
{
	int i,j;
	for(i=x0;i<=x0+x1;i++)
	{
		setpixel(y0,i);// vertical line
		
	}
}

void
verti_lineD(int x0,int y0,int x1,int y1,int color)  //vertical down
{
	int i,j,w=0,h=0;
	for(i=x0;i<=x0+x1;i++)
	{
		setpixel(y1+y0,i);// vertical line
		
	}
}

void
hori_linel(int x0,int y0,int x1,int y1,int color) //hori left
{
	int i,j;
	for(i=y0;i<=y0+y1;i++)
	{
		setpixel(i,x0);// horizontal line
	}
}

void
hori_liner(int x0,int y0,int x1,int y1,int color) //hori right
{
	int i,j;
	for(i=y0;i<=y0+y1;i++)
	{
		setpixel(i,x1+x0);// horizontal line
	}
}
void 
rect(int x0,int y0,int x1,int y1)
{
	int i,j,w,h;
	verti_lineT(x0,y0,x1,y1,0);
	verti_lineD(x0,y0,x1,y1,0);
	hori_linel(x0,y0,x1,y1,0);
	hori_liner(x0,y0,x1,y1,0);

//	verti_lineT(X0,Y0,X1,Y1,0);
//	verti_lineD(X0,Y0,X1,Y1,0);
//	hori_linel(X0,Y0,X1,Y1,0);
//	hori_liner(X0,Y0,X1,Y1,0);

}
void
fill_rect(int x0,int y0,int x1,int y1)
{
	int i,j,w,h;
	for(i=x0;i<x0+x1;i++)
	hori_linel(i,y0,0,y1-1,0);
	//for(i=0;i<)
}

void
initialize()
{
	int i,j;
	for(i=0;i<DISP_Y;i++)
	{
		for(j=0;j<DISP_X;j++)
		{
			memory[i][j]=' ';
			//printf(".");
		}
		//printf("\n");
	}


}

void
cleardisplay()
{
	int i,j;
	for(i=0;i<DISP_Y;i++)
	{
		for(j=0;j<DISP_X;j++)
		{
			memory[i][j]=' ';
			//printf(".");
		}
		//printf("\n");
	}


}


void
display()
{
	int i,j;
	for(i=0;i<DISP_Y;i++)
	{
		for(j=0;j<DISP_X;j++)
		{
			printf("%c",memory[i][j]);
		}
		printf("\n");
	}
}

