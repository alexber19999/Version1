# FinalProject
#Alexander Ives and Alexander(Sasha) Berezovsky

##Version 1:
### How to compile: g++ -Wall main.cpp gl_frontEnd.cpp -lm -lGL -lglut -o main
### How to run: ./main widthHeightOfGrid numTravelers numMovesBeforeAddingSegment
### Major design decisions: We decided to use a recursive search which started at the exit
### and then expanded outwords recursively until the grid(except for walls) was fully explored. Afterwords, the traveler could repeatedly find the neighbor with the lowest weight
###(this corresponds to the neighbor that is closest to the exit) and moves towards that neighbor until it has reached the exit.
### What does not work: If the dimensions of the grid are too small, it will not be able to properly generate the walls and the partitions

##Version 2:

##Version 3:

##Version 4:

##Version 5:
 
