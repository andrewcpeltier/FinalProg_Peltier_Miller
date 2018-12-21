#!/bin/sh

# This script takes as parameters:
# 
#   -Width and height N of the grid to be displayed in the window
#   -Number of boxes for the simulation to work with
#   -Number of doors where 1 <= numDoors <= 3
#   -The number of times the simulation is to be run
# 
# Using these parameters, the script compiles the executable for robotsV4.c, then runs
# the program, producing an output text file titled "robotSimulOut Num.txt" where num
# correlates to the current simulation number.
#
#   Authors: Luke Miller and Andrew Peltier

# User specified input params
GRID_WIDTH=$1
GRID_HEIGHT=$2
NUM_BOXES=$3
NUM_DOORS=$4
NUM_SIMULATIONS=$5

# Initial Simulations executed to 1
SIMULATIONS_EXECUTED=1

# Bounds for the number of doors specified by the user
MIN_DOORS=1
MAX_DOORS=3

# Compile the robotsV4 executable
gcc -pthread -Wall "robotsV4.c" "gl_frontEnd.c" -lm -lGL -lglut  -o robots

# Make sure the user has specified a number of doors 1 <= NUM_DOORS <= 3
if  [ "$NUM_DOORS" -lt "$MIN_DOORS" ] || [ "$NUM_DOORS" -gt "$MAX_DOORS" ]; then
    echo "Please specify a number of doors between the bounds of 1 and 3, inclusive."

# If the user has specified the correct number of doors, run the simulation NUM_SIMULATIONS times
else
    # While there are still simulations to be run
    while [ "$SIMULATIONS_EXECUTED" -le "$NUM_SIMULATIONS" ]; do

        # update string containing name of the outputFile
        OUTPUT_FILE_NAME="robotSimulOut_$SIMULATIONS_EXECUTED.txt"

        # Call the robots executable with the specified input params
        ./robots $GRID_HEIGHT $GRID_WIDTH $NUM_DOORS $NUM_BOXES $OUTPUT_FILE_NAME

        # Decrement simulation count
        SIMULATIONS_EXECUTED=$(($SIMULATIONS_EXECUTED + 1))
        
    done
fi
