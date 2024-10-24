#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

#define RANGE 20.0

void check_correctness(float *, int *, int, int );

int main(int argc, char *argv[])
{
	
	double start, end, local_time, elapsed;
	int i; // loop index
	int num_items = 0;
	int num_bins = 0;
	float * data; // array containing data items
	int * bins; // the histogram itself
    int * local_bins; // local histogram for each process
	int myrank, numprocs, local_num_items;
	
	
	
	if(argc != 3)
	{
		printf("You need to enter two items (in the following orders):\n");
		printf("Number of data items: [1, 10000000000]\n");
		printf("Number of bins: positive integer\n");
		return 1;
	}
	
	num_items = atoi(argv[1]);
	num_bins = atoi(argv[2]);
	
	
	MPI_Init(&argc, &argv);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

	// Allocate memory for local histogram
    local_bins = (int *)calloc(num_bins, sizeof(int));
    if (!local_bins) {
        printf("Error allocating local bins\n");
        MPI_Finalize();
        exit(1);
    }
	
	// Only process 0 generates the data.
	if(myrank == 0){
		
	      //Allocating the arrays containing the data items and the bins
	      if( !(data = (float *)calloc(num_items, sizeof(float))) )
	      {
			printf("Error allocating data items array\n");
			exit(1);
		  }
	
		  if( !(bins = (int *)calloc(num_bins, sizeof(int))) )
			{
				printf("Error allocating array of bins\n");
				exit(1);
			}
	
		/* Now, fill up the data items */
		srand(time(NULL));
		for( i = 0; i < num_items; i++)
			data[i] = ((float)rand()/(float)(RAND_MAX)) * RANGE;
	
	
	} // end if
	
	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime();
	
    // Broadcast the number of items and bins to all processes
    MPI_Bcast(&num_items, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num_bins, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Calculate how many items each process will handle
    local_num_items = num_items / numprocs;
    
    float * local_data = (float *)malloc(local_num_items * sizeof(float));
    
    // Scatter the data to all processes
    MPI_Scatter(data, local_num_items, MPI_FLOAT, local_data, local_num_items, MPI_FLOAT, 0, MPI_COMM_WORLD);
    
    // Compute local histogram
    float bin_range = RANGE / num_bins;
    for (i = 0; i < local_num_items; i++) {
        int bin_index = local_data[i] / bin_range;
        if (bin_index == num_bins) bin_index--; // Handle edge case where data equals RANGE
        local_bins[bin_index]++;
    }
    
    // Reduce the local histograms to get the global histogram
    MPI_Reduce(local_bins, bins, num_bins, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	/* Do not modify the code from here: */
	end = MPI_Wtime();
    local_time = end-start;
	/* Assuming process 0 will end up having the time elapsed to be measured. You can change that if you want. */
	MPI_Reduce(&local_time, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	if (myrank == 0)
		check_correctness(data, bins, num_items, num_bins);
	/* to here ... */
	
    if(myrank == 0) {
        printf("Time taken = %f seconds\n", elapsed);
        for(i = 0; i < num_bins; i++) {
            printf("histogram[%d] = %d\n", i, bins[i]);
        }
        
        // Free data memory
        free(data);
        free(bins);
    }
    
    // Free allocated memory
    free(local_data);
    free(local_bins);
    
    MPI_Finalize();
	
	
	return 0;

}


/*** Do not modify the following function **/
void check_correctness(float * data, int * bins, int num_items, int num_bins){
	
	int i;
	int * refbins;
	float binrange;
	
	binrange = (float)(RANGE)/(float)(num_bins);
	
	if( !(refbins = (int *)calloc(num_bins, sizeof(int))) )
	{
	   printf("Error allocating array of refbins\n");
	   exit(1);
	}
	
	for(i = 0; i < num_items; i++)
		refbins[(int)(data[i]/binrange)]++;
	
	for(i = 0; i < num_bins; i++)
		if(bins[i] != refbins[i])
			printf("Mistmatch in element %d of the histogram.... your item = %d ... the correct is %d\n", i, bins[i], refbins[i]);
	
}



/*******************************************/