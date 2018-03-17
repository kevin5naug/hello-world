#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

/*** Skeleton for Lab 1 ***/

/***** Globals ******/
float **a; /* The coefficients */
float *new_a; /*The coefficients for broadcast use*/
float *x;  /* The unknowns */
float *b;  /* The constants */
float err; /* The absolute relative error */
int num = 0;  /* number of unknowns */
int *assigned_num_of_x; /*number of unkowns tracked by each core(the workload)*/

/****** Function declarations */
void check_matrix(); /* Check whether the matrix will converge */
void get_input(int, int, char*);  /* Read input from file */

/********************************/



/* Function definitions: functions are ordered alphabetically ****/
/*****************************************************************/

/* 
   Conditions for convergence (diagonal dominance):
   1. diagonal element >= sum of all other elements of the row
   2. At least one diagonal element > sum of all other elements of the row
 */
void check_matrix()
{
  int bigger = 0; /* Set to 1 if at least one diag element > sum  */
  int i, j;
  float sum = 0;
  float aii = 0;
  
  for(i = 0; i < num; i++)
  {
    sum = 0;
    aii = fabs(a[i][i]);
    
    for(j = 0; j < num; j++)
       if( j != i)
	 sum += fabs(a[i][j]);
       
    if( aii < sum)
    {
      printf("The matrix will not converge.\n");
      exit(1);
    }
    
    if(aii > sum)
      bigger++;
    
  }
  
  if( !bigger )
  {
     printf("The matrix will not converge\n");
     exit(1);
  }
}


/******************************************************/
/* Read input from file */
/* After this function returns:
 * a[][] will be filled with coefficients and you can access them using a[i][j] for element (i,j)
 * x[] will contain the initial values of x
 * b[] will contain the constants (i.e. the right-hand-side of the equations
 * num will have number of variables
 * err will have the absolute error that you need to reach
 */
void get_input(int my_rank, int comm_sz, char filename[])
{
  FILE * fp;
  int i,j;  
  /*Only master core read the file*/
  if(my_rank==0){
    fp = fopen(filename, "r");
    if(!fp)
    {
	printf("Cannot open file %s\n", filename);
	exit(1);
    }

    fscanf(fp,"%d ",&num);
    MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
    fscanf(fp,"%f ",&err);
    MPI_Bcast(&err, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
  }else{
    MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&err, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
  }

 /* Now, time to allocate the matrices and vectors */
 /* all cores should reserve the space for variables*/
 a = (float**)malloc(num * sizeof(float*));
 if( !a)
  {
	printf("Cannot allocate a!\n");
	exit(1);
  }

 for(i = 0; i < num; i++) 
  {
    a[i] = (float *)malloc(num * sizeof(float)); 
    if( !a[i])
  	{
		printf("Cannot allocate a[%d]!\n",i);
		exit(1);
  	}
  }
 
 x = (float *) malloc(num * sizeof(float));
 if( !x)
  {
	printf("Cannot allocate x!\n");
	exit(1);
  }


 b = (float *) malloc(num * sizeof(float));
 if( !b)
  {
	printf("Cannot allocate b!\n");
	exit(1);
  }

 assigned_num_of_x=(int *) malloc(num * sizeof(int));
 if( !assigned_num_of_x)
 {
     printf("Cannot allocate assigned_num_of_x!\n");
     exit(1);
 }

 /* Now .. Filling the blanks */ 
 /*Only the master core should fill the blanks and close the file. Other cores receive the data*/
 
 if(my_rank==0){
    /* The initial values of Xs */
    for(i = 0; i < num; i++)
    {
	fscanf(fp,"%f ", &x[i]);
    }
    for(i = 0; i < num; i++)
    {
	for(j = 0; j < num; j++){
	    fscanf(fp,"%f ",&a[i][j]);
	}
   
	/* reading the b element */
	fscanf(fp,"%f ",&b[i]);
    }
    fclose(fp);
    
    /*check the convergence of our method first before broadcasting*/
    /*if fails, no need to proceed. exit directly*/
    check_matrix();

    /*todo:broadcast the data*/
    /*to broadcast the coefficient Aij, we have to first ensure the memory of Aij is contiguous*/
    int k=0;
    new_a=(float *)malloc(num*num*sizeof(float));
    if(!new_a)
    {
	printf("Cannot allocate new_a!\n");
	exit(1);
    }
    for(int i=0;i<num;i++){
	for(int j=0;j<num;j++){
	    new_a[k]=a[i][j];
	    k++;
	}
    }
    MPI_Bcast(new_a, num*num, MPI_FLOAT, 0, MPI_COMM_WORLD);
    free(new_a);

    int base_num=num/comm_sz;
    int remainder=num%comm_sz;
    /*next, we have to decide the workload for each core*/
    for(int i=0;i<num;i++){
	if(i<remainder){
	    assigned_num_of_x[i]=base_num+1;
	}else{
	    assigned_num_of_x[i]=base_num;
	}
    }
    MPI_Bcast(assigned_num_of_x, num, MPI_INT, 0, MPI_COMM_WORLD);

    /*Third, we broad cast the arrays b[] and x[]*/
    MPI_Bcast(b, num, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(x, num, MPI_FLOAT, 0, MPI_COMM_WORLD);

 }else{
    /*todo:receive the broadcasted data*/
    new_a=(float *)malloc(num*num*sizeof(float));
    MPI_Bcast(new_a, num*num, MPI_FLOAT, 0, MPI_COMM_WORLD);
    int k=0;
    for(int i=0;i<num;i++){
	for(int j=0;j<num;j++){
	    a[i][j]=new_a[k];
	    k++;
	}
    }
    free(new_a);

    MPI_Bcast(assigned_num_of_x, num, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(b, num, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(x, num, MPI_FLOAT, 0, MPI_COMM_WORLD);
 }
}

int check_err(float *current_x, float *next_x){
    float cur, next, temp;
    int result=0;
    for(int i=0;i<num;i++){
	cur=current_x[i];
	next=next_x[i];
	temp=fabsf((next-cur)/next);
	if(temp>err){
	    result=1;
	}
	current_x[i]=next;
    }
    return result;
}
/************************************************************/


int main(int argc, char *argv[])
{
 
 int i;
 int nit = 0; /* number of iterations */
 FILE * fp;
 char output[100] ="";
  
 if( argc != 2)
 {
   printf("Usage: ./gsref filename\n");
   exit(1);
 }
 
 int comm_sz;
 int my_rank;
 MPI_Init(&argc, &argv);
 MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
 MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
 /* Read the input file and fill the global data structure above */ 
 get_input(my_rank, comm_sz, argv[1]);
 

 /*Since we check the convergence condition in get_input, no need to check it again on each core*/
 /* Check for convergence condition */
 /* This function will exit the program if the coffeicient will never converge to 
  * the needed absolute error. 
  * This is not expected to happen for this programming assignment.
  */
 
 //check_matrix();
 
 /*now we start our iteration method*/
 float new_assigned_x[assigned_x_num];
 float current_x[num];
 float next_x[num];
 int assigned_x_num=assigned_num_of_x[my_rank];
 int displacement=0;
 int dp[num];
 int index=0;
 int sum=0;
 for(int i=0;i<my_rank;i++){
    displacement+=assigned_num_of_x[i];
 }
 for(int i=0;i<num;i++){
    current_x[i]=x[i];
    if(i==0){
	dp[i]=0;
    }else{
	dp[i]=dp[i-1]+assigned_num_of_x[i-1];
    }
 }
 do{
    for(int i=0;i<assigned_x_num;i++){
	index=displacement+i;
	sum=0;
	for(int j=0;j<num;i++){
	    if(j==0){
		sum=sum+b[index];
	    }else{
		sum=sum-a[index][j]*current_x[i];
	    }
	}
	sum=sum/a[index][index];
	new_assigned_x[i]=sum;
    }
    MPI_Allgatherv(new_assigned_x, assigned_x_num, MPI_FLOAT, next_x, assigned_num_of_x, dp, MPI_FLOAT, MPI_COMM_WORLD);
    nit++;
    /*we swap current_x with next_x when we check for the error*/
 }while(check_err(current_x, next_x));

 
 if(my_rank==0){
     /* Writing results to file */
     sprintf(output,"%d.sol",num);
     fp = fopen(output,"w");
     if(!fp)
     {
       printf("Cannot create the file %s\n", output);
       exit(1);
     }
	
     for( i = 0; i < num; i++)
       fprintf(fp,"%f\n", current_x[i]);
     
     printf("total number of iterations: %d\n", nit);
     
     fclose(fp);
 }
 
 free(a);
 free(x);
 free(b);
 free(assigned_num_of_x);

 MPI_Barrier(MPI_COMM_WORLD);
 MPI_Finalize();

 exit(0);

}
