//
//  main.c
//  Cellular Automaton
//
//  Created by Jean-Yves Hervé on 2018-04-09.
//	edited 2018-11-06

 /*-------------------------------------------------------------------------+
 |	A graphic front end for a grid+state simulation.						|
 |																			|
 |	This application simply creates a glut window with a pane to display	|
 |	a colored grid and the other to display some state information.			|
 |	Sets up callback functions to handle menu, mouse and keyboard events.	|
 |	Normally, you shouldn't have to touch anything in this code, unless		|
 |	you want to change some of the things displayed, add menus, etc.		|
 |	Only mess with this after everything else works and making a backup		|
 |	copy of your project.  OpenGL & glut are tricky and it's really easy	|
 |	to break everything with a single line of code.							|
 |																			|
 |	Current keyboard controls:												|
 |																			|
 |		- 'ESC' --> exit the application									|
 |		- space bar --> resets the grid										|
 |																			|
 |		- 'c' --> toggle color mode on/off									|
 |		- 'b' --> toggles color mode off/on									|
 |		- 'l' --> toggles on/off grid line rendering						|
 |																			|
 |		- '+' --> increase simulation speed									|
 |		- '-' --> reduce simulation speed									|
 |																			|
 |		- '1' --> apply Rule 1 (Conway's classical Game of Life: B3/S23)	|
 |		- '2' --> apply Rule 2 (Coral: B3/S45678)							|
 |		- '3' --> apply Rule 3 (Amoeba: B357/S1358)							|
 |		- '4' --> apply Rule 4 (Maze: B3/S12345)							|
 |																			|
 +-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
//
#include "gl_frontEnd.h"

//==================================================================================
//	Custom data types
//==================================================================================
typedef struct ThreadInfo
{
	int index;
	//
	//	whatever other input or output data may be needed
	unsigned int startRow, endRow;
	
} ThreadInfo;


//==================================================================================
//	Function prototypes
//==================================================================================
void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);
void* threadFunc(void*);
void swapGrids(void);
unsigned int cellNewState(unsigned int i, unsigned int j);


//==================================================================================
//	Precompiler #define to let us specify how things should be handled at the
//	border of the frame
//==================================================================================

#define FRAME_FIXED		-1	//	the one I demo-ed in class, now disabled (values at border are kept fixed)
#define FRAME_DEAD		0	//	cell borders are kept dead
#define FRAME_RANDOM	1	//	new random values are generated at each generation
#define FRAME_CLIPPED	2	//	same rule as elsewhere, with clipping to stay within bounds
#define FRAME_WRAP		3	//	same rule as elsewhere, with wrapping around at edges

//	Pick one value for FRAME_BEHAVIOR
#define FRAME_BEHAVIOR	FRAME_DEAD

//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch
extern const int GRID_PANE, STATE_PANE;
extern int gMainWindow, gSubwindow[2];

//	The state grid and its dimensions.  We now have two copies of the grid:
//		- currentGrid is the one displayed in the graphic front end
//		- nextGrid is the grid that stores the next generation of cell
//			states, as computed by our threads.
int* currentGrid;
int* nextGrid;
int** currentGrid2D;
int** nextGrid2D;

//	Piece of advice, whenever you do a grid-based (e.g. image processing),
//	you should always try to run your code with a non-square grid to
//	spot accidental row-col inversion bugs.
const unsigned int NUM_ROWS = 400, NUM_COLS = 420;

//	the number of live threads (that haven't terminated yet)
const unsigned int MAX_NUM_THREADS = 100;
unsigned int numLiveThreads = 0;
unsigned int finishedThreadCount = 0;
unsigned int generation = 0;
pthread_mutex_t gridLock;
pthread_mutex_t* threadLock;
	

unsigned int rule = GAME_OF_LIFE_RULE;

unsigned int colorMode = 0;

//------------------------------
//	Threads and synchronization
//	Reminder of all declarations and function calls
//------------------------------
//pthread_mutex_t myLock;
//pthread_mutex_init(&myLock, NULL);
//int err = pthread_create(pthread_t*, NULL, threadFunc, ThreadInfo*);
//int pthread_join(pthread_t , void**);
//pthread_mutex_lock(&myLock);
//pthread_mutex_unlock(&myLock);



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

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render the grid.
	//
	//---------------------------------------------------------
	drawGrid(currentGrid2D, NUM_ROWS, NUM_COLS);
	
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

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//---------------------------------------------------------
	drawState(numLiveThreads);
	
	
	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	
	glutSetWindow(gMainWindow);
}

//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//	This takes care of initializing glut and the GUI.
	//	You shouldn’t have to touch this
	initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);
	
	//	Now we can do application-level initialization
	initializeApplication();

	
//int err = pthread_create(pthread_t*, NULL, threadFunc, ThreadInfo*);
//int pthread_join(pthread_t , void**);
//pthread_mutex_lock(&myLock);
//pthread_mutex_unlock(&myLock);
	//	Now would be the place & time to initialize mutex locks and threads


	pthread_t* thread = (pthread_t*) malloc(MAX_NUM_THREADS*sizeof(pthread_t));
	ThreadInfo* threadInfo = (ThreadInfo*) malloc(MAX_NUM_THREADS*sizeof(ThreadInfo));

	threadLock = (pthread_mutex_t*) malloc(MAX_NUM_THREADS*sizeof(pthread_mutex_t));

	pthread_mutex_init(&gridLock, NULL);
	for (int k=0; k<MAX_NUM_THREADS; k++)
	{
		pthread_mutex_init(threadLock+k, NULL);
		pthread_mutex_lock(threadLock+k);
	}

	const unsigned int bandHeight = (unsigned int) ceil((1.0*NUM_ROWS)/MAX_NUM_THREADS);
	//	create all the threads
	for (int k=0; k<MAX_NUM_THREADS; k++)
	{
		threadInfo[k].index = k;
		threadInfo[k].startRow = k*bandHeight;
		threadInfo[k].endRow = threadInfo[k].startRow + bandHeight - 1;
		
		int err = pthread_create(thread+k, NULL, threadFunc, threadInfo+k);
		//	test err
		{
		}
	}
	
//	printf("Main thread created all processors\n");
	
	//	start the threads
	for (int k=0; k<MAX_NUM_THREADS; k++)
	{
		pthread_mutex_unlock(threadLock+k);
	}	
	
	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	In fact this code is never reached because we only leave the glut main
	//	loop through an exit call.
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	free(currentGrid2D);
	free(currentGrid);
	
	
	//	This will never be executed (the exit point will be in one of the
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
    //  Allocate 1D grids
    //--------------------
    currentGrid = (int*) malloc(NUM_ROWS*NUM_COLS*sizeof(int));
    nextGrid = (int*) malloc(NUM_ROWS*NUM_COLS*sizeof(int));

    //  Scaffold 2D arrays on top of the 1D arrays
    //---------------------------------------------
    currentGrid2D = (int**) malloc(NUM_ROWS*sizeof(int*));
    nextGrid2D = (int**) malloc(NUM_ROWS*sizeof(int*));
    currentGrid2D[0] = currentGrid;
    nextGrid2D[0] = nextGrid;
    for (int i=1; i<NUM_ROWS; i++)
    {
        currentGrid2D[i] = currentGrid2D[i-1] + NUM_COLS;
        nextGrid2D[i] = nextGrid2D[i-1] + NUM_COLS;
    }

	
	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	simulation), only some color, in meant-to-be-thrown-away code
	
	//	seed the pseudo-random generator
	srand((unsigned int) time(NULL));
	
	resetGrid();
}

//---------------------------------------------------------------------
//	Implement this function
//---------------------------------------------------------------------
void* threadFunc(void* arg)
{
	ThreadInfo info = *((ThreadInfo*)arg);
//	printf("Thread %d created\n", info.index);

	pthread_mutex_lock(threadLock+info.index);
	
//	printf("Thread %d started\n", info.index);

	while (1) 
	{
		oneGeneration(info.startRow, info.endRow);

	
//printf("\tThread %d finisshed calculations\n", info.index);

		pthread_mutex_lock(&gridLock);
		finishedThreadCount++;
		if (finishedThreadCount == MAX_NUM_THREADS) 
		{
//printf("\tThread %d is last\n", info.index);

			finishedThreadCount = 0;
			swapGrids();
			pthread_mutex_unlock(&gridLock);

			generation++;	
			
			// wake up all the other threads
			for (int k=0; k<MAX_NUM_THREADS; k++)
			{
				if (k != info.index)
					pthread_mutex_unlock(threadLock+k);
			}
		}
		else 
		{
			pthread_mutex_unlock(&gridLock);
			//	threads should block here
			pthread_mutex_lock(threadLock+info.index);
		}
		
		usleep(1000);
	}
	
	return NULL;
}

void resetGrid(void)
{
	for (int i=0; i<NUM_ROWS; i++)
	{
		for (int j=0; j<NUM_COLS; j++)
		{
			nextGrid2D[i][j] = rand() % 2;
		}
	}
	swapGrids();
}

//	This function swaps the current and next grids, as well as their
//	companion 2D grid.  Note that we only swap the "top" layer of
//	the 2D grids.
void swapGrids(void)
{
	//	swap grids
	int* tempGrid;
	int** tempGrid2D;
	
	tempGrid = currentGrid;
	currentGrid = nextGrid;
	nextGrid = tempGrid;
	//
	tempGrid2D = currentGrid2D;
	currentGrid2D = nextGrid2D;
	nextGrid2D = tempGrid2D;
}

//	I have decided to go for maximum modularity and readability, at the
//	cost of some performance.  This may seem contradictory with the
//	very purpose of multi-threading our application.  I won't deny it.
//	My justification here is that this is very much an educational exercise,
//	my pbjective being for you to understand and master the mechanisms of
//	multithreading and synchronization with mutex.  After you get there,
//	you can micro-optimi1ze your synchronized multithreaded apps to your
//	heart's content.
void oneGeneration(unsigned int startIndex, unsigned int endIndex)
{
	for (unsigned int i=startIndex; i<=endIndex; i++)
	{
		for (unsigned int j=0; j<NUM_COLS; j++)
		{
			unsigned int newState = cellNewState(i, j);

			//	In black and white mode, only alive/dead matters
			//	Dead is dead in any mode
			if (colorMode == 0 || newState == 0)
			{
				nextGrid2D[i][j] = newState;
			}
			//	in color mode, color reflext the "age" of a live cell
			else
			{
				//	Any cell that has not yet reached the "very old cell"
				//	stage simply got one generation older
				if (currentGrid2D[i][j] < NB_COLORS-1)
					nextGrid2D[i][j] = currentGrid2D[i][j] + 1;
				//	An old cell remains old until it dies
				else
					nextGrid2D[i][j] = currentGrid2D[i][j];

			}
		}
	}
}

//	The quick implementation I did in class is not good because it keeps
//	the border unchanged.  Here I give three different implementations
//	of a slightly different algorithm, allowing for changes at the border
//	All three variants are used for simulations in research applications.
//	I also refer explicitly to the S/B elements of the "rule" in place.
unsigned int cellNewState(unsigned int i, unsigned int j)
{
	//	First count the number of neighbors that are alive
	//----------------------------------------------------
	//	Again, this implementation makes no pretense at being the most efficient.
	//	I am just trying to keep things modular and somewhat readable
	int count = 0;

	//	Away from the border, we simply count how many among the cell's
	//	eight neighbors are alive (cell state > 0)
	if (i>0 && i<NUM_ROWS-1 && j>0 && j<NUM_COLS-1)
	{
		//	remember that in C, (x == val) is either 1 or 0
		count = (currentGrid2D[i-1][j-1] != 0) +
				(currentGrid2D[i-1][j] != 0) +
				(currentGrid2D[i-1][j+1] != 0)  +
				(currentGrid2D[i][j-1] != 0)  +
				(currentGrid2D[i][j+1] != 0)  +
				(currentGrid2D[i+1][j-1] != 0)  +
				(currentGrid2D[i+1][j] != 0)  +
				(currentGrid2D[i+1][j+1] != 0);
	}
	//	on the border of the frame...
	else
	{
		#if FRAME_BEHAVIOR == FRAME_DEAD
		
			//	Hack to force death of a cell
			count = -1;
		
		#elif FRAME_BEHAVIOR == FRAME_RANDOM
		
			count = rand() % 9;
		
		#elif FRAME_BEHAVIOR == FRAME_CLIPPED
	
			if (i>0)
			{
				if (j>0 && currentGrid2D[i-1][j-1] != 0)
					count++;
				if (currentGrid2D[i-1][j] != 0)
					count++;
				if (j<NUM_COLS-1 && currentGrid2D[i-1][j+1] != 0)
					count++;
			}

			if (j>0 && currentGrid2D[i][j-1] != 0)
				count++;
			if (j<NUM_COLS-1 && currentGrid2D[i][j+1] != 0)
				count++;

			if (i<NUM_ROWS-1)
			{
				if (j>0 && currentGrid2D[i+1][j-1] != 0)
					count++;
				if (currentGrid2D[i+1][j] != 0)
					count++;
				if (j<NUM_COLS-1 && currentGrid2D[i+1][j+1] != 0)
					count++;
			}
			
	
		#elif FRAME_BEHAVIOR == FRAME_WRAPPED
	
			unsigned int 	iM1 = (i+NUM_ROWS-1)%NUM_ROWS,
							iP1 = (i+1)%NUM_ROWS,
							jM1 = (j+NUM_COLS-1)%NUM_COLS,
							jP1 = (j+1)%NUM_COLS;
			count = currentGrid2D[iM1][jM1] != 0 +
					currentGrid2D[iM1][j] != 0 +
					currentGrid2D[iM1][jP1] != 0  +
					currentGrid2D[i][jM1] != 0  +
					currentGrid2D[i][jP1] != 0  +
					currentGrid2D[iP1][jM1] != 0  +
					currentGrid2D[iP1][j] != 0  +
					currentGrid2D[iP1][jP1] != 0 ;

		#else
			#error undefined frame behavior
		#endif
		
	}	//	end of else case (on border)
	
	//	Next apply the cellular automaton rule
	//----------------------------------------------------
	//	by default, the grid square is going to be empty/dead
	unsigned int newState = 0;
	
	//	unless....
	
	switch (rule)
	{
		//	Rule 1 (Conway's classical Game of Life: B3/S23)
		case GAME_OF_LIFE_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (currentGrid2D[i][j] != 0)
			{
				if (count == 3 || count == 2)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;
	
		//	Rule 2 (Coral Growth: B3/S45678)
		case CORAL_GROWTH_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (currentGrid2D[i][j] != 0)
			{
				if (count > 3)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;
			
		//	Rule 3 (Amoeba: B357/S1358)
		case AMOEBA_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (currentGrid2D[i][j] != 0)
			{
				if (count == 1 || count == 3 || count == 5 || count == 8)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 1 || count == 3 || count == 5 || count == 8)
					newState = 1;
			}
			break;
		
		//	Rule 4 (Maze: B3/S12345)							|
		case MAZE_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (currentGrid2D[i][j] != 0)
			{
				if (count >= 1 && count <= 5)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;

			break;
		
		default:
			printf("Invalid rule number\n");
			exit(5);
	}

	return newState;
}
