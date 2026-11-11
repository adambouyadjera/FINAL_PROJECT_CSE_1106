#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_CAPACITY 5
#define LOG_FILE "data/occupancy_log.csv"
#define LOG_FILE_FROM_SRC "../data/occupancy_log.csv"

int count = 0;
int capacity = DEFAULT_CAPACITY;

void setLed(int on)
{
    int result;

    if (on)
    {
        result = system("pinctrl set 17 op dh 2>/dev/null");
        if (result != 0)
        {
            result = system("raspi-gpio set 17 op dh");
        }
    }
    else
    {
        result = system("pinctrl set 17 op dl 2>/dev/null");
        if (result != 0)
        {
            result = system("raspi-gpio set 17 op dl");
        }
    }

    if (result != 0)
    {
        printf("Warning: could not change the LED\n");
    }
}

int isFull(void)
{
    return count >= capacity;
}

void updateLed(void)
{
    setLed(isFull());
}

void showStatus(void)
{
    printf("\n%-20s %-18s %s\n", "Current Occupancy", "System Status", "Full Indicator LED");

    if (isFull())
    {
        printf("%2d / %-15d %-18s %s\n\n", count, capacity, "ROOM FULL", "ON");
    }
    else
    {
        printf("%2d / %-15d %-18s %s\n\n", count, capacity, "SPACE AVAILABLE", "OFF");
    }
}

void logEvent(char event[])
{
    FILE *fp;
    char timeStr[64];
    time_t now = time(NULL);

    fp = fopen(LOG_FILE, "a");
    if (fp == NULL)
    {
        fp = fopen(LOG_FILE_FROM_SRC, "a");
    }
    if (fp == NULL)
    {
        printf("Warning: could not open %s for writing\n", LOG_FILE);
        return;
    }

    if (ftell(fp) == 0)
    {
        fprintf(fp, "timestamp,event,occupancy,capacity,status\n");
    }

    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));

    if (isFull())
    {
        fprintf(fp, "%s,%s,%d,%d,ROOM FULL\n", timeStr, event, count, capacity);
    }
    else
    {
        fprintf(fp, "%s,%s,%d,%d,SPACE AVAILABLE\n", timeStr, event, count, capacity);
    }

    fclose(fp);
}

void clearInput(void)
{
    int ch = getchar();

    while (ch != '\n' && ch != EOF)
    {
        ch = getchar();
    }
}

void recordEntry(void)
{
    if (isFull())
    {
        printf("Entry denied: the room is full (%d/%d). Someone must exit first.\n\n",
               count, capacity);
        logEvent("ENTRY DENIED");
        return;
    }

    count++;
    printf("Person entered.\n");
    logEvent("ENTER");
    updateLed();
    showStatus();
}

void recordExit(void)
{
    if (count <= 0)
    {
        printf("Exit rejected: the room is already empty.\n\n");
        logEvent("EXIT REJECTED");
        return;
    }

    count--;
    printf("Person exited.\n");
    logEvent("EXIT");
    updateLed();
    showStatus();
}

void changeCapacity(void)
{
    int newCapacity;

    printf("Enter new maximum capacity: ");
    if (scanf("%d", &newCapacity) != 1)
    {
        clearInput();
        printf("Invalid number. Capacity unchanged.\n\n");
        return;
    }
    clearInput();

    if (newCapacity < 1)
    {
        printf("Capacity must be at least 1. Capacity unchanged.\n\n");
        return;
    }
    if (newCapacity < count)
    {
        printf("There are %d people in the room. Capacity cannot be lower than that.\n\n",
               count);
        return;
    }

    capacity = newCapacity;
    printf("Maximum capacity changed to %d.\n", capacity);
    logEvent("CAPACITY CHANGE");
    updateLed();
    showStatus();
}

int main(void)
{
    int choice;
    int running = 1;

    printf("=== Smart Study Room Occupancy Tracker ===\n");
    setLed(0);
    logEvent("SYSTEM START");
    showStatus();

    while (running)
    {
        printf("1. Person enters\n");
        printf("2. Person exits\n");
        printf("3. Show status\n");
        printf("4. Change maximum capacity\n");
        printf("5. Quit\n");
        printf("Choice: ");

        if (scanf("%d", &choice) != 1)
        {
            if (feof(stdin))
            {
                break;
            }
            clearInput();
            printf("Please enter a number from 1 to 5.\n\n");
            continue;
        }
        clearInput();

        switch (choice)
        {
            case 1:
                recordEntry();
                break;
            case 2:
                recordExit();
                break;
            case 3:
                showStatus();
                break;
            case 4:
                changeCapacity();
                break;
            case 5:
                running = 0;
                break;
            default:
                printf("Please enter a number from 1 to 5.\n\n");
        }
    }

    setLed(0);
    logEvent("SYSTEM STOP");
    printf("Goodbye. Events were saved to the data folder.\n");
    return 0;
}