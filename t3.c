#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "err_code.h"

extern int output_device_info(cl_device_id);

//#define LENGTH (1024*1024*10)    // length of vectors a, b, and c

const char *KernelSource = "\n" \
"__kernel void pythgrsTrplts(                                            \n" \
"   __global int* a,                                                     \n" \
"   __global int b_lim,                                                  \n" \
"   __global int c_lim,                                                  \n" \
"   const unsigned int count)                                            \n" \
"{                                                                       \n" \
"   int[b_lim] res_a;                                                    \n" \
"   int[b_lim] res_b;                                                    \n" \
"   int i = get_global_id(0);                                            \n" \
"   if(i < count){                                                       \n" \
"       int res_idx = 0;                                                 \n" \
"       for (int b = a[i] + 1; b <= b_lim; b++){                         \n" \
"            for (int c = b + 1; c <= c_lim; c++){                       \n" \
"                if(a[i]*a[i] + b*b == c*c){                             \n" \
"                    res_a[res_idx] = a[i];                              \n" \
"                    res_b[res_idx++] = b;                               \n" \
"                   break;                                               \n" \
"                }                                                       \n" \
"           }                                                            \n" \
"        }                                                               \n" \
"   }                                                                    \n" \
"}                                                                       \n" \
"\n";

//------------------------------------------------------------------------------


int main(int argc, char** argv)
{
    
    if (argc < 2) {
    	printf("\nUsage: vadd <size of vectors>\n\n");
    	exit(-1);
    }
    
    int PERIMETER = 100000;
    int err;               // error code returned from OpenCL calls
    
    int   h_var_lim_a   = (PERIMETER/3) - 1;    // max value possible for 'a'
    int   h_var_lim_b   = (PERIMETER/3);        // max value possible for 'b'
    int   h_var_lim_c   = h_var_lim_b + (PERIMETER - h_var_lim_b * 3) + 1;  // max value possible for 'c'
    int   num_poss_comb = h_var_lim_a * (h_var_lim_b - 1); 
    int*  h_a           = (int*) calloc(h_var_lim_a, sizeof(int));       // the a values to check
    int*  h_res_a       = (int*) calloc(num_poss_comb, sizeof(int));     // result vector to store the 'a' of the triplet
    int*  h_res_b       = (int*) calloc(num_poss_comb, sizeof(int));    // result vector to store the 'b' of the triplet

    double rtime;					// current time
    
    size_t global;                  // global domain size
    
    cl_device_id 	device_id;		// device id
    cl_context      context;       // compute context
    cl_command_queue commands;      // compute command queue
    cl_program      program;       // compute program
    cl_kernel       pythgrsTrplt;       // compute kernel
    
    cl_mem d_a;                     // device memory used for the input  a vector
    cl_mem d_var_lim_b;                     
    cl_mem d_var_lim_c;                     
    cl_mem d_res_a;                    
    cl_mem d_res_b;                                        
    
    // Fill vector a 
    int i = 0;
    for(i = 0; i < h_var_lim_a; i++){
        h_a[i] = i + 1;
    }
    
    // Set up platform and GPU device
    cl_uint numPlatforms;
    cl_uint numDevices;
    
    // Find number of platforms
    err = clGetPlatformIDs(0, NULL, &numPlatforms);
    checkError(err, "Finding platforms");
    if (numPlatforms == 0)
    {
        printf("Found no platforms!\n");
        return EXIT_FAILURE;
    }
    
    printf("\nNumber of OpenCL platforms: %d\n", numPlatforms);
    printf("\n-------------------------\n");
    
    // Get all platforms
    cl_platform_id Platform[numPlatforms];
    err = clGetPlatformIDs(numPlatforms, Platform, NULL);
    checkError(err, "Getting platforms");
    
    // Get all Devices
    cl_device_id Device[MAXDEVICES];
    for (i = 0; i < numPlatforms; i++)
    {
        err = clGetDeviceIDs(Platform[i], CL_DEVICE_TYPE_ALL, MAXDEVICES, Device, &numDevices);
        checkError(err, "Finding devices");
    }
    if (numDevices == 0)
    {
        printf("Found no devices!\n");
        return EXIT_FAILURE;
    }
    printf("\nNumber of OpenCL devices: %d\n", numDevices);
    printf("\n-------------------------\n");
    
    
    for (i= 0; i < numDevices; i++)
    {
        err = output_device_info(Device[i]);
        checkError(err, "Printing device output");
        
        device_id= Device[i];
        // Create a compute context
        context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
        checkError(err, "Creating context");
        
        // Create a command queue
        commands = clCreateCommandQueue(context, device_id, 0, &err);
        checkError(err, "Creating command queue");
        
        // Create the compute program from the source buffer
        program = clCreateProgramWithSource(context, 1, (const char **) &KernelSource, NULL, &err);
        checkError(err, "Creating program");
        
        // Build the program
        err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            size_t len;
            char buffer[2048];
            
            printf("Error: Failed to build program executable!\n%s\n", err_code(err));
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
            printf("%s\n", buffer);
            return EXIT_FAILURE;
        }
        

        
        // Create the input (a, b) and output (c) arrays in device memory
        d_a  = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(int) * count, NULL, &err);
        checkError(err, "Creating buffer d_a");
        
        d_var_lim_b  = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(int), NULL, &err);
        checkError(err, "Creating buffer d_var_lim_b");
        
        d_var_lim_c  = clCreateBuffer(context,  CL_MEM_READ_ONLY, sizeof(int), NULL, &err);
        checkError(err, "Creating buffer d_var_lim_c");

        d_res_a  = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,  sizeof(int) * num_poss_comb, NULL, &err);
        checkError(err, "Creating buffer d_res_a");
        
        d_res_b  = clCreateBuffer(context,  CL_MEM_WRITE_ONLY, sizeof(int) * num_poss_comb, NULL, &err);
        checkError(err, "Creating buffer d_res_b");
        
        // Write a and b vectors into compute device memory
        err = clEnqueueWriteBuffer(commands, d_a, CL_TRUE, 0, sizeof(int) * count, h_a, 0, NULL, NULL);
        checkError(err, "Copying h_a to device at d_a");
        
        err = clEnqueueWriteBuffer(commands, d_var_lim_b, CL_TRUE, 0, sizeof(int), h_var_lim_b, 0, NULL, NULL);
        checkError(err, "Copying h_var_lim_b to device at d_var_lim_b");

        err = clEnqueueWriteBuffer(commands, d_var_lim_c, CL_TRUE, 0, sizeof(int), h_var_lim_c, 0, NULL, NULL);
        checkError(err, "Copying h_var_lim_c to device at d_var_lim_c");

        err = clEnqueueWriteBuffer(commands, d_res_a, CL_TRUE, 0, sizeof(int) * num_poss_comb, h_res_a, 0, NULL, NULL);
        checkError(err, "Copying h_res_a to device at d_res_a");

        err = clEnqueueWriteBuffer(commands, d_res_b, CL_TRUE, 0, sizeof(int) * num_poss_comb, h_res_b, 0, NULL, NULL);
        checkError(err, "Copying h_res_b to device at d_res_b");
        
        // Create the compute kernel from the program
        pythgrsTrplts = clCreateKernel(program, "pythgrsTrplts", &err);
        checkError(err, "Creating kernel");

        // Set the arguments to our compute kernel
        err  = clSetKernelArg(pythgrsTrplts, 0, sizeof(cl_mem), &d_a);
        err |= clSetKernelArg(pythgrsTrplts, 1, sizeof(cl_mem), &d_var_lim_b);
        err |= clSetKernelArg(pythgrsTrplts, 2, sizeof(cl_mem), &d_var_lim_c);
        err |= clSetKernelArg(pythgrsTrplts, 3, sizeof(cl_mem), &d_res_a);
        err |= clSetKernelArg(pythgrsTrplts, 4, sizeof(cl_mem), &d_res_b);
        err |= clSetKernelArg(pythgrsTrplts, 5, sizeof(unsigned int), &count);
        checkError(err, "Setting kernel arguments");
        
        rtime = wtime();
        
        // Execute the kernel over the entire range of our 1d input data set
        // letting the OpenCL runtime choose the work-group size
        global = count;
        err = clEnqueueNDRangeKernel(commands, pythgrsTrplts, 1, NULL, &global, NULL, 0, NULL, NULL);
        checkError(err, "Enqueueing kernel");
        
        // Wait for the commands to complete before stopping the timer
        err = clFinish(commands);
        checkError(err, "Waiting for kernel to finish");
        
        rtime = wtime() - rtime;
        printf("\nThe kernel ran in %lf seconds\n",rtime);
        printf("\n-------------------------\n");
        
        // Read back the results from the compute device
        err = clEnqueueReadBuffer( commands, d_res_b, CL_TRUE, 0, sizeof(float) * num_poss_comb, h_res_b, 0, NULL, NULL );
        err = clEnqueueReadBuffer( commands, d_res_c, CL_TRUE, 0, sizeof(float) * num_poss_comb, h_res_c, 0, NULL, NULL );
        if (err != CL_SUCCESS)
        {
            printf("Error: Failed to read output array!\n%s\n", err_code(err));
            exit(1);
        }
        // cleanup then shutdown
        clReleaseMemObject(d_a);
        clReleaseMemObject(d_b);
        clReleaseMemObject(d_c);
        clReleaseProgram(program);
        clReleaseKernel(pythgrsTrplts);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
    }
    
    
    // Test the results
    correct = 0;
    float tmp;
    
    rtime = wtime();
    for(i = 0; i < count; i++)
    {
        h_C[i] = h_a[i] + h_b[i];     // assign element i of a+b to tmp
    }
    rtime = wtime() - rtime;
    printf("\nCPU serial time = %lf seconds\n",rtime);
    
    printf("\nChecking results...  ");
    for(i = 0; i < count; i++)
    {
        tmp = h_C[i] - h_c[i];             // compute deviation of expected and output result
        if(tmp*tmp < TOL*TOL)        // correct if square deviation is less than tolerance squared
            correct++;
        else {
            printf(" tmp %f h_a %f h_b %f h_c %f \n",tmp, h_a[i], h_b[i], h_c[i]);
        }
    }
    
    // summarise results
    printf("C = A+B:  %d out of %d results were correct.\n", correct, count);
    
   
    free(h_a);
    free(h_b);
    free(h_c);
    
    return 0;
}