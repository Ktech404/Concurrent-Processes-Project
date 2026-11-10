#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "file_parser.h"
#include "race_condition.h"
#include "cv_mode.h"

int main()
{
    int choice;

    while (1) {
        printf("\n========== MODES/OPTIONG MENU ==========\n");
        printf("1. cv queue\n");
        printf("2. sem queue\n");
        printf("3. race condition\n");
        printf("4. run trace checker\n");
        printf("5. quit\n");
        printf("===============================\n");
        printf("choose an option/mode: ");

        // read the selection
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n') {
            }
            printf("Invalid choice. Please enter a number from 1 to 5.\n");
            continue;
        }

        //handles the selection
        switch (choice) {
            case 1: {
                Config config;

                printf("\nStarting CV Queue Mode...\n");

                if (!read_config("config/config_vals.txt", &config)) {
                    printf("Unable to start CV mode. Check configuration.\n");
                    break;
                }
                printf("Configuration successfully loaded.\n");

                // TODO cv queue stuff will go here eventually
                run_cv_mode(&config);
                break;
            }

            case 2: {
                Config config;

                printf("\nStarting SEM Queue Mode...\n");

                if (!read_config("config/config_vals.txt", &config)) {
                    printf("Unable to start SEM mode. Check configuration.\n");
                    break;
                }

                printf("Configuration successfully loaded.\n");

                // TODO !!sem queue implementation here
                printf("SEM mode is not implemented yet.\n");
                break;
            }

            case 3:
                printf("\nStarting Race Condition Mode...\n");
                run_race_condition();
                break;

            case 4:
                printf("\nStarting Trace Checker...\n");

                // TODO trace checker implementation will go here
                printf("Trace checker is not implemented yet.\n");
                break;

            case 5:
                printf("Quitting program...bye!\n");
                return 0;

            default:
                printf("Invalid selection. Please choose between 1 and 5.\n");
                break;
        }
    }
    return 0;
}
