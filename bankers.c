#include <stdio.h>

#include "bank.h"
#include "customer.h"

pthread_mutex_t resourceLock;

/* initializes the program by creating threads for each customer process, initializing the resourceLock mutex,
 * reading the available resources from the command line arguments, generating random values for the maximum and need
 * arrays, printing the initial state of the system, and joining the threads. The customer function is executed by each
 * thread and simulates the process of a customer requesting and releasing resources
 */
int main(int argc, char* argv[])
{
    pthread_t customers[NUMBER_OF_CUSTOMERS];
    int retCode;

    srand((int)time(NULL));

    if (argc != 4)
    {
        printf("Invalid argument count");
        return -1;
    }

    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        retCode = pthread_create(&customers[i], NULL, customer, (void*)i);

        if (retCode)
        {
            printf("ERROR; return code from pthread_create() is %d\n", retCode);
            exit(1);
        }
    }

    if (pthread_mutex_init(&resourceLock, NULL) != 0)
    {
        printf("ERROR: resourceLock mutex init failed\n");
        exit(1);
    }

    for (int i = 0; i < 3; i++)
        available[i] = atoi(argv[i+1]);

    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        for(int j = 0; j < NUMBER_OF_RESOURCES; j++)
        {
            maximum[i][j] = rand() % available[j];
            need[i][j] = maximum[i][j];
        }
    }

    state_print();

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        pthread_join(customers[i], NULL);
    }

    return 0;
}

/*
 * represents the behavior of a customer thread in the banker's algorithm simulation. Each customer thread continually
 * requests and releases resources in a loop.
 * */
void* customer(void* args)
{
    // Create an array to store the requested resources
    int resources[NUMBER_OF_RESOURCES];

    // Get the customer number from the function argument
    int customerNumber = (long)args;

    // Loop indefinitely
    while(true)
    {
        // Print the customer's request
        printf("Request P%d <", customerNumber);

        // For each resource type
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        {
            // If the customer still needs this resource
            if (need[customerNumber][i] != 0)
                // Generate a random number between 1 and the maximum needed
                resources[i] = rand() % need[customerNumber][i] + 1;
            else
                // Otherwise, request 0 of this resource
                resources[i] = 0;

            // Print the requested amount of this resource
            printf("%d", resources[i]);

            // If this is not the last resource type, print a comma separator
            if (i != NUMBER_OF_RESOURCES - 1)
                printf(", ");
        }
        printf(">\n");

        // Attempt to acquire the resourceLock
        bool granted = true;
        pthread_mutex_lock(&resourceLock);

        // If the requested resources can be granted safely
        if (request_resources(customerNumber, resources) == 0)
        {
            // Set the granted flag to true
            printf("Safe, request granted\n");
            // Print the current state of the system
            state_print();
        }
        else
            // Otherwise, the requested resources cannot be granted safely
            printf("Unsafe, request denied\n");

        // Release the resourceLock
        pthread_mutex_unlock(&resourceLock);

        // If the resources were granted
        if (granted)
        {
            // Print the release request
            printf("Release P%d <", customerNumber);

            // For each resource type
            for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
            {
                // If the customer has any of this resource
                if (allocation[customerNumber][i] != 0)
                    // Generate a random number between 1 and the allocated amount
                    resources[i] = rand() % allocation[customerNumber][i] + 1;
                else
                    // Otherwise, release 0 of this resource
                    resources[i] = 0;

                // Print the released amount of this resource
                printf("%d", resources[i]);

                // If this is not the last resource type, print a comma separator
                if (i != NUMBER_OF_RESOURCES - 1)
                    printf(", ");
            }
            printf(">\n");

            // Attempt to acquire the resourceLock
            pthread_mutex_lock(&resourceLock);
            // If the requested resources can be safely released
            if(release_resources(customerNumber, resources) == 0)
            {
                printf("Safe, release granted\n");
                // Print the current state of the system
                state_print();
            }
            else
                // Otherwise, the resources cannot be released safely
                printf("Unsafe, release denied\n");

            // Release the resourceLock
            pthread_mutex_unlock(&resourceLock);
        }

        // Sleep for 3 seconds before making the next request
        sleep(3);
    }
}

/*
 * checks if the request can be satisfied by checking if the available resources are greater than or equal to the
 * requested resources. If the request can be satisfied, it checks if granting the request would result in a safe state
 * by calling the safety_test function. If the safety test is successful, the request is granted, and the allocation,
 * need, and available arrays are updated accordingly.
 */
int request_resources(int customer_num, int request[])
{
    // check if the requested resources exceed the available resources
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        if(request[i] > available)
            return -1;
    }

    // check if the request would result in an unsafe state
    if(safety_test(customer_num, request) == -1)
        return -1;

    // allocate the requested resources
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
        available[i] -= request[i];
    }

    return 0;
}

/*
 * Creates temporary arrays to simulate the state of the system after granting the request. It then checks if the new
 * state is safe by simulating the process of executing the banker's algorithm to check if there is a sequence of
 * processes that can complete without causing a deadlock. If the new state is safe, the function returns 0; otherwise,
 * it returns -1.
 * */
int release_resources(int customer_num, int release[])
{
    // For each resource, update the allocation, available, and need arrays.
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        allocation[customer_num][i] -= release[i];
        available[i] += release[i];
        need[customer_num][i] += release[i];
    }

    // Return 0 to indicate that the release was successful.
    return 0;
}

/*
 * prints the current state of the system, including the allocation, need, and available arrays.
 * */
void state_print()
{
    printf("\tAllocation\tNeed\t\tAvailable\n\tA  B  C  \tA  B  C  \tA  B  C\n");

    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        printf("P%d\t", i);

        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
            printf("%-3d", allocation[i][j]);

        printf("\t");

        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
            printf("%-3d", need[i][j]);

        if(i == 0)
        {
            printf("\t");

            for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                printf("%-3d", allocation[i][j]);
        }
        printf("\n");
    }
}

/*
 * checks if a request made by a customer can be granted while ensuring the system remains in a safe state. The function
 * performs a simulation of the system by temporarily allocating the requested resources to the customer and checking if
 * this allocation can lead to a safe state or not
 * */
int safety_test(int customer_num, int request[])
{
    int temp_available[NUMBER_OF_RESOURCES];
    int temp_need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
    int temp_allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

    // Calculate the temporary arrays by subtracting the requested resources from the available resources
    // and updating the allocation matrix for the given customer
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
    {
        temp_available[i] = available[i] - request[i];
        temp_allocation[customer_num][i] = request[i];
    }

    // Copy the original need and allocation matrices to the temporary ones
    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
    {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
        {
            temp_allocation[i][j] = allocation[i][j];
            temp_need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }

    int num_finished = 0;

    // Loop until all customers have been satisfied or deadlock has been detected
    while (num_finished < NUMBER_OF_CUSTOMERS)
    {
        int num_satisfied = 0;
        // Check each customer to see if their resource needs can be met
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        {
            if (temp_allocation[i][0] != -1)
            {  // check if customer is not finished
                int is_safe = 1;
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                {
                    if (temp_need[i][j] > temp_available[j])
                    { // check if customer's resource needs can be met
                        is_safe = 0;
                        break;
                    }
                }

                // If the customer's resource needs can be met, update the available resources and mark the customer as finished
                if (is_safe)
                {
                    for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                    {
                        temp_available[j] += temp_allocation[i][j];
                        temp_allocation[i][j] = -1;  // mark customer as finished
                    }
                    num_satisfied++;
                    num_finished++;
                }
            }
        }
        // If no customers were satisfied in the current iteration, deadlock has been detected
        if (num_satisfied == 0)
            // deadlock detected
            return -1;
    }
    // safe to grant request
    return 0;
}
