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
#include <dirent.h>

#include <sys/stat.h>	//stat struct used for program output
#include <dirent.h>		
//
#include "gl_frontEnd.h"

#define PARAMS 1
#define DOORS 3
#define BOXES 5
#define ROBOTS 7

struct robot
{
	int robotNum,
		robotRow,
		robotCol,
		boxRow,
		boxCol,
		doorNum,
		doorRow,
		doorCol,
		reached;
	char *path;
}; 


//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane();
void displayStatePane(void);
void initializeApplication(void);
void moveBox(void);


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

struct robot *myRobots[1024];
struct robot *(*robots)[] = &myRobots;

FILE *fp = NULL;
//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================


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
	
	//Allocate memory for each of the robot structs
	for(int i = 0; i < numBoxes; i++)
	{
		myRobots[i] = malloc(sizeof(struct robot));
	}

	//	Now we can do application-level initialization
	initializeApplication();



	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	fclose(fp);

	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (int i=0; i< numRows; i++)
		free(grid[i]);
	free(grid);
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);

	// fclose(fp);
	
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


void displayGridPane() 
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0, GRID_PANE_HEIGHT, 0);
	glScalef(1.f, -1.f, 1.f);
	
	//-----------------------------
	//	CHANGE THIS
	//-----------------------------
	//	Here I hard-code myself some data for robots and doors.  Obviously this code
	//	this code must go away.  I just want to show you how to display the information
	//	about a robot-box pair or a door.
	//	Important here:  I don't think of the locations (robot/box/door) as x and y, but
	//	as row and column.  So, the first index is a row (y) coordinate, and the second
	//	index is a column (x) coordinate.



	//	normally, here I would initialize the location of my doors, boxes,
	//	and robots, and create threads (not necessarily in that order).
	//	For the handout I have nothing to do.

	for (int i=0; i<numBoxes; i++)
	{
		//	here I would test if the robot thread is still live
		//						row				column			row			column
		drawRobotAndBox((*robots)[i]->robotNum, (*robots)[i]->robotRow, (*robots)[i]->robotCol, (*robots)[i]->boxRow, (*robots)[i]->boxCol, (*robots)[i]->doorNum);
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



void moveBox(void)
{
	for(int i = 0; i < numBoxes; i++)
	{
		// Box is not in the x position of the door
		if((*robots)[i]->boxCol != (*robots)[i]->doorCol)
		{
			// Box is to the left of the door
			if((*robots)[i]->doorCol - (*robots)[i]->boxCol < 0)
			{
				// Robot is not on the right side of the box
				if((*robots)[i]->robotCol != (*robots)[i]->boxCol + 1)
				{
					// Move robot to the East
					if((*robots)[i]->robotCol > (*robots)[i]->boxCol + 1)
					{
						fprintf(fp, "robot %d move E\n", i);
						(*robots)[i]->robotCol--;
					}
					// Move robot to the West
					else
					{
						fprintf(fp, "robot %d move W\n", i);
						(*robots)[i]->robotCol++;
					}
				}
				// Robot is on the right side of the box
				else
				{
					// Robot is not on same y position, so move it to that position
					if((*robots)[i]->robotRow < (*robots)[i]->boxRow)
					{
						// Robot moves South
						fprintf(fp, "robot %d move S\n", i);
						(*robots)[i]->robotRow++;
					}
					else if ((*robots)[i]->robotRow > (*robots)[i]->boxRow)
					{
						// Robot moves North
						fprintf(fp, "robot %d move N\n", i);
						(*robots)[i]->robotRow--;
					}
					// Move the box and the robot, they are now both in correct spot
					else
					{
						// Push the box West
						fprintf(fp, "robot %d push W\n", i);
						(*robots)[i]->robotCol--;
						(*robots)[i]->boxCol--;
					}
				}
			}
			// Box is to the right of the door
			else
			{
				// Robot is not on the left side of the box
				if((*robots)[i]->robotCol != (*robots)[i]->boxCol - 1)
				{
					// Move robot to the West
					if((*robots)[i]->robotCol > (*robots)[i]->boxCol - 1)
					{
						fprintf(fp, "robot %d move W\n", i);
						(*robots)[i]->robotCol--;
					}
					// Move robot to the East
					else
					{
						fprintf(fp, "robot %d move E\n", i);
						(*robots)[i]->robotCol++;
					}
				}
				// Robot is on the left side of the box
				else
				{
					// Robot is not aligned with y, so do that
					if((*robots)[i]->robotRow < (*robots)[i]->boxRow)
					{
						// Move robot North
						fprintf(fp, "robot %d move N\n", i);
						(*robots)[i]->robotRow++;
					}
					else if ((*robots)[i]->robotRow > (*robots)[i]->boxRow)
					{
						// Move robot South
						fprintf(fp, "robot %d move S\n", i);
						(*robots)[i]->robotRow--;
					}
					// Move the box
					else
					{
						// Move the box to the East
						fprintf(fp, "robot %d push E\n", i);
						(*robots)[i]->robotCol++;
						(*robots)[i]->boxCol++;
					}
				}
			}
		}
		else if( (*robots)[i]->boxRow != (*robots)[i]->doorRow)
		{
			// Box is above the door
			if((*robots)[i]->doorRow - (*robots)[i]->boxRow < 0)
			{
				// Robot is not above the box
				if((*robots)[i]->robotRow != (*robots)[i]->boxRow + 1)
				{
					// Move robot North
					if((*robots)[i]->robotRow > (*robots)[i]->boxRow + 1)
					{
						fprintf(fp, "robot %d move N\n", i);
						(*robots)[i]->robotRow--;
					}
					// Move robot South
					else
					{
						fprintf(fp, "robot %d move S\n", i);
						(*robots)[i]->robotRow++;
					}
				}
				// Robot is in right y position
				else
				{
					// Robot is not on same x position, so move it to that position
					if((*robots)[i]->robotCol < (*robots)[i]->boxCol)
					{
						// Moving the robot East
						fprintf(fp, "robot %d move E\n", i);
						(*robots)[i]->robotCol++;
					}
					else if ((*robots)[i]->robotCol > (*robots)[i]->boxCol)
					{
						// Moving the West
						fprintf(fp, "robot %d move W\n", i);
						(*robots)[i]->robotCol--;
					}
					// Move the box and the robot, they are now both in correct spot
					else
					{
						// Moving the box North
						fprintf(fp, "robot %d push N\n", i);
						(*robots)[i]->robotRow--;
						(*robots)[i]->boxRow--;
					}
				}
			}
			// Box is below the door
			else
			{
				// Robot is not below
				if((*robots)[i]->robotRow != (*robots)[i]->boxRow - 1)
				{
					// Move robot to the left
					if((*robots)[i]->robotRow > (*robots)[i]->boxRow - 1)
					{
						// Moving the robot West
						fprintf(fp, "robot %d move W\n", i);
						(*robots)[i]->robotRow--;
					}
					// Move robot to the right
					else
					{
						// Moving robot East
						fprintf(fp, "robot %d move E\n", i);
						(*robots)[i]->robotRow++;
					}
				}
				// Robot is on the left side of the box
				else
				{
					// Robot is not aligned with y, so do that
					if((*robots)[i]->robotCol < (*robots)[i]->boxCol)
					{
						// Move robot South
						fprintf(fp, "robot %d move S\n", i);
						(*robots)[i]->robotCol++;
					}
					else if ((*robots)[i]->robotCol > (*robots)[i]->boxCol)
					{
						// Move robot North
						fprintf(fp, "robot %d move N\n", i);
						(*robots)[i]->robotCol--;
					}
					// Move the box
					else
					{
						// Moving the box South
						fprintf(fp, "robot %d push S\n", i);
						(*robots)[i]->robotRow++;
						(*robots)[i]->boxRow++;
					}
				}
			}
		}
		else
		{
			if(!(*robots)[i]->reached)
			{
				fprintf(fp, "robot %d end\n", i);
				(*robots)[i]->reached = 1;
			}
		}
	}
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

	for(int i = 0; i < numBoxes; i++)
	{
		// Initialize struct values of each robot
		(*robots)[i]->robotNum = i;
		(*robots)[i]->reached = 0;
		(*robots)[i]->robotRow = rand() % numRows-1;
		(*robots)[i]->robotCol = rand() % numCols-1;
		(*robots)[i]->doorNum = (rand() % numDoors);
		(*robots)[i]->boxRow = (rand() % ((numRows - 1) - 2 + 1)) + 1;
		(*robots)[i]->boxCol = (rand() % ((numCols - 1) - 2 + 1)) + 1;

	}

	//Initialize locations of all the doors
	doorLoc = (int **) malloc(numDoors * sizeof(int *));
	for(int i = 0; i < numDoors; i++)
	{
		doorLoc[i] = (int *) malloc(2 * sizeof(int));
		doorLoc[i][0] = rand() % numRows;
		doorLoc[i][1] = rand() % numCols;
	}

	//Store the location of the robot's destination door
	for(int j = 0; j < numBoxes; j++)
	{
		for(int i = 0; i < numDoors; i++)
		{
			if((*robots)[j]->doorNum == i)
			{
				(*robots)[j]->doorRow = doorLoc[i][0];
				(*robots)[j]->doorCol = doorLoc[i][1];
			}
		}
	}


	//Allocate memory for outputFolder name
	char outputFolder[128];
	snprintf(outputFolder, sizeof(outputFolder), "%s/%s", ".", "Output" );


	//if an Output folder hasn't been created yet	
	struct stat outputExists = {0};
	if(stat(outputFolder, &outputExists) == -1)
	{

		//creates a new output folder with read write and execute permissions enabled for the user
		mkdir(outputFolder, 0700);

		//Create a new text file to write output
		// FILE *fp = NULL;
		fp = fopen("robotSimulOut.txt", "w");
		fclose(fp);
	} 
	//Output folder exists
	else 
	{
		//opens the text file already present within the Output folder
		// FILE *fp = NULL;
		char resultFilePath[128];
		snprintf(resultFilePath, sizeof(resultFilePath), "%s/%s", outputFolder, "robotSimulOut.txt");
			
		printf("File path: %s", resultFilePath);
		//append to result text file
		fp = fopen(resultFilePath, "w");
		
		int lineNum = 1;

		//	(Input Params + Line) + (numDoors + line) + (numBoxes + line) + (numRobots=numBoxes + line)
		int initialLineCount = 2 + (numDoors + 1) + 2*(numBoxes + 1); 

		while(lineNum < initialLineCount)
		{
			if(lineNum % 2 == 0 && lineNum < initialLineCount)
			{
				fprintf(fp, "\n");
				lineNum++;
			}
			else
			{
				switch(lineNum)
				{
					//Dimensions of window
					case PARAMS:
						fprintf(fp, "Input Parameters: \tNumber of Rows: %d Number of Columns: %d Number of Boxes: %d Number of Doors: %d Number of Robots %d\n", numRows, numCols, numDoors, numBoxes, numBoxes);
						break;

					// Doors will always be printed on line 3
					case DOORS:
						while( lineNum < DOORS + numDoors)
						{
							fprintf(fp, "Door %d spawned at (row %d, column %d)\n", (lineNum - DOORS), doorLoc[lineNum - DOORS][0], doorLoc[lineNum - DOORS][1]);
							lineNum++;
						}
						lineNum -= numDoors;
						break;
					
					//Boxes
					case BOXES:
						while( lineNum < BOXES + numBoxes)
						{
							fprintf(fp, "Box %d spawned at (row %d, column %d)\n", (*robots)[lineNum - BOXES]->robotNum, (*robots)[lineNum - BOXES]->boxRow, (*robots)[lineNum - BOXES]->boxCol);
							lineNum++;
						}
						lineNum -= numBoxes;
					    break;
					
					//Write initial spawn points of robots and their destination doors
					case ROBOTS:
						while( lineNum < ROBOTS + numBoxes)
						{
							fprintf(fp, "Robot %d spawned at (row %d, column %d) with destination door %d\n", (*robots)[lineNum - ROBOTS]->robotNum, (*robots)[lineNum - ROBOTS]->robotRow, (*robots)[lineNum - ROBOTS]->robotCol, (*robots)[lineNum - ROBOTS]->doorNum);
							lineNum++;
						}
						lineNum += (numDoors + 2*numBoxes);
					    break;

				}//end switch 
				lineNum++;
			}
		}
		fprintf(fp, "\n");
	}
}

