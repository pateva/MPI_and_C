#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define ROOT 0              /* rank of the root process */
#define RQUST_SIZE 2        /* size of the array of communication handles (send and receive) */
#define MAX_PARTICLES 4000  /* total maximum number of particles */
#define MAX_PART_PRCS 128   /* maximum number of particles per process */
#define REPLIC 4            /* replication for contiguous data type */
#define MAX_ITER 10         /* number of iterations of the simulation */

typedef struct
{
	/*coordinates in a plane*/
	double x;
	double y;
} Coord;

int readNumPart(int argc, char* argv[], int numb_prcs)
{
    int numb_part;
    numb_part = 0;

    /* handle command line arguments */
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <number of particles>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* number of particles per process */
    numb_part = atoi(argv[1]) / numb_prcs;

    /* number of particles per process must not be too big */
    if (numb_part * numb_prcs > MAX_PARTICLES)
    {
        fprintf(
            stderr, 
            "%d number of particles is more than the max %d\n", 
            numb_part * numb_prcs, 
            MAX_PARTICLES
            );
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    return numb_part;
}

void createRing(MPI_Comm* comm_ring, int* rank_left, int* rank_right, int* num_prc) {
	int periodic; /*0 - not periodinc, 1 - periodic*/
	periodic = 1;
	MPI_Cart_create(MPI_COMM_WORLD, 1, num_prc, &periodic, 1, comm_ring);
	MPI_Cart_shift(*comm_ring, 0, 1, rank_left, rank_right);
}

void printArr(int arr[], int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        printf("[%d]: %d\n", i, arr[i]);
    }
}

void printPart(Coord part[], int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        printf("%d: (%f, %f)\n", i, part[i].x, part[i].y);
    }
}

void randPart(Coord part[], int numb_part)
{
    int i;

    for (i = 0; i < numb_part; i++)
    {
        part[i].x = drand48();
        part[i].y = drand48();
    }
}

double calcCurrDist(Coord curr[], Coord rest[],int num_part)
{
    int i, j;
    double max_dist;
    double loc_dist;

    i = 0;
    j = 0;
    max_dist = 0.0;
    loc_dist = 0.0;

    for (i = 0; i < num_part; i++)
    {
        for (j = 0; j < num_part; j++)
        {
            loc_dist = 
                sqrt( 
                    (curr[i].x - rest[j].x) * (curr[i].x - rest[j].x)
                    + (curr[i].y - rest[j].y) * (curr[i].y - rest[j].y)
                );
            max_dist = loc_dist > max_dist ? loc_dist : max_dist;
        }
    }

    return max_dist;
}

int main(int argc, char* argv[]) {
	/*MPI variables*/
	MPI_Comm comm_ring;  				
	MPI_Datatype part_type;    
    MPI_Request rqst[RQUST_SIZE];
	MPI_Status stats[RQUST_SIZE];
	int num_prc;
	int curr_rank;
	int periodic;
	int rank_left;
	int rank_right;
	double exec_time;
	
	/*alg variables*/
	int index_pipe;
	int numb_part;
	double max_dist;
	double max_dist_curr;
	int part_per_prc[MAX_PART_PRCS];
	/*particles for all nodes*/
	Coord part[MAX_PARTICLES];
	/*Send buffer*/
	Coord sbuf[MAX_PARTICLES];
	/*Receive buffer*/
	Coord rbuf[MAX_PARTICLES];
	
	/* init */
    comm_ring = 0;
    part_type = 0;
    num_prc = 0;
    curr_rank = 0;
    periodic = 0;
    rank_left = 0;
    rank_right = 0;
    exec_time = 0.0;
    index_pipe = 0;
    numb_part = 0;
    max_dist = 0.0;
    max_dist_curr = 0.0;

    /* init MPI */
    MPI_Init(&argc, &argv);

    /* get number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &num_prc);

    /* get the rank of the current process */
    MPI_Comm_rank(MPI_COMM_WORLD, &curr_rank);
	
	if(curr_rank == ROOT) {
	numb_part = readNumPart(argc, argv, num_prc);
	}
	
	MPI_Bcast(&numb_part, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
	
	/* 1D periodic Cartesian communicator: ring pipeline */
    createRing(&comm_ring, &rank_left, &rank_right, &num_prc);
	
	/* each process gets the number of particles in each process */
    /* it is possible to rework that number of particles are not equally distributed among processes */
	MPI_Allgather(&numb_part, 1, MPI_INT, part_per_prc, 1, MPI_INT, comm_ring);
	
	/* root process reads number of particles and broadcasts it */
    if (curr_rank == ROOT)
    {
        /* print the number of particles per process */
        printf("Number of particles per process:\n");
        printArr(part_per_prc, num_prc);
    }

    /* create contiguous datatype for particles */
    MPI_Type_contiguous(
        REPLIC,            
        MPI_DOUBLE,         
        &part_type          
    );
	
    MPI_Type_commit(&part_type);
	
	/* initialize particles with random coordinates */
    srand48(time(0) * (curr_rank + 1));
    randPart(part, numb_part);

    printf("Particles in %d\n", curr_rank);
    printPart(part, numb_part);

    exec_time = MPI_Wtime();    /* starting time */
	
	/* implement the pipeline */
    /* copy particles into send buffer */
    memcpy(sbuf, part, numb_part * sizeof(Coord));
    max_dist = 0.0;
    for (index_pipe = 0; index_pipe <  num_prc; index_pipe++)
    {
        if (index_pipe != num_prc - 1)
        {
            /* non-blocking send */
            MPI_Isend(sbuf, numb_part, part_type, rank_right, index_pipe, comm_ring, &rqst[0]);

            /* non-blocking receive */
            MPI_Irecv(
                rbuf, numb_part, part_type, rank_left, index_pipe, comm_ring, &rqst[1]);
        }

        /* calculate distances with the known points */
        max_dist_curr = calcCurrDist(part, sbuf, numb_part);
        
        /* current maximum in the pipe */
        max_dist = max_dist_curr > max_dist ? max_dist_curr : max_dist;

        /* push the pipe */
        if (index_pipe != num_prc - 1)
        {
            /* block until send and receive communications to complete */
            MPI_Waitall( RQUST_SIZE, rqst, stats);
        }

        /* copy the received into the send buffers */
        memcpy(sbuf, rbuf, part_per_prc[index_pipe] * sizeof(Coord));
    }

    exec_time = MPI_Wtime() - exec_time;    /* ending time */

    /* output execution */
    if (curr_rank == ROOT)
    {
        printf("Max distance: %f. Execution time: %f sec.\n", max_dist, exec_time);
    }

    /* free contiguous data type */
    MPI_Type_free(&part_type);    

    /* clear MPI resources */
    MPI_Finalize();

    return 0;
	
	
}