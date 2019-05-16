//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2018-12-05
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.
#include <iostream>
#include <iomanip>
#include <string>
//
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits.h> //Used to get INT_MAX

//ADDED: using for sleep function
#include <unistd.h>
//
#include "gl_frontEnd.h"

using namespace std;

//==================================================================================
//	Function prototypes
//==================================================================================
void initializeApplication(void);
GridPosition getNewFreePosition(void);
Direction newDirection(Direction forbiddenDir = NUM_DIRECTIONS);
TravelerSegment newTravelerSeg(const TravelerSegment& currentSeg, bool& canAdd);
void generateWalls(void);
void generatePartitions(void);
void findExit(unsigned int);
bool exitFound(Traveler);
void findNextStep(unsigned int);
//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
SquareType** grid;
std::vector<std::vector<int>> weightedGrid;	//ADDED our weighted grid
unsigned int numRows = 0;			//	height of the grid
unsigned int numCols = 0;			//	width
unsigned int numTravelers = 0;		//	initial number
unsigned int numTravelersDone = 0;
unsigned int numLiveThreads = 0;	//	the number of live traveler threads
vector<Traveler> travelerList;
vector<SlidingPartition> partitionList;
GridPosition	exitPos;			//	location of the exit
unsigned int movesDone = 0;
unsigned int numMovesBeforeNewSegment = 0;
bool programRunning = true;			//	Global called when exit command is issued

//	robot sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int travelerSleepTime = 100000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
time_t launchTime;

void findNextStep(unsigned int travelerIndex){
	//get traveler and traveler head
	Traveler& traveler = travelerList[travelerIndex];
	TravelerSegment head = traveler.segmentList[0];

	//find starting coords
	unsigned int	lowestRow = head.row;
	unsigned int	lowestCol = head.col;
	Direction		lowestDir = head.dir;
	std::cout << "	Starting [Row, Col]: [" << lowestRow << ", "<< lowestCol << "] "<<std::endl;

	//find lowest neighbor

	//NORTH
	if(head.row < numRows - 1){
		if(weightedGrid[lowestRow][lowestCol] > weightedGrid[head.row + 1][head.col]){
			lowestRow = head.row + 1;
			lowestCol = head.col;
			//direction is north but this works
			lowestDir = SOUTH;
		}
	}

	//SOUTH
	if(head.row > 0){
		if(weightedGrid[lowestRow][lowestCol] > weightedGrid[head.row - 1][head.col]){
			lowestRow = head.row - 1;
			lowestCol = head.col;
			//direction is south but this works
			lowestDir = NORTH;
		}
	}

	//EAST
	if(head.col < numCols - 1){lowestCol = head.col;
		if(weightedGrid[lowestRow][lowestCol] > weightedGrid[head.row][head.col + 1]){
			lowestCol = head.col + 1;
			lowestRow = head.row;
			lowestDir = EAST;
		}
	}

	//WEST
	if(head.col > 0){
		if(weightedGrid[lowestRow][lowestCol] > weightedGrid[head.row][head.col - 1]){
			lowestCol = head.col - 1;
			lowestRow = head.row;
			lowestDir = WEST;
		}
	}

	//set traveler coordinates to lowest neighbor coordinates
	head.row = lowestRow;
	head.col = lowestCol;
	head.dir = lowestDir;

	//Create placeholder tail segment for iterating and updating tail
	TravelerSegment tail;
	unsigned int length = traveler.segmentList.size() - 1;
	tail.row = traveler.segmentList[length].row;
	tail.col = traveler.segmentList[length].col;
	tail.dir = traveler.segmentList[length].dir;

	//Update tail positions
	for(size_t i = traveler.segmentList.size() - 1; i > 0;i--){
		traveler.segmentList[i].row = traveler.segmentList[i - 1].row;
		traveler.segmentList[i].col = traveler.segmentList[i - 1].col;
		traveler.segmentList[i].dir = traveler.segmentList[i - 1].dir;
	}

	//do stuff with the addition of tail/tail
	traveler.segmentList[0] = head;
	if (grid[head.row][head.col] != EXIT)
		grid[head.row][head.col] = TRAVELER;

	std::cout << "	Ending [Row, Col]:   [" << lowestRow << ", "<< lowestCol << "] "<<std::endl;
	traveler.movesDone++;
	if(traveler.movesDone % numMovesBeforeNewSegment == 0){
		std::cout << "Moves Done: " << traveler.movesDone << std::endl;
		traveler.segmentList.push_back(tail);
		std::cout << "SIZE OF LIST: " << traveler.segmentList.size() << std::endl;
	 }
	 else {
		 grid[tail.row][tail.col] = FREE_SQUARE;
	 }

}

void* threadFunction(void* voidIndex){
	int* index = (int*) voidIndex;
	int ind = *index;
	//Traveler& tourist = travelerList[ind];
	while (true) {
		findNextStep(ind);
		usleep(200000);
		if(exitFound(travelerList[ind])){
			travelerList[ind].status = TERMINATED;
			numTravelersDone++;
			break;
		}
	}
	return index;
}

//==================================================================================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================

void threadCreate(void){
	// Create a thread for each entry in numTravelers
	for(unsigned int i = 0; i < numTravelers; i++){
		std::cout << "reached here: "  << i << '\n';
		numLiveThreads++;
		if(pthread_create(&(travelerList[i].thread_id), NULL, threadFunction, &(travelerList[i].index))){
			perror("join error");
			exit(1);
		}
	}
}
void drawTravelers(void)
{
	//-----------------------------
	//	You may have to sychronize things here
	//-----------------------------

	for (unsigned int k=0; k < numTravelers; k++)
	{
		// Draw thread if still searching for exit
		if (travelerList[k].status == RUNNING) {
			drawTraveler(travelerList[k]);
		}

		// Join thread if thread has found exit
		else if (travelerList[k].status == TERMINATED || !programRunning) {
			if(pthread_join(travelerList[k].thread_id, NULL)){
				perror("join error");
			 	exit(1);
			 }
			 travelerList[k].status = JOINED;
			 numLiveThreads--;
		}
	}
}

void updateMessages(void)
{
	//	Here I hard-code a few messages that I want to see displayed
	//	in my state pane.  The number of live robot threads will
	//	always get displayed.  No need to pass a message about it.
	unsigned int numMessages = 4;
	sprintf(message[0], "We created %d travelers", numTravelers);
	sprintf(message[1], "%d travelers solved the maze", numTravelersDone);
	sprintf(message[2], "I like cheese");
	sprintf(message[3], "Simulation run time is %ld", time(NULL)-launchTime);

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//
	//---------------------------------------------------------
	drawMessages(numMessages, message);
}

void handleKeyboardEvent(unsigned char c, int x, int y)
{
	int ok = 0;

	switch (c)
	{
		//	'esc' to quit
		case 27:
			exit(0);
			break;

		//	slowdown
		case ',':
			slowdownTravelers();
			ok = 1;
			break;

		//	speedup
		case '.':
			speedupTravelers();
			ok = 1;
			break;

		default:
			ok = 1;
			break;
	}
	if (!ok)
	{
		//	do something?
	}
}


//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupTravelers(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * travelerSleepTime) / 10;

	if (newSleepTime > MIN_SLEEP_TIME)
	{
		travelerSleepTime = newSleepTime;
	}
}

void slowdownTravelers(void)
{
	//	increase sleep time by 20%
	travelerSleepTime = (12 * travelerSleepTime) / 10;
}

void recursivePathWeighting(unsigned int row, unsigned int col, int distance){
//	unsigned int x = row;
//	unsigned int y = col;
	if (grid[row][col] == FREE_SQUARE || grid[row][col] == EXIT) {
		if (weightedGrid[row][col] > distance ) {
			weightedGrid[row][col] = distance;

			//Recursive searching
			if (row < numRows - 1){//SOUTH
				if (grid[row+1][col] == FREE_SQUARE) {//Check for empty square
					recursivePathWeighting(row+1, col, distance + 1);
				}
			}
			if (row > 0) 			{//NORTH
				if (grid[row-1][col] == FREE_SQUARE) {//Check for empty square
					recursivePathWeighting(row-1,col, distance + 1);
				}
			}
			if (col < numCols - 1){//EAST
				if (grid[row][col+1] == FREE_SQUARE) {//Check for empty square
					recursivePathWeighting(row, col+1, distance + 1);
				}
			}
			if (col > 0) 			{//WEST
				if (grid[row][col-1] == FREE_SQUARE) {//Check for empty square
					recursivePathWeighting(row, col-1, distance + 1);
				}
			}
		}
		return;
	}
	return;
}

void generateWeightedGrid(){
	recursivePathWeighting(exitPos.row, exitPos.col, 0);
}

bool exitFound(Traveler traveler){
	//assuming that first segment in traveler is the head of the traveler
	TravelerSegment head = traveler.segmentList[0];
	return exitPos.row == head.row && exitPos.col == head.col;
}

//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	initialization of the various global variables and lists
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	//	We know that the arguments  of the program  are going
	//	to be the width (number of columns) and height (number of rows) of the
	//	grid, the number of travelers, etc.
	//	So far, I hard code-some values

	numRows = atoi(argv[1]);		//45
	numCols = atoi(argv[2]);		//40
	numTravelers = 1; 	// 1 ### 8
	numLiveThreads = 0;
	numTravelersDone = 0;
	if(argc < 5){
		numMovesBeforeNewSegment = INT_MAX;
	} else {
		numMovesBeforeNewSegment = atoi(argv[3]);
	}
	travelerList.reserve(numTravelers);

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv);

	//	Now we can do application-level initialization
	initializeApplication();

	launchTime = time(NULL);

	threadCreate();

	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();

	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (unsigned int i=0; i< numRows; i++)
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
//	This is a function that you have to edit and add to.
//
//==================================================================================


void initializeApplication(void)
{

	//	Allocate the grid
	grid = new SquareType*[numRows];
	for (unsigned int i=0; i<numRows; i++)
	{
		grid[i] = new SquareType[numCols];
		for (unsigned int j=0; j< numCols; j++)
			grid[i][j] = FREE_SQUARE;

	}

	//------------------------------------
	//-- Initialize empty weighted grid --
	//------------------------------------
	std::vector<int> rowToPushBackIntoGrid;
	for (size_t jCols = 0; jCols < numCols; jCols++) { // A Row
		rowToPushBackIntoGrid.push_back(INT_MAX);
	}

	for (size_t iRows = 0; iRows < numRows; iRows++) { //A Grid
		weightedGrid.push_back(rowToPushBackIntoGrid);
	}
	//------------------------------------



	message = (char**) malloc(MAX_NUM_MESSAGES*sizeof(char*));
	for (unsigned int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = (char*) malloc((MAX_LENGTH_MESSAGE+1)*sizeof(char));

	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I initialize the grid's pixels to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator after ranting in class that the C random
	//	generator was junk.  Here I am not using it to produce "serious" data (as in a
	//	real simulation), only wall/partition location and some color
	srand((unsigned int) time(NULL));

	//	generate a random exit
	exitPos = getNewFreePosition();
	grid[exitPos.row][exitPos.col] = EXIT;

	//	Generate walls and partitions
	generateWalls();
	generatePartitions();


	//------------------------------------
	//-------- Generate grid -------------
	//------------------------------------
	generateWeightedGrid();
	// Prints grid
	for (size_t i = 0; i < numRows; i++) {
		std::cout << std::fixed << std::left << std::setw(3)<< i << ":   ";
		for (size_t j = 0; j < numCols; j++) {
			if (weightedGrid[i][j] == INT_MAX) {
				std::cout<< std::fixed << std::left << std::setw(4) << "Wall" << "|";
			}
			else{
				std::cout<< " "<< std::fixed << std::left << std::setw(3) << weightedGrid[i][j] << "|";
			}
		}
		std::cout << std::endl;
	}
	//------------------------------------



	//	Initialize traveler info structs
	//	You will probably need to replace/complete this as you add thread-related data
	float** travelerColor = createTravelerColors(numTravelers);
	for (unsigned int k=0; k<numTravelers; k++) {
		GridPosition pos = getNewFreePosition();
		Direction dir = static_cast<Direction>(rand() % NUM_DIRECTIONS);
		TravelerSegment seg = {pos.row, pos.col, dir};
		Traveler traveler;
		traveler.index = k;
		traveler.status = RUNNING;
		traveler.segmentList.push_back(seg);
		grid[pos.row][pos.col] = TRAVELER;

		//	I add 0-6 segments to my travelers
		unsigned int numAddSegments = rand() % 7;
		TravelerSegment currSeg = traveler.segmentList[0];
		bool canAddSegment = true;
		for (unsigned int s=0; s<numAddSegments && canAddSegment; s++)
		{
			TravelerSegment newSeg = newTravelerSeg(currSeg, canAddSegment);
			if (canAddSegment)
			{
				traveler.segmentList.push_back(newSeg);
				currSeg = newSeg;
				grid[newSeg.row][newSeg.col] = TRAVELER;
			}
		}

		for (unsigned int c=0; c<4; c++)
			traveler.rgba[c] = travelerColor[k][c];

		travelerList.push_back(traveler);
	}

	//threadCreate();
	//	free array of colors
	for (unsigned int k=0; k<numTravelers; k++)
		delete []travelerColor[k];
	delete []travelerColor;
}


//------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Generation Helper Functions
#endif
//------------------------------------------------------

GridPosition getNewFreePosition(void)
{
	GridPosition pos;

	bool noGoodPos = true;
	while (noGoodPos)
	{
		unsigned int row = rand() % numRows;
		unsigned int col = rand() % numCols;
		if (grid[row][col] == FREE_SQUARE)
		{
			pos.row = row;
			pos.col = col;
			noGoodPos = false;
		}
	}
	return pos;
}

Direction newDirection(Direction forbiddenDir)
{
	bool noDir = true;

	Direction dir = NUM_DIRECTIONS;
	while (noDir)
	{
		dir = static_cast<Direction>(rand() % NUM_DIRECTIONS);
		noDir = (dir==forbiddenDir);
	}
	return dir;
}


TravelerSegment newTravelerSeg(const TravelerSegment& currentSeg, bool& canAdd)
{
	TravelerSegment newSeg;
	switch (currentSeg.dir)
	{
		case NORTH:
			if (	currentSeg.row < numRows-1 &&
					grid[currentSeg.row+1][currentSeg.col] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row+1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(SOUTH);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case SOUTH:
			if (	currentSeg.row > 0 &&
					grid[currentSeg.row-1][currentSeg.col] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row-1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(NORTH);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case WEST:
			if (	currentSeg.col < numCols-1 &&
					grid[currentSeg.row][currentSeg.col+1] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col+1;
				newSeg.dir = newDirection(EAST);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case EAST:
			if (	currentSeg.col > 0 &&
					grid[currentSeg.row][currentSeg.col-1] == FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col-1;
				newSeg.dir = newDirection(WEST);
				grid[newSeg.row][newSeg.col] = TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		default:
			canAdd = false;
	}

	return newSeg;
}

void generateWalls(void)
{
	const unsigned int NUM_WALLS = (numCols+numRows)/4;

	//	I decide that a wall length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its orientation
	const unsigned int MIN_WALL_LENGTH = 3;
	const unsigned int MAX_HORIZ_WALL_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_WALL_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodWall = true;

	//	Generate the vertical walls
	for (unsigned int w=0; w< NUM_WALLS; w++)
	{
		goodWall = false;

		//	Case of a vertical wall
		if (rand() %2)
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;

				//	select a column index
				unsigned int HSP = numCols/(NUM_WALLS/2+1);
				unsigned int col = (1+ rand()%(NUM_WALLS/2-1))*HSP;
				unsigned int length = MIN_WALL_LENGTH + rand()%(MAX_VERT_WALL_LENGTH-MIN_WALL_LENGTH+1);

				//	now a random start row
				unsigned int startRow = rand()%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodWall = false;
				}

				//	if the wall first, add it to the grid
				if (goodWall)
				{
					for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
					{
						grid[row][col] = WALL;
					}
				}
			}
		}
		// case of a horizontal wall
		else
		{
			goodWall = false;

			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;

				//	select a column index
				unsigned int VSP = numRows/(NUM_WALLS/2+1);
				unsigned int row = (1+ rand()%(NUM_WALLS/2-1))*VSP;
				unsigned int length = MIN_WALL_LENGTH + rand()%(MAX_HORIZ_WALL_LENGTH-MIN_WALL_LENGTH+1);

				//	now a random start row
				unsigned int startCol = rand()%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodWall = false;
				}

				//	if the wall first, add it to the grid
				if (goodWall)
				{
					for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
					{
						grid[row][col] = WALL;
					}
				}
			}
		}
	}
}


void generatePartitions(void)
{
	const unsigned int NUM_PARTS = (numCols+numRows)/4;

	//	I decide that a partition length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its orientation
	const unsigned int MIN_PART_LENGTH = 3;
	const unsigned int MAX_HORIZ_PART_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_PART_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodPart = true;

	for (unsigned int w=0; w< NUM_PARTS; w++)
	{
		goodPart = false;

		//	Case of a vertical partition
		if (rand() %2)
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;

				//	select a column index
				unsigned int HSP = numCols/(NUM_PARTS/2+1);
				unsigned int col = (1+ rand()%(NUM_PARTS/2-2))*HSP + HSP/2;
				unsigned int length = MIN_PART_LENGTH + rand()%(MAX_VERT_PART_LENGTH-MIN_PART_LENGTH+1);

				//	now a random start row
				unsigned int startRow = rand()%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodPart = false;
				}

				//	if the partition is possible,
				if (goodPart)
				{
					//	add it to the grid and to the partition list
					SlidingPartition part;
					part.isVertical = true;
					for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
					{
						grid[row][col] = VERTICAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
				}
			}
		}
		// case of a horizontal partition
		else
		{
			goodPart = false;

			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;

				//	select a column index
				unsigned int VSP = numRows/(NUM_PARTS/2+1);
				unsigned int row = (1+ rand()%(NUM_PARTS/2-2))*VSP + VSP/2;
				unsigned int length = MIN_PART_LENGTH + rand()%(MAX_HORIZ_PART_LENGTH-MIN_PART_LENGTH+1);

				//	now a random start row
				unsigned int startCol = rand()%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
				{
					if (grid[row][col] != FREE_SQUARE)
						goodPart = false;
				}

				//	if the wall first, add it to the grid and build SlidingPartition object
				if (goodPart)
				{
					SlidingPartition part;
					part.isVertical = false;
					for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
					{
						grid[row][col] = HORIZONTAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
				}
			}
		}
	}
}
