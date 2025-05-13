/*
 * Process Monitor - A lightweight Task Manager for Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <ncurses.h>

#define MAX_PROCESSES 1024
#define MAX_PATH 1024
#define MAX_LINE 256

typedef struct {
    int pid;
    char name[MAX_PATH];
    char state;
    char username[32];
    float cpu_usage;
    long memory_usage;
} Process;

Process process_list[MAX_PROCESSES];
int process_count = 0;

void get_processes();
void display_processes();
void get_process_info(int pid, Process *process);

int main() {
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);

    while (1) {
        clear();
        mvprintw(0, 0, "Process Monitor | Press 'q' to quit");
        get_processes();
        display_processes();
        refresh();
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        sleep(1);
    }

    endwin();
    return 0;
}

void get_processes() {
    DIR *dir;
    struct dirent *entry;
    process_count = 0;

    dir = opendir("/proc");
    if (!dir) {
        perror("/proc");
        exit(1);
    }

    while ((entry = readdir(dir)) != NULL && process_count < MAX_PROCESSES) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            int pid = atoi(entry->d_name);
            get_process_info(pid, &process_list[process_count]);
            if (process_list[process_count].memory_usage > 0) {
                process_count++;
            }
        }
    }
    closedir(dir);
}

void get_process_info(int pid, Process *process) {
    char path[MAX_PATH], line[MAX_LINE];
    FILE *file;

    process->pid = pid;
    strcpy(process->name, "unknown");
    process->state = '?';
    strcpy(process->username, "unknown");
    process->cpu_usage = 0;
    process->memory_usage = 0;

    // Get process name
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    file = fopen(path, "r");
    if (file) {
        if (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;
            strncpy(process->name, line, sizeof(process->name) - 1);
        }
        fclose(file);
    }

    // Get UID and memory from /proc/[pid]/status
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    file = fopen(path, "r");
    if (file) {
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "Uid:", 4) == 0) {
                int uid;
                sscanf(line, "Uid:\t%d", &uid);
                struct passwd *pw = getpwuid(uid);
                if (pw) strncpy(process->username, pw->pw_name, sizeof(process->username) - 1);
            }
            if (strncmp(line, "VmRSS:", 6) == 0) {
                long mem;
                sscanf(line, "VmRSS: %ld", &mem);
                process->memory_usage = mem; // Already in KB
            }
        }
        fclose(file);
    }
}

void display_processes() {
    mvprintw(1, 0, "%-6s %-20s %-10s %-8s", "PID", "NAME", "USER", "MEM(KB)");
    for (int i = 0; i < process_count && i < LINES - 3; i++) {
        mvprintw(i + 2, 0, "%-6d %-20s %-10s %-8ld",
                 process_list[i].pid,
                 process_list[i].name,
                 process_list[i].username,
                 process_list[i].memory_usage);
    }
}
