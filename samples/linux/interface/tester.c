#include <string.h>
#include <stdlib.h>
#include <stdio.h>



int main(void) {

    char thisIn [100];
    sprintf(thisIn, "%s", in);

    char delimiter[] = ",;";
    char *ptr;

// initialisieren und ersten Abschnitt erstellen
    ptr = strtok(thisIn, delimiter);

    while(ptr != NULL) {
        printf("Abschnitt gefunden: %s\n", ptr);
        // naechsten Abschnitt erstellen
        ptr = strtok(NULL, delimiter);
    }




    FILE *fp;
    char buffer[10000];

    char command[] = "cat";
    char blank[] = " ";
    char options[] = "/proc/cpuinfo";
    char commandRun[80];

    strcpy(commandRun, command);
    strcat(commandRun, blank);
    strcat(commandRun, options);


    /* Open the command for reading. */
    fp = popen(commandRun, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }

    /* Read the output a line at a time - output it. */
    while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
        printf("%s", buffer);
    }

    pclose(fp); // close


    return 0;
}