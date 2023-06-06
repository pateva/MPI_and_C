#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

#define NUMB_BLCK 2

/*Implement a parallel program that creates a structure Particle, composed by the coordinates in a plane in floating-point numbers, and an integer label. The program executes two roles:
sender and receiver. The sender sends the structure to the receiver. In order to communicate the
structure, create a MPI derived data type structure*/

typedef struct {
double xcoord;
double ycoord;
int label;
} Part;

int main(int argc, char* argv[]) {
/*MPI variables*/
	int num_prc;
	int curr_rank;
	MPI_Datatype dt_part; 
	MPI_Datatype types[NUMB_BLCK];
	MPI_Aint dsplc[NUMB_BLCK];
	int block_count[NUMB_BLCK];
	int i;
	Part send;
	Part rcv;

	num_prc = 0;
	curr_rank = 0;
	i = 0;
	send.xcoord = 0.1;
	send.ycoord = 0.2;
	send.label = 1;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &num_prc);
	MPI_Comm_rank(MPI_COMM_WORLD, &curr_rank);

	types[0] = MPI_DOUBLE;
	types[1] = MPI_INT;

	block_count[0] = 2;
	block_count[1] = 1;

	dsplc[0] = offsetof(Part, xcoord);
    dsplc[1] = offsetof(Part, label);
	
	MPI_Type_create_struct(NUMB_BLCK, block_count, dsplc, types, &dt_part);
	MPI_Type_commit(&dt_part);
	
	switch(curr_rank) {
		case 0:
		MPI_Send(&send, 1, dt_part, 1, 0, MPI_COMM_WORLD);
		break;
		case 1:
		MPI_Recv(&rcv, 1, dt_part, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Received particle: (%f, %f), %d\n", rcv.xcoord, rcv.ycoord, rcv.label);
		break;
		default:
		fprintf(stderr, "Wrong role code in exchange()\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

return 0;
}