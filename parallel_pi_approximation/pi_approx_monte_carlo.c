#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#define RQST 1
#define REPL 2

/* number of random numbers */
#define RAND_ARR_SIZE 1000

/* maximum number of random points */
#define MAX_POINTS 1000000000

int main(int argc, char* argv[]) {
	/*using command line arguments*/
	
	/*MPI variables*/
	MPI_Comm wrld; /*communicator for the world*/
	MPI_Comm wrkr; /*communicator for the workers*/
	MPI_Group wrld_group; /*group of the World communicator*/
	MPI_Group wrkr_group; /*group of the workers communicator*/
	MPI_Status status; /*status structure */
	
	int num_prcs; /*num of processes*/
	int curr_rank; /*current rank*/
	int srvr_rank; /*rank of the server process*/
	int wrkr_rank; /*rank of thr worker*/
	int arr_ranks[1]; /*num of ranks to be excluded*/
	
	/*alg variables*/
	double eps; /*accuracy*/
	int request; /*more requests for random number*/
	int arr_rand[RAND_ARR_SIZE]; /*array random numbers*/
	int i;
	int iter;
	int done;
	double x;               /* random x-coordinate */
    double y;               /* random y-coordinate */
	int in; /*nums inside a circle*/
	int total_in;
	int out; /*nums in square*/
	int total_out;
	double pi;
	double err; /*error in the estimation %f*/

	/*init*/
	num_prcs = 0;
	curr_rank = 0;
	srvr_rank = 0;
	arr_ranks[0] = 0;
	request = 0;
	wrkr = 0;
	i = 0;
	iter = 0;
	done = 0;
	pi = 0.0;
	err = 0.0;
	
	/*init MPI*/
	MPI_Init(&argc, &argv);
	
	/*set the world communicator*/
	wrld = MPI_COMM_WORLD;
	
	/*get the number of processes*/
	MPI_Comm_size(wrld, &num_prcs);
	
	/*get the rank of the current process*/
	MPI_Comm_rank(wrld, &curr_rank);
	
	/*set server rank - highest rank*/
	srvr_rank = num_prcs - 1;
	
	/*process with rank 0 reads accuracy */
	if(curr_rank == 0) {
		/*0 - executable, 1 - accuracy*/
		if(argc < 2) {
			fprintf(stderr, "Usage: %s accuracy\n", argv[0] );
			MPI_Abort(wrld, 1);
		}
		
		/*read formatted data from string*/
		sscanf(argv[1], "%lf", &eps);
	}
	
	MPI_Bcast(
	&eps,
	1, 
	MPI_DOUBLE,
	0,
	wrld
	);
	
	/*extract the group from the World communicator*/
	MPI_Comm_group(
	wrld,
	&wrld_group
	);
	
	/*exclude the random number server process from the group*/
	arr_ranks[0] = srvr_rank;
	MPI_Group_excl(
	wrld_group,
	1,
	arr_ranks,
	&wrkr_group
	);
	
	/*create the new communicator*/
	MPI_Comm_create(
	wrld,
	wrkr_group,
	&wrkr
	);
	
	/*free the reference to the workers group*/
	MPI_Group_free(&wrkr_group);
	
	/*server/worker structure of the program*/
	if(curr_rank == srvr_rank) {
		/*random numbers server*/
		srandom(time(0));
		
		do{
			MPI_Recv(
			&request,  /*address of the receive buffer*/
			1,
			MPI_INT,
			MPI_ANY_SOURCE,
			RQST,
			wrld,
			&status
			);
			
			if(request) {
				/*generate an array of random integers*/
				while(i < RAND_ARR_SIZE) {
					arr_rand[i] = random();
					
					if(arr_rand[i] <= INT_MAX) {
						i++;
					}
				}
	
				/*send the array to the worker*/
				MPI_Send(arr_rand, RAND_ARR_SIZE, MPI_INT, status.MPI_SOURCE, REPL, wrld);
			}
		} while(request > 0);
		
	} else {
		/*workers*/
		request = 1;
		
		MPI_Send(&request, 1, MPI_INT, srvr_rank, RQST, wrld);
		
		MPI_Comm_rank(wrkr, &wrkr_rank);
		
		iter = 0;
		while(!done) {
			iter++;
			
			MPI_Recv(&arr_rand, RAND_ARR_SIZE, MPI_INT, srvr_rank, REPL, wrld, MPI_STATUS_IGNORE);
			
			/*count points inside and outside of the circle*/
			i = 0;
            while (i < RAND_ARR_SIZE)
            {
                x = (((double) arr_rand[i++]) / INT_MAX) * 2 - 1;
                y = (((double) arr_rand[i++]) / INT_MAX) * 2 - 1;
                (x * x + y * y < 1.0) ? in++ : out++;
            }
			
			/*SUM from all workers*/
			MPI_Allreduce(&in, &total_in, 1, MPI_INT, MPI_SUM, wrkr); 
			MPI_Allreduce(&out, &total_out, 1, MPI_INT, MPI_SUM, wrkr);
			
			/* calculate pi approximation */
			pi = (4.0 * total_in) / (total_in + total_out);
            err = fabs(pi - M_PI);
            done = (err < eps) || (total_in + total_out > MAX_POINTS);
            request = (done) ? 0 : 1;
			
			if (curr_rank == 0)
            {
                /* printf("Approximated: %.20f Error: %.20f\n", pi, err); */

                MPI_Send(
                    &request,           /* address of send buffer */
                    1,                  /* number of elements */
                    MPI_INT,            /* data type of communicated data */
                    srvr_rank,          /* rank of the destination process */
                    RQST,              /* message tag is request */
                    wrld                /* communicator */
                    );
            }
            else
            {
                if (request)
                {
                    MPI_Send(
                        &request,           /* address of send buffer */
                        1,                  /* number of elements */
                        MPI_INT,            /* data type of communicated data */
                        srvr_rank,          /* rank of the destination process */
                        RQST,              /* message tag is request */
                        wrld                /* communicator */
                        );
                }
            }
		}
		
		MPI_Comm_free(&wrkr);
	}
	
	if (curr_rank == 0)
    {
        printf("Approximated: %.20f Error: %.20f\n", pi, err);
    }
	
	/*clear resources*/
	MPI_Finalize();
	
	return 0;
}