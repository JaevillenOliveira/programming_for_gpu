#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <CL/cl.h>

//#define LENGTH (1024*1024*10)    // length of vectors a, b, and c

const char *KernelSource = "\n" \
"__kernel void pythgrsTrplts(                                            \n" \
"   __global long* a,                                                    \n" \
"   __global long* res_a_b,                                              \n" \
"   const unsigned long b_lim,                                           \n" \
"   const unsigned long c_lim,                                           \n" \
"   const unsigned long count)                                           \n" \
"{                                                                       \n" \
"   long i = get_global_id(0);                                           \n" \
"   if(i < count){                                                       \n" \
"       long res_idx = i * (b_lim / 2);                                  \n" \
"       res_a_b[res_idx++] = a[i];                                       \n" \
"       for (long b = a[i] + 1; b <= b_lim; b++){                        \n" \
"           for (long c = b + 1; c <= c_lim; c++){                       \n" \
"               if(a[i]*a[i] + b*b == c*c){                              \n" \
"                   res_a_b[res_idx++] = b;                              \n" \
"                   break;                                               \n" \
"               }                                                        \n" \
"           }                                                            \n" \
"       }                                                                \n" \
"   }                                                                    \n" \
"}                                                                       \n" \
"\n";

//------------------------------------------------------------------------------

void checkError(int code, char message[])
{
    if (code != CL_SUCCESS){
        printf("Error %s\n", message);
        exit(-1);
    }
}

int main(int argc, char** argv)
{   
    long PERIMETER = 100000;
    int MAXDEVICES = 1;
    long count;
    int err;               // error code returned from OpenCL calls
    long i;

    long   var_lim_a   = (PERIMETER/3) - 1;    // max value possible for 'a'
    long   var_lim_b   = (PERIMETER/3);        // max value possible for 'b'
    long   var_lim_c   = var_lim_b + (PERIMETER - var_lim_b * 3) + 1;  // max value possible for 'c'
    long   num_poss_comb = var_lim_a * (var_lim_b / 2) ;
    long*  h_a           = (long*) calloc(var_lim_a,   sizeof(long));    // the a values to check
    long*  h_res_a_b     = (long*) calloc(num_poss_comb, sizeof(long));    // result vector to store the 'b' of the triplet
    count = var_lim_a;

    clock_t start, end;
    double time_taken;					// current time
    
    size_t global;                  // global domain size
    
    cl_device_id 	device_id;		  // device id
    cl_context      context;         // compute context
    cl_command_queue commands;      // compute command queue
    cl_program      program;       // compute program
    cl_kernel       pythgrsTrplts; // compute kernel
    
    cl_mem d_a;                     // device memory used for the input  a vector                   
    cl_mem d_res_a_b;                                                          
    
    // Fill vector a 
    i = 0;
    for(i = 0; i < count; i++){
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
            
            printf("Error: Failed to build program executable!");
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
            printf("%s\n", buffer);
            return EXIT_FAILURE;
        }
        
        // Create the input (a, b) and output (c) arrays in device memory
        d_a  = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(long) * count, NULL, &err);
        checkError(err, "Creating buffer d_a");
        
        // d_var_lim_b  = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(long), NULL, &err);
        // checkError(err, "Creating buffer d_var_lim_b");
        
        // d_var_lim_c  = clCreateBuffer(context,  CL_MEM_READ_ONLY, sizeof(long), NULL, &err);
        // checkError(err, "Creating buffer d_var_lim_c");

        d_res_a_b  = clCreateBuffer(context,  CL_MEM_WRITE_ONLY,  sizeof(long) * num_poss_comb, NULL, &err);
        checkError(err, "Creating buffer d_res_a_b");
        
        // Write a and b vectors into compute device memory
        err = clEnqueueWriteBuffer(commands, d_a, CL_TRUE, 0, sizeof(long) * count, h_a, 0, NULL, NULL);
        checkError(err, "Copying h_a to device at d_a");
        
        // err = clEnqueueWriteBuffer(commands, d_var_lim_b, CL_TRUE, 0, sizeof(long), h_var_lim_b, 0, NULL, NULL);
        // checkError(err, "Copying h_var_lim_b to device at d_var_lim_b");

        // err = clEnqueueWriteBuffer(commands, d_var_lim_c, CL_TRUE, 0, sizeof(long), h_var_lim_c, 0, NULL, NULL);
        // checkError(err, "Copying h_var_lim_c to device at d_var_lim_c");
        
        // Create the compute kernel from the program
        pythgrsTrplts = clCreateKernel(program, "pythgrsTrplts", &err);
        checkError(err, "Creating kernel");

        // Set the arguments to our compute kernel
        err  = clSetKernelArg(pythgrsTrplts, 0, sizeof(cl_mem), &d_a);
        err |= clSetKernelArg(pythgrsTrplts, 1, sizeof(cl_mem), &d_res_a_b);
        err |= clSetKernelArg(pythgrsTrplts, 2, sizeof(unsigned long), &var_lim_b);
        err |= clSetKernelArg(pythgrsTrplts, 3, sizeof(unsigned long), &var_lim_c);
        err |= clSetKernelArg(pythgrsTrplts, 4, sizeof(unsigned long), &count);
        checkError(err, "Setting kernel arguments");
        
        start = clock();
        
        // Execute the kernel over the entire range of our 1d input data set
        // letting the OpenCL runtime choose the work-group size
        global = count;
        err = clEnqueueNDRangeKernel(commands, pythgrsTrplts, 1, NULL, &global, NULL, 0, NULL, NULL);
        checkError(err, "Enqueueing kernel");
        
        // Wait for the commands to complete before stopping the timer
        err = clFinish(commands);
        checkError(err, "Waiting for kernel to finish");
        
        end = clock();
        time_taken = ((double)end - start)/CLOCKS_PER_SEC;

        printf("\nThe kernel ran in %lf seconds\n",time_taken);
        printf("\n-------------------------\n");
        
        // Read back the results from the compute device
        err = clEnqueueReadBuffer( commands, d_res_a_b, CL_TRUE, 0, sizeof(long) * num_poss_comb, h_res_a_b, 0, NULL, NULL );
        
        if (err != CL_SUCCESS)
        {
            printf("Error: Failed to read output array!\n");
            exit(1);
        }
        // cleanup then shutdown
        clReleaseMemObject(d_a);
        clReleaseMemObject(d_res_a_b);
        clReleaseProgram(program);
        clReleaseKernel(pythgrsTrplts);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);

        FILE *fptr;

        fptr = fopen("pythagoreanTriplets.txt","w");
        fptr = freopen("pythagoreanTriplets.txt","a", fptr);

        long a, b, c, j;
        for(i = 0; i < num_poss_comb; i += var_lim_b/2){
            a = h_res_a_b[i];
            for(j = i + 1; j < (i + var_lim_b/2); j++){
                if(h_res_a_b[j] != 0){
                    b = h_res_a_b[j];
                    c = sqrt(a*a + b*b);
                    fprintf(fptr, "Triplet (a,b,c) = %ld %ld %ld\n", a, b, c);
                }
            }
        }

        fclose(fptr);


    }
    
   
    free(h_a);
    free(h_res_a_b);
    
    return 0;
}