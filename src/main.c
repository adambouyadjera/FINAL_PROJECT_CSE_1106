#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_CAPACITY 5
#define LED_PIN 17
#define LOG_FILE "data/occupancy_log.csv"
#define LOG_FILE_FROM_SRC "../data/occupancy_log.csv"
#define BUF_SIZE 64

typedef struct
{
    int count;
    int capacity;
} Room;

void setLed(int on)
{
    #ifdef ON_PI
    char cmd[BUF_SIZE];

    snprintf(cmd, sizeof(cmd), "pinctrl set %d op %s", LED_PIN, on ? "dh" : "dl");
    if (system(cmd) != 0)
    {
        printf("Warning: could not change the LED (is pinctrl installed?)\n");
    }
    #else
    static int lastState = -1;

    if (on != lastState)
    {
        printf("[simulated LED is now %s]\n", on ? "ON" : "OFF");
        lastState = on;
    }
    #endif
}

int isFull(const Room *room)
{
    return room->count >= room->capacity;
}

void updateLed(const Room *room)
{
    setLed(isFull(room));
}

const char *getStatus(const Room *room)
{
    return isFull(room) ? "ROOM FULL" : "SPACE AVAILABLE";
}

void showStatus(const Room *room)
{
    printf("\n%-20s %-18s %s\n", "Current Occupancy", "System Status", "Full Indicator LED");
    printf("%2d / %-15d %-18s %s\n\n",
           room->count, room->capacity, getStatus(room), isFull(room) ? "ON" : "OFF");
}

const char *logPath = LOG_FILE;

FILE *openLog(void)
{
    FILE *fp = fopen(LOG_FILE, "a");

    if (fp == NULL)
    {
        fp = fopen(LOG_FILE_FROM_SRC, "a");
        logPath = LOG_FILE_FROM_SRC;
    }
    else
    {
        logPath = LOG_FILE;
    }

    if (fp != NULL && ftell(fp) == 0)
    {
        fprintf(fp, "timestamp,event,occupancy,capacity,status\n");
    }

    return fp;
}

void logEvent(const Room *room, const char *event)
{
    FILE *fp = openLog();
    char timeStr[BUF_SIZE];
    time_t now = time(NULL);

    if (fp == NULL)
    {
        printf("Warning: could not open %s for writing\n", LOG_FILE);
        return;
    }

    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(fp, "%s,%s,%d,%d,%s\n",
            timeStr, event, room->count, room->capacity, getStatus(room));
    fclose(fp);
}

int readInt(const char *prompt, int *value)
{
    char buffer[BUF_SIZE];
    char *end;
    long num;

    printf("%s", prompt);
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
    {
        return -1;
    }

    num = strtol(buffer, &end, 10);
    if (end == buffer || (*end != '\n' && *end != '\0'))
    {
        return 0;
    }

    *value = (int)num;
    return 1;
}

void recordEntry(Room *room)
{
    if (isFull(room))
    {
        printf("Entry denied: the room is full (%d/%d). Someone must exit first.\n",
               room->count, room->capacity);
        logEvent(room, "ENTRY DENIED");
        return;
    }

    room->count++;
    printf("Person entered.\n");
    logEvent(room, "ENTER");
    updateLed(room);
    showStatus(room);
}

void recordExit(Room *room)
{
    if (room->count <= 0)
    {
        printf("Exit rejected: the room is already empty.\n");
        logEvent(room, "EXIT REJECTED");
        return;
    }

    room->count--;
    printf("Person exited.\n");
    logEvent(room, "EXIT");
    updateLed(room);
    showStatus(room);
}

void changeCapacity(Room *room)
{
    int newCapacity;
    int result = readInt("Enter new maximum capacity: ", &newCapacity);

    if (result != 1)
    {
        printf("Invalid number. Capacity unchanged.\n");
        return;
    }
    if (newCapacity < 1)
    {
        printf("Capacity must be at least 1. Capacity unchanged.\n");
        return;
    }
    if (newCapacity < room->count)
    {
        printf("There are %d people in the room. Capacity cannot be lower than that.\n",
               room->count);
        return;
    }

    room->capacity = newCapacity;
    printf("Maximum capacity changed to %d.\n", room->capacity);
    logEvent(room, "CAPACITY CHANGE");
    updateLed(room);
    showStatus(room);
}

int main(void)
{
    Room room = {0, DEFAULT_CAPACITY};
    int choice;
    int result;
    int running = 1;

    printf("=== Smart Study Room Occupancy Tracker ===\n");
    setLed(0);
    logEvent(&room, "SYSTEM START");
    showStatus(&room);

    while (running)
    {
        printf("1. Person enters\n");
        printf("2. Person exits\n");
        printf("3. Show status\n");
        printf("4. Change maximum capacity\n");
        printf("5. Quit\n");

        result = readInt("Choice: ", &choice);
        if (result == -1)
        {
            break;
        }
        if (result == 0)
        {
            printf("Please enter a number from 1 to 5.\n\n");
            continue;
        }

        switch (choice)
        {
            case 1:
                recordEntry(&room);
                break;
            case 2:
                recordExit(&room);
                break;
            case 3:
                showStatus(&room);
                break;
            case 4:
                changeCapacity(&room);
                break;
            case 5:
                running = 0;
                break;
            default:
                printf("Please enter a number from 1 to 5.\n\n");
        }
    }

    setLed(0);
    logEvent(&room, "SYSTEM STOP");
    printf("Goodbye. Events were saved to %s\n", logPath);
    return 0;
}