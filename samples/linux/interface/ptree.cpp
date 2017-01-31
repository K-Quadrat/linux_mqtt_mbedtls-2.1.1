#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <string>
#include <map>
#include <boost/foreach.hpp>

#include <vector>



/* system example : DIR */
#include <stdio.h>      /* printf */
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */



int main ()
{
    int i;
    printf ("Checking if processor is available...");
    if (system(NULL)) puts ("Ok");
    else exit (EXIT_FAILURE);
    printf ("Executing command DIR...\n");
    i=system ("ls");
    printf ("The value returned was: %d.\n",i);
    return 0;
}
