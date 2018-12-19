//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2018-12-05
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//
#include "gl_frontEnd.h"

//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);
void moveBox(void);
void moveRobot(void);


//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern const int	GRID_PANE, STATE_PANE;
extern const int 	GRID_PANE_WIDTH, GRID_PANE_HEIGHT;
extern int	gMainWindow, gSubwindow[2];

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
int** grid;
int numRows = -1;	//	height of the grid
int numCols = -1;	//	width
int numBoxes = -1;	//	also the number of robots
int numDoors = -1;	//	The number of doors.

int numLiveThreads = 0;		//	the number of live robot threads

//	robot sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int robotSleepTime = 100000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;

int **robotLoc;
int **boxLoc;
int *doorAssign;	//	door id assigned to each robot-box pair
int **doorLoc;

//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================


void displayGridPane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0, GRID_PANE_HEIGHT, 0);
	glScalef(1.f, -1.f, 1.f);


	//	normally, here I would initialize the location of my doors, boxes,
	//	and robots, and create threads (not necessarily in that order).
	//	For the handout I have nothing to do.

	for (int i=0; i<numBoxes; i++)
	{
		//	here I would test if the robot thread is still live
		//						row				column			row			column
		drawRobotAndBox(i, robotLoc[i][0], robotLoc[i][1], boxLoc[i][0], boxLoc[i][1], doorAssign[i]);
	}

	for (int i=0; i<numDoors; i++)
	{
		//	here I would test if the robot thread is still alive
		//				row				column	
		drawDoor(i, doorLoc[i][0], doorLoc[i][1]);
	}

	moveBox();


	//	This call does nothing important. It only draws lines
	//	There is nothing to synchronize here
	drawGrid();

	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

void displayStatePane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[STATE_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//	Here I hard-code a few messages that I want to see displayed
	//	in my state pane.  The number of live robot threads will
	//	always get displayed.  No need to pass a message about it.
	int numMessages = 3;
	sprintf(message[0], "We have %d doors", numDoors);
	sprintf(message[1], "I like cheese");
	sprintf(message[2], "System time is %ld", time(NULL));
	
	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
	drawState(numMessages, message);
	
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupRobots(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * robotSleepTime) / 10;
	
	if (newSleepTime > MIN_SLEEP_TIME)
	{
		robotSleepTime = newSleepTime;
	}
}

void slowdownRobots(void)
{
	//	increase sleep time by 20%
	robotSleepTime = (12 * robotSleepTime) / 10;
}




//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	the initialization of numRows, numCos, numDoors, numBoxes.
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//	We know that the arguments  of the program  are going
	//	to be the width (number of columns) and height (number of rows) of the
	//	grid, the number of boxes (and robots), and the number of doors.
	//	You are going to have to extract these.  For the time being,
	//	I hard code-some values
	numRows = atoi(argv[1]);
	numCols = atoi(argv[2]);
	numDoors = atoi(argv[3]);
	numBoxes = atoi(argv[4]);

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);
	
	//	Now we can do application-level initialization
	initializeApplication();

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (int i=0; i< numRows; i++)
		free(grid[i]);
	free(grid);
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);

	
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//
//	This is a part that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void)
{
	//	Allocate the grid
	grid = (int**) malloc(numRows * sizeof(int*));
	for (int i=0; i<numRows; i++)
		grid[i] = (int*) malloc(numCols * sizeof(int));
	
	message = (char**) malloc(MAX_NUM_MESSAGES*sizeof(char*));
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = (char*) malloc((MAX_LENGTH_MESSAGE+1)*sizeof(char));
		
	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	simulation), only some color, in meant-to-be-thrown-away code

	//	seed the pseudo-random generator
	srand((unsigned int) time(NULL));
	robotLoc = (int **) malloc(numBoxes * sizeof(int *));
	for(int i = 0; i < numBoxes; i++)
	{
		robotLoc[i] = (int *) malloc(2 * sizeof(int));
		robotLoc[i][0] = rand() % numRows - 1;
		robotLoc[i][1] = rand() % numCols - 1;

		//printf("Robot Row: %d\t Col: %d\n", robotLoc[i][0], robotLoc[i][1]);
	}

	boxLoc = (int **) malloc(numBoxes * sizeof(int *));
	for(int i = 0; i < numBoxes; i++)
	{
		boxLoc[i] = (int *) malloc(2 * sizeof(int));
		boxLoc[i][0] = (rand() % ((numRows - 1) - 2 + 1)) + 1;
		boxLoc[i][1] = (rand() % ((numCols - 1) - 2 + 1)) + 1;
		//printf("Box Row: %d\t Col: %d\n", boxLoc[i][0], boxLoc[i][1]);
	}

	doorAssign = (int *) malloc(numBoxes * sizeof(int));
	for(int i = 0; i < numBoxes; i++)
	{
		doorAssign[i] = (rand() % numDoors);
		//printf("Col: %d\n", doorAssign[i]);
	}

	doorLoc = (int **) malloc(numDoors * sizeof(int *));
	for(int i = 0; i < numDoors; i++)
	{
		doorLoc[i] = (int *) malloc(2 * sizeof(int));
		doorLoc[i][0] = rand() % numRows;
		doorLoc[i][1] = rand() % numCols;
		//printf("Door Row: %d\t Col: %d\n", doorLoc[i][0], doorLoc[i][1]);
	}
}

void moveBox(void)
{
	for(int i = 0; i < numBoxes; i++)
	{
		// Box is not in the x position of the door
		if(boxLoc[i][1] != doorLoc[doorAssign[i]][1])
		{
			// Box is to the left of the door
			if(doorLoc[doorAssign[i]][1] - boxLoc[i][1] < 0)
			{
				// Robot is not on the right side of the box
				if(robotLoc[i][1] != boxLoc[i][1] + 1)
				{
					// Move robot to the right
					if(robotLoc[i][1] > boxLoc[i][1] + 1)
						robotLoc[i][1]--;
					// Move robot to the left
					else
						robotLoc[i][1]++;
				}
				// Robot is on the right side of the box
				else
				{
					// Robot is not on same y position, so move it to that position
					if(robotLoc[i][0] < boxLoc[i][0])
						robotLoc[i][0]++;
					else if (robotLoc[i][0] > boxLoc[i][0])
						robotLoc[i][0]--;
					// Move the box and the robot, they are now both in correct spot
					else
					{
						robotLoc[i][1]--;
						boxLoc[i][1]--;
					}
				}
			}
			// Box is to the right of the door
			else
			{
				// Robot is not on the left side of the box
				if(robotLoc[i][1] != boxLoc[i][1] - 1)
				{
					// Move robot to the left
					if(robotLoc[i][1] > boxLoc[i][1] - 1)
						robotLoc[i][1]--;
					// Move robot to the right
					else
						robotLoc[i][1]++;
				}
				// Robot is on the left side of the box
				else
				{
					// Robot is not aligned with y, so do that
					if(robotLoc[i][0] < boxLoc[i][0])
						robotLoc[i][0]++;
					else if (robotLoc[i][0] > boxLoc[i][0])
						robotLoc[i][0]--;
					// Move the box
					else
					{
						robotLoc[i][1]++;
						boxLoc[i][1]++;
					}
				}
			}
		}
		else if( boxLoc[i][0] != doorLoc[doorAssign[i]][0])
		{
			// Box is above the door
			if(doorLoc[doorAssign[i]][0] - boxLoc[i][0] < 0)
			{
				// Robot is not above the box
				if(robotLoc[i][0] != boxLoc[i][0] - 1)
				{
					// Move robot down
					if(robotLoc[i][0] > boxLoc[i][0] - 1)
						robotLoc[i][0]--;
					// Move robot up
					else
						robotLoc[i][0]++;
				}
				// Robot is in right y position
				else
				{
					// Robot is not on same x position, so move it to that position
					if(robotLoc[i][1] < boxLoc[i][1])
						robotLoc[i][1]++;
					else if (robotLoc[i][1] > boxLoc[i][1])
						robotLoc[i][1]--;
					// Move the box and the robot, they are now both in correct spot
					else
					{
						robotLoc[i][0]--;
						boxLoc[i][0]--;
					}
				}
			}
			// Box is below the door
			else
			{
				// Robot is not below
				if(robotLoc[i][0] != boxLoc[i][0] + 1)
				{
					// Move robot to the left
					if(robotLoc[i][0] > boxLoc[i][0] + 1)
						robotLoc[i][0]--;
					// Move robot to the right
					else
						robotLoc[i][0]++;
				}
				// Robot is on the left side of the box
				else
				{
					// Robot is not aligned with y, so do that
					if(robotLoc[i][1] < boxLoc[i][1])
						robotLoc[i][1]++;
					else if (robotLoc[i][1] > boxLoc[i][1])
						robotLoc[i][1]--;
					// Move the box
					else
					{
						robotLoc[i][0]++;
						boxLoc[i][0]++;
					}
				}
			}
		}
	}
}

void moveRobot(void)
{
	
}









