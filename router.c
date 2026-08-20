/////////////////////////////////////////////////////////////////////////////////////
// File       : router.c
// Language   : C/C++
// Description: Router
// Author     : Sandesh Prabhakar
// Created    : April 13, 2013
// Modified   : August 19, 2026
// Version    : 1.0
//
// Copyright (c) 2026 Sandesh Prabhakar
// All rights reserved
//////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


struct node
{
	int  cost;
	int  pred_tag;
	bool reached;

	int cell_xcoor;
	int cell_ycoor;
	int cell_layer;
	int path_cost;

	int pin1_layer;
	int pin1_xcoor;
	int pin1_ycoor;
	int pin2_layer;
 	int pin2_xcoor;	
	int pin2_ycoor;

	node *link;
};

////////////////////////////////////////////////////////////////////////
// circuit class
////////////////////////////////////////////////////////////////////////

class circuit
{
    // circuit information
    int num_nets;       // total number of nets
	int num_layers;     // total number of layers 

    node ***grid_layer1;
    node ***grid_layer2;
    node ***ref_grid_layer1;
    node ***ref_grid_layer2;
	node *wavefront;
	node **netlist;
	node *path;
    // you may wish to add more private variables here

public:
    circuit(char *);            // constructor

    // you may wish to add more function declarations (or public variables) here
    void additem_netlist (int, int, int, int, int, int, int);
	void additem_grid_layer1 (int ,int ,int , bool , int );
	void additem_grid_layer2 (int ,int ,int , bool , int );
	void additem_ref_grid_layer1 (int ,int ,int , bool , int );
	void additem_ref_grid_layer2 (int ,int ,int , bool , int );
	void additem_wavefront (int ,int ,int , int , int );
	void additem_path (int l,int ,int );
    void add_element(int , int , int , int , int );

	int  expand(int , int, int , int , int ,int ,int ,int ,int , int , int , int ,int , int, int* );	
	int  backtrace(int , int , int , int , int , int , int , int , int , int );
	void delete_wavefront_cell ();
	void delete_grid_layer1 (int , int);
	void delete_grid_layer2 (int , int);
	void delete_ref_grid_layer1 (int, int);
	void delete_ref_grid_layer2 (int, int);
	void delete_path ();
};

//void strcpy (char* , char* );
//char* strcat(char* , char const* );
////////////////////////////////////////////////////////////////////////
// constructor: reads in the *.lev file and builds basic data structure
// 		for the gate-level ckt
////////////////////////////////////////////////////////////////////////
circuit::circuit(char *cktName)
{
    FILE *inFile1;
    FILE *inFile2;
    FILE *outFile;
    char fName1[40];
    char fName2[40];
	char foName[40];
    int i, j, k, l;
    char c;
    int netnum,num_nets,num_grids, num_grid_cols, num_grid_rows, junk, num_layers;
    int f1, f4;
	int f2, f3, f5, f6;
	int bend_penalty, via_penalty;
	bool layer2=0;
	bool rch=0;
	int pred;

    strcpy(fName1, cktName);
    strcat(fName1, ".nl");
    inFile1 = fopen(fName1, "r");
    if (inFile1 == NULL)
    {
	fprintf(stderr, "Can't open .nl file\n");
	exit(-1);
    }

    strcpy(fName2, cktName);
    strcat(fName2, ".grid");
    inFile2 = fopen(fName2, "r");
    if (inFile2 == NULL)
    {
	fprintf(stderr, "Can't open .grid file\n");
	exit(-1);
    }

    strcpy(foName, cktName);
    strcat(foName, ".router.output");

    outFile = fopen(foName, "w");
    if (outFile == NULL)
    {
	fprintf(stderr, "Can't open router output file to write\n");
	exit(-1);
    }

    fscanf(inFile1, "%d", &num_nets);	// number of nets

    // allocate space for netlist data structure
	netlist = new node * [num_nets+1];

	//Initializing the netlist list
	for (i=0; i<=num_nets; i++)
			{
             netlist[i] = NULL;
			}

    // now read in the circuit
    for (i=1; i<=num_nets; i++)
    {
	fscanf(inFile1, "%d", &netnum);
	fscanf(inFile1, "%d", &f1);
	fscanf(inFile1, "%d", &f2);
	fscanf(inFile1, "%d", &f3);
	fscanf(inFile1, "%d", &f4);
	fscanf(inFile1, "%d", &f5);
	fscanf(inFile1, "%d", &f6);

	additem_netlist(netnum, f1, f2, f3, f4, f5, f6);

	// skip till end of line
	while ((c = getc(inFile1)) != '\n' && c != EOF)
	    ;
    }	// for (i...)
    fclose(inFile1);

    printf("Successfully read in netlist:\n");
    printf("%d No.of nets\n", num_nets);

    fscanf(inFile2, "%d", &num_grid_cols);	// number of grid columns
    fscanf(inFile2, "%d", &num_grid_rows);	// number of grid rows
    fscanf(inFile2, "%d", &bend_penalty);	// bend penalty
    fscanf(inFile2, "%d", &via_penalty);	// via penalty

    // allocate space for grid data structure
         grid_layer1 = new node**[num_grid_rows+1];
         grid_layer2 = new node**[num_grid_rows+1];
         ref_grid_layer1 = new node**[num_grid_rows+1];
         ref_grid_layer2 = new node**[num_grid_rows+1];
         for(int i=0;i<=num_grid_rows;i++)
			{
         	grid_layer1[i] = new node*[num_grid_cols+1];
         	grid_layer2[i] = new node*[num_grid_cols+1];
         	ref_grid_layer1[i] = new node*[num_grid_cols+1];
         	ref_grid_layer2[i] = new node*[num_grid_cols+1];
			}
	
    //Initializing the grid list
	 for(int i=0; i<num_grid_rows;i++)
	 { 
            for (int j=0; j<num_grid_cols; j++)
	    {
		    grid_layer1[i][j] = NULL;
		    grid_layer2[i][j] = NULL;
		    ref_grid_layer1[i][j] = NULL;
		    ref_grid_layer2[i][j] = NULL;
            }
        }

    // now read in the circuit
    for (i=0; i<num_grid_rows; i++)
    {

		for (j=0; j<num_grid_cols;j++)
			{
			fscanf(inFile2, "%d", &f2);
			rch=0;
			pred=0;
			additem_grid_layer1(i,j,f2,rch,pred);
			additem_ref_grid_layer1(i,j,f2,rch,pred);
			}
	}
    for (i=0; i<num_grid_rows; i++)
    {
		for (j=0; j<num_grid_cols;j++)
			{
			fscanf(inFile2, "%d", &f2);
			rch=0;
			pred=0;
			additem_grid_layer2(i,j,f2,rch,pred);
			additem_ref_grid_layer2(i,j,f2,rch,pred);
			}
	}
	
    fclose(inFile2);
	num_layers=2;
    printf("Successfully read in grid:\n");
    printf("%d No.of rows\n", num_grid_rows);
    printf("%d No.of columns\n", num_grid_cols);
    printf("%d bend penalty\n", bend_penalty);
    printf("%d via penalty\n", via_penalty);
    printf("%d No. of layers\n", num_layers);

//Router Code Starts here:
fprintf(outFile, "%d\n", num_nets);

//Initializing the wavefront and path list
wavefront = NULL;
path=NULL;

int source_x, source_y, source_l, source_c, source_p;
int target_x, target_y, target_l, target_c, target_p;

for (i=1;i<=num_nets;i++)
  {
//printf("\n I am here start %d \n",i);
printf("\n Routing net number %d ....... \n",i);

fprintf(outFile, "%d\n", i);

	source_l=netlist[i]->pin1_layer;
	source_x=netlist[i]->pin1_xcoor;
	source_y=netlist[i]->pin1_ycoor;

	target_l=netlist[i]->pin2_layer;
	target_x=netlist[i]->pin2_xcoor;
	target_y=netlist[i]->pin2_ycoor;

grid_layer1[source_y][source_x]->cost=1;
grid_layer2[source_y][source_x]->cost=1;
grid_layer1[target_y][target_x]->cost=1;
grid_layer2[target_y][target_x]->cost=1;
ref_grid_layer1[source_y][source_x]->cost=1;
ref_grid_layer2[source_y][source_x]->cost=1;
ref_grid_layer1[target_y][target_x]->cost=1;
ref_grid_layer2[target_y][target_x]->cost=1;


	additem_wavefront(source_l,source_x,source_y,source_c,source_p); 

	int return_flag=0;
int *btrace_flag;
    btrace_flag = new int[1];
	   btrace_flag[0]=0;

	return_flag=expand(source_l, source_x, source_y, source_c, source_p, target_l,target_x,target_y,target_c, num_grid_cols, num_grid_rows, num_layers, bend_penalty, via_penalty, btrace_flag);

if(btrace_flag[0]==1)
  {
		   node *current = path;
		   while(current != NULL)
		     {
				fprintf(outFile,"%d %d %d\n", current->cell_layer, current->cell_xcoor, current->cell_ycoor );
				if(current->cell_layer==1)
					{
					delete_ref_grid_layer1(current->cell_ycoor,current->cell_xcoor);
					additem_ref_grid_layer1(current->cell_ycoor,current->cell_xcoor,-1,0,0);
					}
				else if(current->cell_layer==2)
					{
					delete_ref_grid_layer2(current->cell_ycoor,current->cell_xcoor);
					additem_ref_grid_layer2(current->cell_ycoor,current->cell_xcoor,-1,0,0);
					}
				current = current->link;
	         }
  }

	delete_ref_grid_layer1(source_y,source_x);
	additem_ref_grid_layer1(source_y,source_x,-1,0,0);
	delete_ref_grid_layer1(target_y,target_x);
	additem_ref_grid_layer1(target_y,target_x,-1,0,0);
	delete_ref_grid_layer2(source_y,source_x);
	additem_ref_grid_layer2(source_y,source_x,-1,0,0);
	delete_ref_grid_layer2(target_y,target_x);
	additem_ref_grid_layer2(target_y,target_x,-1,0,0);

fprintf(outFile, "%d\n", 0);
delete_path();

   for(k=0;k<num_grid_rows;k++)
		{
		for(l=0;l<num_grid_cols;l++)
			{
			delete_grid_layer1(k,l);
			additem_grid_layer1(k,l,ref_grid_layer1[k][l]->cost, ref_grid_layer1[k][l]->reached,ref_grid_layer1[k][l]->pred_tag);

			if(num_layers==2)
				{
					delete_grid_layer2(k,l);
					additem_grid_layer2(k,l,ref_grid_layer2[k][l]->cost, ref_grid_layer2[k][l]->reached,ref_grid_layer2[k][l]->pred_tag);
				}
			}
		}

printf("\n Net number %d routed... \n",i);

  }

    fclose(outFile);

}

////////////////////////////////////////////////////////////////////////
// other member functions here
////////////////////////////////////////////////////////////////////////

int circuit::expand(int source_l, int source_x, int source_y, int source_c, int source_p,int target_l,int target_x,int target_y,int target_c, int num_grid_cols, int num_grid_rows, int num_layers,int bend_penalty, int via_penalty, int* btrace_flag )
{
	int temp_l,temp_x,temp_y,temp_c, temp_p;
	temp_l=wavefront->cell_layer;
	temp_x=wavefront->cell_xcoor;
	temp_y=wavefront->cell_ycoor;
	temp_c=wavefront->path_cost;
	temp_p=wavefront->pred_tag;

if((temp_x==target_x)&&(temp_y==target_y)&&(temp_l==target_l))
  {
	btrace_flag[0]=1;
	int flag=0;
	additem_path(temp_l,temp_x,temp_y);
	flag=backtrace(source_l, source_x, source_y, source_c, source_p, temp_l, temp_x, temp_y, temp_c, temp_p);
	if(flag==1)
	return 1;
 }
	
int flag=0;
int expand_l, expand_x,expand_y, expand_c, expand_p;
int grid_c, grid_p;
bool grid_r;
int bck_l,bck_x,bck_y,bck_c, bck_p;
expand_l=temp_l;

delete_wavefront_cell();

//West 4
if(temp_p!=3)
	{
	expand_x=temp_x-1;
	expand_y=temp_y;
	expand_c=temp_c;
		if(expand_x>=0)
 			 {
			if(expand_l==1)
				{
				grid_c=grid_layer1[expand_y][expand_x]->cost;
				grid_r=grid_layer1[expand_y][expand_x]->reached;
				grid_p=grid_layer1[expand_y][expand_x]->pred_tag;
				}		
			else
		  		{	
				grid_c=grid_layer2[expand_y][expand_x]->cost;
				grid_r=grid_layer2[expand_y][expand_x]->reached;
				grid_p=grid_layer2[expand_y][expand_x]->pred_tag;
				}
			  }
		if((expand_x>=0)&&(grid_c!=-1)&&(grid_r!=1))
			{
			if(expand_l==1)
				{	
	    		delete_grid_layer1(expand_y,expand_x);	
				}
			else
				{
	    		delete_grid_layer2(expand_y,expand_x);	
				}

			grid_r=1;
			if((temp_p==0)||(temp_p==4))
				{
				expand_c=expand_c+grid_c;
			    grid_p=4;
				expand_p=grid_p;
				}
			else
				{
				expand_c=expand_c+grid_c+bend_penalty;
			    grid_p=4;
				expand_p=grid_p;
				}


			if(expand_l==1)
				{
				additem_grid_layer1(expand_y,expand_x,grid_c,grid_r,grid_p);
				}
			else
				{
				additem_grid_layer2(expand_y,expand_x,grid_c,grid_r,grid_p);
				}

//Adding the new element, making sure that the first element in the wavefront has the lowest cost
            add_element(expand_l,expand_x,expand_y,expand_c,expand_p);


			}

	}//West ends


//East 3 
if(temp_p!=4)
 	 {
		expand_x=temp_x+1;
		expand_y=temp_y;
		expand_c=temp_c;

			if(expand_x<num_grid_cols)
  				{
					if(expand_l==1)
						{
						grid_c=grid_layer1[expand_y][expand_x]->cost;
						grid_r=grid_layer1[expand_y][expand_x]->reached;
						grid_p=grid_layer1[expand_y][expand_x]->pred_tag;
						}
					else
  						{
						grid_c=grid_layer2[expand_y][expand_x]->cost;
						grid_r=grid_layer2[expand_y][expand_x]->reached;
						grid_p=grid_layer2[expand_y][expand_x]->pred_tag;
						}
  				}
			if(expand_x<num_grid_cols&&(grid_c!=-1)&&(grid_r!=1))
  				{

					if(expand_l==1)
						{
					    delete_grid_layer1(expand_y,expand_x);	
						}
					else
						{
					    delete_grid_layer2(expand_y,expand_x);	
						}

					grid_r=1;
					if((temp_p==0)||(temp_p==3))
						{
						expand_c=expand_c+grid_c;
					    grid_p=3;
						expand_p=grid_p;
						}
					else
						{
						expand_c=expand_c+grid_c+bend_penalty;
			   		 	grid_p=3;
						expand_p=grid_p;
						}
					if(expand_l==1)
						{
						additem_grid_layer1(expand_y,expand_x,grid_c,grid_r,grid_p);
						}
					else
						{
						additem_grid_layer2(expand_y,expand_x,grid_c,grid_r,grid_p);
						}

			//Adding the new element, making sure that the first element in the wavefront has the lowest cost
            add_element(expand_l,expand_x,expand_y,expand_c,expand_p);

 			 }

 	 }//East ends


//North 1
if(temp_p!=2)
 		 {
		expand_x=temp_x;
		expand_y=temp_y+1;
		expand_c=temp_c;

			if(expand_y<num_grid_rows)
  				{
					if(expand_l==1)
							{
							grid_c=grid_layer1[expand_y][expand_x]->cost;
							grid_r=grid_layer1[expand_y][expand_x]->reached;
							grid_p=grid_layer1[expand_y][expand_x]->pred_tag;
							}
					else
  							{
							grid_c=grid_layer2[expand_y][expand_x]->cost;
							grid_r=grid_layer2[expand_y][expand_x]->reached;
							grid_p=grid_layer2[expand_y][expand_x]->pred_tag;
							}
  				}

			if(expand_y<num_grid_rows&&(grid_c!=-1)&&(grid_r!=1))
  				{
					if(expand_l==1)
							{
						    delete_grid_layer1(expand_y,expand_x);	
							}
					else
							{
						    delete_grid_layer2(expand_y,expand_x);	
							}

					grid_r=1;

					if((temp_p==0)||(temp_p==1))
							{
							expand_c=expand_c+grid_c;
						    grid_p=1;
							expand_p=grid_p;
							}
					else
							{
							expand_c=expand_c+grid_c+bend_penalty;
						    grid_p=1;
							expand_p=grid_p;
							}
	 			 	if(expand_l==1)
							{
							additem_grid_layer1(expand_y,expand_x,grid_c,grid_r,grid_p);
							}
					else
							{
							additem_grid_layer2(expand_y,expand_x,grid_c,grid_r,grid_p);
							}

					//Adding the new element, making sure that the first element in the wavefront has the lowest cost
            		add_element(expand_l,expand_x,expand_y,expand_c,expand_p);

 			 }

  	}//North ends


//South 2
if(temp_p!=1)
  		{
		expand_x=temp_x;
		expand_y=temp_y-1;
		expand_c=temp_c;

			if(expand_y>=0)
				  {
					if(expand_l==1)
							{
								grid_c=grid_layer1[expand_y][expand_x]->cost;
								grid_r=grid_layer1[expand_y][expand_x]->reached;
								grid_p=grid_layer1[expand_y][expand_x]->pred_tag;
							}
					else
  							{
								grid_c=grid_layer2[expand_y][expand_x]->cost;
								grid_r=grid_layer2[expand_y][expand_x]->reached;
								grid_p=grid_layer2[expand_y][expand_x]->pred_tag;
							}
  				}

			if(expand_y>=0&&(grid_c!=-1)&&(grid_r!=1))
  				{
						if(expand_l==1)
							{
	    					delete_grid_layer1(expand_y,expand_x);	
							}
						else
							{
	  					  delete_grid_layer2(expand_y,expand_x);	
							}

						grid_r=1;
						if((temp_p==0)||(temp_p==2))
							{
								expand_c=expand_c+grid_c;
			 				    grid_p=2;
								expand_p=grid_p;
							}
						else
							{
							expand_c=expand_c+grid_c+bend_penalty;
			   				grid_p=2;
							expand_p=grid_p;
							}
						if(expand_l==1)
							{
							additem_grid_layer1(expand_y,expand_x,grid_c,grid_r,grid_p);
							}
							else
							{
							additem_grid_layer2(expand_y,expand_x,grid_c,grid_r,grid_p);
							}
						//Adding the new element, making sure that the first element in the wavefront has the lowest cost
                      	 add_element(expand_l,expand_x,expand_y,expand_c,expand_p);

  				}

 	 }//South ends


if((num_layers==2))
  {

	//Up 5
	if(temp_p!=6)
			{

			expand_x=temp_x;
			expand_y=temp_y;
			expand_c=temp_c;

			grid_c=grid_layer2[expand_y][expand_x]->cost;
			grid_r=grid_layer2[expand_y][expand_x]->reached;
			grid_p=grid_layer2[expand_y][expand_x]->pred_tag;

			if((grid_c!=-1)&&(grid_r!=1))
					{
					expand_l=2;
				    delete_grid_layer2(expand_y,expand_x);	

					grid_r=1;
					expand_c=expand_c+grid_c+via_penalty;
			    	grid_p=5;
					expand_p=grid_p;

					additem_grid_layer2(expand_y,expand_x,grid_c,grid_r,grid_p);


					//Adding the new element, making sure that the first element in the wavefront has the lowest cost
		            add_element(expand_l,expand_x,expand_y,expand_c,expand_p);
	
					}

			}//Up ends

	//Down 6
	if(temp_p!=5)
		{

			expand_x=temp_x;
			expand_y=temp_y;
			expand_c=temp_c;

			grid_c=grid_layer1[expand_y][expand_x]->cost;
			grid_r=grid_layer1[expand_y][expand_x]->reached;
			grid_p=grid_layer1[expand_y][expand_x]->pred_tag;

			if((grid_c!=-1)&&(grid_r!=1))
					{
					expand_l=1;
	    			delete_grid_layer1(expand_y,expand_x);	

					grid_r=1;
					expand_c=expand_c+grid_c+via_penalty;
			    	grid_p=6;
					expand_p=grid_p;

					additem_grid_layer1(expand_y,expand_x,grid_c,grid_r,grid_p);

					//Adding the new element, making sure that the first element in the wavefront has the lowest cost
            add_element(expand_l,expand_x,expand_y,expand_c,expand_p);
	

					}

		}//Down ends

  }//Up and down ends if two layers are there

int return_flag=0;
if(wavefront==NULL)
{
	return 1;
}
	return_flag=expand(source_l, source_x, source_y, source_c, source_p,target_l,target_x,target_y,target_c, num_grid_cols, num_grid_rows, num_layers, bend_penalty, via_penalty, btrace_flag);

if(return_flag==1)
	return 1;

}



int circuit::backtrace(int source_l, int source_x, int source_y, int source_c, int source_p, int expand_l, int expand_x, int expand_y, int expand_c, int expand_p)
	{
	int flag=0;
int temp_x, temp_y, temp_c, temp_p, temp_l;

	if(expand_p==1)
		{
		temp_x=expand_x;
		temp_y=expand_y-1;
		temp_l=expand_l;
		if(temp_l==1)
			{
			temp_p=grid_layer1[temp_y][temp_x]->pred_tag;
			temp_c=grid_layer1[temp_y][temp_x]->cost;
			}
		else
			{
			temp_p=grid_layer2[temp_y][temp_x]->pred_tag;
			temp_c=grid_layer2[temp_y][temp_x]->cost;
			}

		if((temp_x==source_x)&&(temp_y==source_y)&&(temp_l==source_l))
			{
			additem_path(temp_l,temp_x,temp_y);
			return 1;
			}
		else
			{
			additem_path(temp_l,temp_x,temp_y);
			flag=backtrace(source_l, source_x, source_y, source_c, source_p, temp_l, temp_x, temp_y, temp_c, temp_p);
			}
	
		}

	if(expand_p==2)
		{
		temp_x=expand_x;
		temp_y=expand_y+1;
		temp_l=expand_l;
		if(temp_l==1)
			{
			temp_p=grid_layer1[temp_y][temp_x]->pred_tag;
			temp_c=grid_layer1[temp_y][temp_x]->cost;
			}
		else
			{
			temp_p=grid_layer2[temp_y][temp_x]->pred_tag;
			temp_c=grid_layer2[temp_y][temp_x]->cost;
			}

		if((temp_x==source_x)&&(temp_y==source_y)&&(temp_l==source_l))
			{
			additem_path(temp_l,temp_x,temp_y);
			return 1;
			}
		else
			{
			additem_path(temp_l,temp_x,temp_y);
			flag=backtrace(source_l, source_x, source_y, source_c, source_p, temp_l, temp_x, temp_y, temp_c, temp_p);
			}
	

		}

	if(expand_p==3)
		{
		temp_x=expand_x-1;
		temp_y=expand_y;
		temp_l=expand_l;
		if(temp_l==1)
			{
			temp_p=grid_layer1[temp_y][temp_x]->pred_tag;
			temp_c=grid_layer1[temp_y][temp_x]->cost;
			}
		else
			{
			temp_p=grid_layer2[temp_y][temp_x]->pred_tag;
			temp_c=grid_layer2[temp_y][temp_x]->cost;
			}

		if((temp_x==source_x)&&(temp_y==source_y)&&(temp_l==source_l))
			{
			additem_path(temp_l,temp_x,temp_y);
			return 1;
			}
		else
			{
			additem_path(temp_l,temp_x,temp_y);
			flag=backtrace(source_l, source_x, source_y, source_c, source_p, temp_l, temp_x, temp_y, temp_c, temp_p);
			}


		}

	if(expand_p==4)
		{
		temp_x=expand_x+1;
		temp_y=expand_y;
		temp_l=expand_l;
		if(temp_l==1)
			{
			temp_p=grid_layer1[temp_y][temp_x]->pred_tag;
			temp_c=grid_layer1[temp_y][temp_x]->cost;
			}
		else
			{
			temp_p=grid_layer2[temp_y][temp_x]->pred_tag;
			temp_c=grid_layer2[temp_y][temp_x]->cost;
			}

		if((temp_x==source_x)&&(temp_y==source_y)&&(temp_l==source_l))
			{
			additem_path(temp_l,temp_x,temp_y);
			return 1;
			}
		else
			{
			additem_path(temp_l,temp_x,temp_y);
			flag=backtrace(source_l, source_x, source_y, source_c, source_p, temp_l, temp_x, temp_y, temp_c, temp_p);
			}

		}

	if(expand_p==5)
		{
	    temp_x=expand_x;	
		temp_y=expand_y;
	    temp_l=1;	
		temp_p=grid_layer1[temp_y][temp_x]->pred_tag;

		if((temp_x==source_x)&&(temp_y==source_y)&&(temp_l==source_l))
			{
			additem_path(temp_l,temp_x,temp_y);
			return 1;
			}
		else
			{
			additem_path(3,temp_x,temp_y);
			additem_path(temp_l,temp_x,temp_y);
			flag=backtrace(source_l, source_x, source_y, source_c, source_p, temp_l, temp_x, temp_y, temp_c, temp_p);
			}
	
		}

	if(expand_p==6)
		{
	    temp_x=expand_x;	
		temp_y=expand_y;
	    temp_l=2;	
		temp_p=grid_layer2[temp_y][temp_x]->pred_tag;

		if((temp_x==source_x)&&(temp_y==source_y)&&(temp_l==source_l))
			{
			additem_path(temp_l,temp_x,temp_y);
			additem_path(100,0,0);
			return 1;
			}
		else
			{
			additem_path(3,temp_x,temp_y);
			additem_path(temp_l,temp_x,temp_y);
			flag=backtrace(source_l, source_x, source_y, source_c, source_p, temp_l, temp_x, temp_y, temp_c, temp_p);
			}
	
		}

	}


void circuit::add_element(int expand_l, int expand_x, int expand_y, int expand_c, int expand_p)
	{
			node *storeNode = new node;
			node *temp = new node;
			int index=0;
			storeNode = wavefront;	
			if(wavefront==NULL)
			{
			additem_wavefront(expand_l,expand_x,expand_y,expand_c,expand_p);
			}
			else
			{
	        while(wavefront!=NULL)	
					{
						if(expand_c<=(wavefront->path_cost))
							{
								additem_wavefront(expand_l,expand_x,expand_y,expand_c,expand_p);
								if(index!=0)
									{
									temp->link=wavefront;
									wavefront=storeNode;
									}
							break;			
							}
						else
							{
							temp=wavefront;
							wavefront=wavefront->link;
							}
					index=index+1;
					}

			if(wavefront==NULL)
  					{
				    additem_wavefront(expand_l,expand_x,expand_y,expand_c,expand_p);
					temp->link=wavefront;
					wavefront=storeNode;
  					}
			}
	}

void circuit::additem_netlist (int netnum,int pin1layer,int pin1xcoor,int pin1ycoor,int pin2layer,int pin2xcoor, int pin2ycoor)

	  {

		  		node *newlink = new node;

		  		newlink->pin1_layer = pin1layer;
		  		newlink->pin1_xcoor = pin1xcoor; 
		  		newlink->pin1_ycoor = pin1ycoor; 
		  		newlink->pin2_layer = pin2layer;
		  		newlink->pin2_xcoor = pin2xcoor; 
		  		newlink->pin2_ycoor = pin2ycoor; 

		  		newlink->link = netlist[netnum];
		        netlist[netnum] = newlink;
		 
	  }

void circuit::additem_grid_layer1 (int i,int j,int f2, bool rch, int pred)

	  {

		  		node *newlink1 = new node;

		  		newlink1->cost = f2;
		  		newlink1->pred_tag = pred; 
		  		newlink1->reached = rch; 

		  		newlink1->link = grid_layer1[i][j];
		        grid_layer1[i][j] = newlink1;
		 
	  }

void circuit::additem_grid_layer2 (int i,int j,int f2, bool rch, int pred)

	  {

		  		node *newlink2 = new node;

		  		newlink2->cost = f2;
		  		newlink2->pred_tag = pred; 
		  		newlink2->reached = rch; 

		  		newlink2->link = grid_layer2[i][j];
		        grid_layer2[i][j] = newlink2;
		 
	  }

void circuit::additem_ref_grid_layer1 (int i,int j,int f2, bool rch, int pred)

	  {

		  		node *newlink1 = new node;

		  		newlink1->cost = f2;
		  		newlink1->pred_tag = pred; 
		  		newlink1->reached = rch; 

		  		newlink1->link = ref_grid_layer1[i][j];
		        ref_grid_layer1[i][j] = newlink1;
		 
	  }

void circuit::additem_ref_grid_layer2 (int i,int j,int f2, bool rch, int pred)

	  {

		  		node *newlink2 = new node;

		  		newlink2->cost = f2;
		  		newlink2->pred_tag = pred; 
		  		newlink2->reached = rch; 

		  		newlink2->link = ref_grid_layer2[i][j];
		        ref_grid_layer2[i][j] = newlink2;
		 
	  }

void circuit::additem_wavefront (int l,int x,int y, int c, int p)

	  {

		  		node *newlink3 = new node;

		  		newlink3->cell_layer = l;
		  		newlink3->cell_xcoor = x; 
		  		newlink3->cell_ycoor = y; 
		  		newlink3->path_cost = c; 
		  		newlink3->pred_tag = p; 

		  		newlink3->link = wavefront;
		        wavefront = newlink3;
		 
	  }

void circuit::additem_path (int l,int x,int y)

	  {

		  		node *newlink4 = new node;

		  		newlink4->cell_layer = l;
		  		newlink4->cell_xcoor = x; 
		  		newlink4->cell_ycoor = y; 

		  		newlink4->link = path;
		        path = newlink4;
		 
	  }

void circuit::delete_wavefront_cell ()

	  	 {
                        node *delNode = new node;
						delNode = wavefront;
						wavefront = wavefront->link;
						delete delNode;
         }

void circuit::delete_grid_layer1 (int i, int j)

	  	 {
                        node *delNode = new node;
						delNode = grid_layer1[i][j];
						grid_layer1[i][j] = grid_layer1[i][j]->link;
						delete delNode;
         }

void circuit::delete_grid_layer2 (int i, int j)

	  	 {
                        node *delNode = new node;
						delNode = grid_layer2[i][j];
						grid_layer2[i][j] = grid_layer2[i][j]->link;
						delete delNode;
         }

void circuit::delete_ref_grid_layer1 (int i, int j)

	  	 {
                        node *delNode = new node;
						delNode = ref_grid_layer1[i][j];
						ref_grid_layer1[i][j] = ref_grid_layer1[i][j]->link;
						delete delNode;
         }

void circuit::delete_ref_grid_layer2 (int i, int j)

	  	 {
                        node *delNode = new node;
						delNode = ref_grid_layer2[i][j];
						ref_grid_layer2[i][j] = ref_grid_layer2[i][j]->link;
						delete delNode;
         }

void circuit::delete_path ()

	  	 {
                        node *delNode = new node;
						while(path != NULL)
						{
						delNode = path;
						path = path->link;
						delete delNode;
						}
         }


//void strcpy (char *s, char *t)
//{
//    while (*s++ = *t++)
//	;
//}


//char * strcat ( char * destination, const char * source )
//{
//char *d = destination;
//while (*d) ++d;
//while ((*d++ = *source++) != '\0') ;
//return (destination);
//}
////////////////////////////////////////////////////////////////////////
// main starts here
////////////////////////////////////////////////////////////////////////
main(int argc, char *argv[])
{
    char cktName[20];
    circuit *ckt;
    int vecWidth, poNum;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ckt>\n", argv[0]);
        exit(-1);
    }

    strcpy(cktName, argv[1]);

    // read in the circuit and build data structure
    ckt = new circuit(cktName);

    // NOW ADD YOUR FUNCTION HERE
}
