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
#include <pthread.h>
#include <string.h>	        //	for memcpy
#include <sys/stat.h>	//stat struct used for program output
#include <dirent.h>	
#include <sys/types.h>		//  for gettid() on Linux	
#include <unistd.h>			//  for usleep()
//
#include "gl_frontEnd.h"

#define PARAMS 1
#define DOORS 3
#define BOXES 5
#define ROBOTS 7
#define PUSH 9

/**
 * Struct used to store all of the information required for a robot
 * (controlled by a thread) to push it's designated box to it's 
 * destination door.
 */ 
struct robot
{
	pthread_t robotThread;	//The pthread designated to the robot represented by this struct
	int robotNum,			//The number of the robot
		robotRow,			//The robot's row
		robotCol,			//The robot's column
		boxRow,				//The row of the box to be pushed the robot
		boxCol,				//The column of the box to be pushed by the robot
		doorNum,			//The door designated to the robot
		doorRow,			//The row of the robot's destination door
		doorCol,			//The column of the robot's destination door
		reached;			//0 if the box has yet to reach the door, 1 otherwise
}; 


//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane();
void displayStatePane(void);
void initializeApplication(void);

/**
 * Function called upon creation of a thread to move each robot's box
 * to it's assigned door.
 * @param	void *argument	The robot struct passed during thread creation
 */ 
void* moveBox(void *argument);

/**
 * Function used to print the direction a box is pushed or moved
 * by a robot. Also used to print when the box has reached it's destination.
 * @param	int robotNum			The number of the robot performing the movement
 * @param	Direction direction		Enum type defined in gl_frontEnd.h to indicate cardinal directions
 * @param	int isPushed			0 if the box is moved, 1 if the box is pushed
 */ 
void printMovement(int robotNum, Direction direction, int isPushed);

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
// int robotSleepTime = 100000;
int robotSleepTime = 1000;

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

int liveThreads;

pthread_mutex_t fileLock;

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

	numLiveThreads = numBoxes;


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

	//Create a thread specific to each robot
	for(int i = 0; i < numBoxes; i++)
	{
		//Create a thread, store it in the robots struct and call moveBox with the current robot as the parameter
		int result = pthread_create(&(*robots)[i]->robotThread, NULL, &moveBox, (*robots)[i]);

		//error handling, written by Professor Jean-Yves Hervé
		if (result != 0)
		{
			printf ("could not pthread_create thread %d. %d/%s\n",
					i, result, strerror(result));
			exit (EXIT_FAILURE);
		}
	}

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


void printMovement(int robotNum, Direction direction, int isPushed)
{
	//lock the file
	pthread_mutex_lock(&fileLock);

	//Switch case dependent on cardinal direction of movement
	switch(direction)
	{
		//box moved/pushed north
		case NORTH:
		{
			if(!isPushed)
				fprintf(fp, "robot %d move N\n", robotNum);
			else
				fprintf(fp, "robot %d push N\n", robotNum);
		}
		break;

		//box moved/pushed west
		case WEST:
		{
			if(!isPushed)
				fprintf(fp, "robot %d move W\n", robotNum);
			else
				fprintf(fp, "robot %d push W\n", robotNum);
		}
		break;

		//box moved/pushed east
		case EAST:
		{
			if(!isPushed)
				fprintf(fp, "robot %d move E\n", robotNum);
			else
				fprintf(fp, "robot %d push E\n", robotNum);
		}
		break;
		
		//box moved/pushed south
		case SOUTH:
		{
			if(!isPushed)
				fprintf(fp, "robot %d move S\n", robotNum);
			else
				fprintf(fp, "robot %d push N\n", robotNum);
		}
		break;

		//box is at destination
		case END:
		{
			fprintf(fp, "robot %d end\n", robotNum);
		}
		break;

		default:
			break;
	}

	//unlock the file
	pthread_mutex_unlock(&fileLock);
}


void* moveBox(void *argument) 
{
		//Define the argument to be a robot struct for the current thread
		struct robot *robotThread = argument;

		//If the robot has yet to push the box to the door
		if(!robotThread->reached)
		{
			// Box is not in the x position of the door
			if(robotThread->boxCol != robotThread->doorCol)
			{
				// Box is to the left of the door
				if(robotThread->doorCol - robotThread->boxCol < 0)
				{
					// Robot is not on the right side of the box
					if(robotThread->robotCol != robotThread->boxCol + 1)
					{
						// Move robot to the East
						if(robotThread->robotCol > robotThread->boxCol + 1)
						{
							printMovement(robotThread->robotNum, EAST, 0);
							robotThread->robotCol--;
						}
						// Move robot to the West
						else
						{
							printMovement(robotThread->robotNum, WEST, 0);
							robotThread->robotCol++;
						}
					}
					// Robot is on the right side of the box
					else
					{
						// Robot is not on same y position, so move it to that position
						if(robotThread->robotRow < robotThread->boxRow)
						{
							// Robot moves South
							printMovement(robotThread->robotNum, SOUTH, 0);
							robotThread->robotRow++;
						}
						else if (robotThread->robotRow > robotThread->boxRow)
						{
							// Robot moves North
							printMovement(robotThread->robotNum, NORTH, 0);
							robotThread->robotRow--;
						}
						// Move the box and the robot, they are now both in correct spot
						else
						{
							// Push the box West
							printMovement(robotThread->robotNum, WEST, 1);
							robotThread->robotCol--;
							robotThread->boxCol--;
						}
					}
				}
				// Box is to the right of the door
				else
				{
					// Robot is not on the left side of the box
					if(robotThread->robotCol != robotThread->boxCol - 1)
					{
						// Move robot to the West
						if(robotThread->robotCol > robotThread->boxCol - 1)
						{
							printMovement(robotThread->robotNum, WEST, 0);
							robotThread->robotCol--;
						}
						// Move robot to the East
						else
						{
							printMovement(robotThread->robotNum, EAST, 0);
							robotThread->robotCol++;
						}
					}
					// Robot is on the left side of the box
					else
					{
						// Robot is not aligned with y, so do that
						if(robotThread->robotRow < robotThread->boxRow)
						{
							// Move robot North
							printMovement(robotThread->robotNum, NORTH, 0);
							robotThread->robotRow++;
						}
						else if (robotThread->robotRow > robotThread->boxRow)
						{
							// Move robot South
							printMovement(robotThread->robotNum, SOUTH, 0);
							robotThread->robotRow--;
						}
						// Move the box
						else
						{
							// Move the box to the East
							printMovement(robotThread->robotNum, EAST, 1);
							robotThread->robotCol++;
							robotThread->boxCol++;
						}
					}
				}
			}
			else if( robotThread->boxRow != robotThread->doorRow)
			{
				// Box is above the door
				if(robotThread->doorRow - robotThread->boxRow < 0)
				{
					// Robot is not above the box
					if(robotThread->robotRow != robotThread->boxRow + 1)
					{
						// Move robot North
						if(robotThread->robotRow > robotThread->boxRow + 1)
						{
							printMovement(robotThread->robotNum, NORTH, 0);
							robotThread->robotRow--;
						}
						// Move robot South
						else
						{
							printMovement(robotThread->robotNum, SOUTH, 0);
							robotThread->robotRow++;
						}
					}
					// Robot is in right y position
					else
					{
						// Robot is not on same x position, so move it to that position
						if(robotThread->robotCol < robotThread->boxCol)
						{
							// Moving the robot East
							printMovement(robotThread->robotNum, EAST, 0);
							robotThread->robotCol++;
						}
						else if (robotThread->robotCol > robotThread->boxCol)
						{
							// Moving the West
							printMovement(robotThread->robotNum, WEST, 0);
							robotThread->robotCol--;
						}
						// Move the box and the robot, they are now both in correct spot
						else
						{
							// Moving the box North
							printMovement(robotThread->robotNum, NORTH, 1);
							robotThread->robotRow--;
							robotThread->boxRow--;
						}
					}
				}
				// Box is below the door
				else
				{
					// Robot is not below
					if(robotThread->robotRow != robotThread->boxRow - 1)
					{
						// Move robot to the left
						if(robotThread->robotRow > robotThread->boxRow - 1)
						{
							// Moving the robot West
							printMovement(robotThread->robotNum, WEST, 0);
							robotThread->robotRow--;
						}
						// Move robot to the right
						else
						{
							// Moving robot East
							printMovement(robotThread->robotNum, EAST, 0);
							robotThread->robotRow++;
						}
					}
					// Robot is on the left side of the box
					else
					{
						// Robot is not aligned with y, so do that
						if(robotThread->robotCol < robotThread->boxCol)
						{
							// Move robot South
							printMovement(robotThread->robotNum, SOUTH, 0);
							robotThread->robotCol++;
						}
						else if (robotThread->robotCol > robotThread->boxCol)
						{
							// Move robot North
							printMovement(robotThread->robotNum, NORTH, 0);
							robotThread->robotCol--;
						}
						// Move the box
						else
						{
							// Moving the box South
							printMovement(robotThread->robotNum, SOUTH, 1);
							robotThread->robotRow++;
							robotThread->boxRow++;
						}
					}
				}
			}
			else	//The door has been reached
			{	
				//If the reached value in the struct is still 0
				if(!robotThread->reached)
				{
					printMovement(robotThread->robotNum, END, 0);
					//flip the bool
					robotThread->reached = 1;
					//decrement live threads
					numLiveThreads--;
				}
			}
		}
		
	return NULL;
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

	//	seed the pseudo-random generator
	srand((unsigned int) time(NULL));


	//Initialize locations of all the doors
	doorLoc = (int **) malloc(numDoors * sizeof(int *));
	for(int i = 0; i < numDoors; i++)
	{
		doorLoc[i] = (int *) malloc(2 * sizeof(int));
		doorLoc[i][0] = rand() % numRows;
		doorLoc[i][1] = rand() % numCols;
	}

	//Initialize the robot structs, numRobots = numBoxes
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

		//Store the location of the robot's destination door
		for(int j = 0; j < numDoors; j++)
		{
			if((*robots)[i]->doorNum == j)
			{
				(*robots)[i]->doorRow = doorLoc[j][0];
				(*robots)[i]->doorCol = doorLoc[j][1];
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
		
		//line counter
		int lineNum = 1;

		//	(Input Params + Line) + (numDoors + line) + (numBoxes + line) + (numRobots=numBoxes + line)
		int initialLineCount = 2 + (numDoors + 1) + 2*(numBoxes + 1); 

		//Loop while there is still initial information to be written to the outfile
		while(lineNum < initialLineCount)
		{
			//If the lineNumber is even print a blank line
			if(lineNum % 2 == 0 && lineNum < initialLineCount)
			{
				fprintf(fp, "\n");
				lineNum++;
			}
			//Switch case dependent on the line at which each piece of information should be printed
			else
			{
				switch(lineNum)
				{
					//Input params always printed on line 1
					case PARAMS:
						fprintf(fp, "Input Parameters: \tNumber of Rows: %d Number of Columns: %d Number of Boxes: %d Number of Doors: %d Number of Robots %d\n", numRows, numCols, numDoors, numBoxes, numBoxes);
						break;

					//Door information will always be printed on line 3
					case DOORS:
						//Write information of each door to file
						while( lineNum < DOORS + numDoors)
						{
							//index of current door is equal to lineNum - DOORS (i.e the first door is accessed at index 5 - 5 = 0)
							fprintf(fp, "Door %d spawned at (row %d, column %d)\n", (lineNum - DOORS), doorLoc[lineNum - DOORS][0], doorLoc[lineNum - DOORS][1]);
							lineNum++;
						}
						//Decrement lineNum by numDoors to ensure constants are still meaningful
						lineNum -= numDoors;
						break;
					
					//Box information printed at line 5 + offset of however many doors were previously printed
					case BOXES:
						//write information of each box to file
						while( lineNum < BOXES + numBoxes)
						{
							//box info is accessed in exact same manner as door info
							fprintf(fp, "Box %d spawned at (row %d, column %d)\n", (*robots)[lineNum - BOXES]->robotNum, (*robots)[lineNum - BOXES]->boxRow, (*robots)[lineNum - BOXES]->boxCol);
							lineNum++;
						}
						//Decrement lineNum by number of boxes
						lineNum -= numBoxes;
					    break;
					
					//Robot information printed at line 7 + offset of how many boxes and doors were previously printed
					case ROBOTS:
						//Write initial spawn points of robots and their destination doors
						while( lineNum < ROBOTS + numBoxes)
						{
							//robot info is accessed in exact same manner as box and door info
							fprintf(fp, "Robot %d spawned at (row %d, column %d) with destination door %d\n", (*robots)[lineNum - ROBOTS]->robotNum, (*robots)[lineNum - ROBOTS]->robotRow, (*robots)[lineNum - ROBOTS]->robotCol, (*robots)[lineNum - ROBOTS]->doorNum);
							lineNum++;
						}
						//Add back the previously decrement offsets to ensure while loop exits
						lineNum += (numDoors + 2*numBoxes);
					    break;

				}//end switch 
				
				//increment lineCount
				lineNum++;
			}
		}
		//Writes a blank line to the file after all the initial information. Done to avoid 
		//checking for first time a robot's movement is appended to the file.
		fprintf(fp, "\n");
	}
}

