# IOS_2.project

## About <a name = "about"></a>

This software synchronizes processes to solve a problem based on "The barbershop problem",
which can be found in the [Little Book of Semaphores](https://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf) in English,
or in the file [IOS-project2.pdf](IOS-project2.pdf) in Czech.

## Usage <a name = "usage"></a>

To run the program:

1. Compile with make in the repository folder.

2. Set execution rights, then run the program as `./proj2 NZ NU TZ TU F`, where:
    + NZ - number of customers,
    + NU - number of workers,
    + TZ - waiting time (ms) for customers to enter (0 < TZ <= 10000),
    + TU - maximum break time (ms) for worker (0 < TU <= 100),
    + F - maximum time (ms) for the office to be open for (0 <= F <= 10000).

3. The output of the program will be in the file `proj2.out`.

## Rating <a name = "rating"></a>

The project was rated 18/15 points in the summer semester of 2023.
