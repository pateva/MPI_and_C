#include <stdio.h>
#include <mpi.h>

/* capacity of arrays */
#define MAX_ROWS 1000
#define MAX_COLS 1000

/* number of elements used */
#define SIZE_ROWS 10
#define SIZE_COLS 10
#define MIN(x,y) (x < y ? x : y)


void printMatr(int matr[MAX_ROWS][MAX_COLS], int rows, int cols) {
	int i, j;
	
	for(i = 0; i < rows; i++) {
		for(j = 0; j < cols; j++) {
		printf("[%d]", matr[i][j]);
		}
		
		printf("\n");
	}
}

void initMatr(int matr[MAX_ROWS][MAX_COLS], int rows, int cols) {
	int i, j;
	
	for(i = 0; i < rows; i++) {
		
		for(j = 0; j < cols; j++) {
		matr[i][j] = j + 1;
		}
	}
	
}

int prod(int vectA[MAX_COLS], int vectB[MAX_COLS], int sz) {
	int res, i;
	res = 0;
	
	for(i = 0; i <sz; i++) {
		res+= vectA[i]*vectB[i];
	}
	
	return res;
	
}

int main(int argc, char* argv[]) {
	
	/*variables*/
	int rows;
	int cols;
	int matrA[MAX_ROWS][MAX_COLS];
	int matrB[MAX_COLS][MAX_ROWS];
	int matrC[MAX_ROWS][MAX_ROWS]; /*product matrix*/
	int i, j;
	int bufA[MAX_COLS];
	int bufB[MAX_COLS];
	int num_workers;
	int numb_sent;
	MPI_Status status;

	
	/*MPI variables*/
	int world_size; /*number of processes*/
	int prc_rank; /*process rank*/
	int manager; /*manager rank*/
	
	/*init*/
	MPI_Init(&argc, &argv);
	rows = SIZE_ROWS;
	cols = SIZE_COLS;
	world_size = 0;
	prc_rank = 0;
	manager = 0;
	numb_sent = 0;
	
	/*get MPI stuff*/
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &prc_rank);
	num_workers = world_size - 1;
	
	if(prc_rank == 0) {
		/*inital matrixes*/
		initMatr(matrA, rows, cols);
	    initMatr(matrB, rows, cols);
		printf("Matrix A:\n");
		printMatr(matrA, rows, cols);
		printf("\nMatrix B:\n");
		printMatr(matrB, rows, cols);
		
		for(i = 1; i < MIN(rows, world_size); i++) {
			for(j = 0; j < cols; j++) {
				bufA[j] = matrA[i-1][j];
				//bufB[j] = matrB[j][i-1];
			}
			
			MPI_Send(&bufA, cols, MPI_INT, i, i-1, MPI_COMM_WORLD); 
			MPI_Send(&bufB, cols, MPI_INT, i, i-1, MPI_COMM_WORLD);
			
			/*count the number of sent rows and cols as they should be equal*/
			rows_sent++;
		}
		
		
		
	}
	else {
		
		if(prc_rank < rows) {
			MPI_Recv(&bufA, cols, MPI_INT, manager, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			MPI_Recv(&bufB, rows, MPI_INT, manager, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			
			if(status.MPI_TAG < rows) {
				
			}
		}
		
	}
	
	MPI_Finalize();
	
	return 0;
}