#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

// ---- Function Prototypes ----
void print_long(const char *name);
void print_horizontal(char **names, int n, int maxlen);
void print_default(char **names, int n, int maxlen);
void permissions_str(mode_t m, char *out);
int compare_names(const void *a, const void *b);

// ---- Permission Helper ----
void permissions_str(mode_t m, char *out) {
    strcpy(out, "----------");

    if (S_ISDIR(m)) out[0] = 'd';
    else if (S_ISLNK(m)) out[0] = 'l';
    else if (S_ISCHR(m)) out[0] = 'c';
    else if (S_ISBLK(m)) out[0] = 'b';

    if (m & S_IRUSR) out[1] = 'r';
    if (m & S_IWUSR) out[2] = 'w';
    if (m & S_IXUSR) out[3] = 'x';
    if (m & S_IRGRP) out[4] = 'r';
    if (m & S_IWGRP) out[5] = 'w';
    if (m & S_IXGRP) out[6] = 'x';
    if (m & S_IROTH) out[7] = 'r';
    if (m & S_IWOTH) out[8] = 'w';
    if (m & S_IXOTH) out[9] = 'x';
}

// ---- Long Listing ----
void print_long(const char *name) {
    struct stat st;
    if (lstat(name, &st) < 0) {
        perror(name);
        return;
    }

    char perm[11];
    permissions_str(st.st_mode, perm);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);
    char *time_str = ctime(&st.st_mtime);
    time_str[strlen(time_str) - 1] = '\0';  // remove newline

    printf("%s %3ld %s %s %8ld %s %s\n",
           perm,
           (long) st.st_nlink,
           pw ? pw->pw_name : "unknown",
           gr ? gr->gr_name : "unknown",
           (long) st.st_size,
           time_str,
           name);
}

// ---- Default Column Display (down then across) ----
void print_default(char **names, int n, int maxlen) {
    struct winsize ws;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        term_width = ws.ws_col;

    int col_width = maxlen + 2;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows = (n + cols - 1) / cols;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int i = c * rows + r;
            if (i < n)
                printf("%-*s", col_width, names[i]);
        }
        printf("\n");
    }
}

// ---- Horizontal (row-major) Display ----
void print_horizontal(char **names, int n, int maxlen) {
    struct winsize ws;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        term_width = ws.ws_col;

    int col_width = maxlen + 2;
    int current_width = 0;

    for (int i = 0; i < n; ++i) {
        if (current_width + col_width > term_width) {
            printf("\n");
            current_width = 0;
        }

        printf("%-*s", col_width, names[i]);
        current_width += col_width;
    }
    printf("\n");
}

// ---- Comparison function for qsort ----
int compare_names(const void *a, const void *b) {
    const char *s1 = *(const char **)a;
    const char *s2 = *(const char **)b;
    return strcmp(s1, s2);
}

// ---- Main ----
int main(int argc, char *argv[]) {
    int display_mode = 0; // 0 = default, 1 = long (-l), 2 = horizontal (-x)
    int opt;

    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l': display_mode = 1; break;
            case 'x': display_mode = 2; break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [dir]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // open current directory or argument
    const char *path = (optind < argc) ? argv[optind] : ".";
    DIR *dp = opendir(path);
    if (!dp) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    char *names[1024];
    int n = 0, maxlen = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue; // skip hidden files
        names[n] = strdup(entry->d_name);
        int len = strlen(names[n]);
        if (len > maxlen) maxlen = len;
        n++;
    }
    closedir(dp);

    // ---- Alphabetical Sort ----
    qsort(names, n, sizeof(char *), compare_names);

    // ---- Display according to mode ----
    if (display_mode == 1) {
        for (int i = 0; i < n; i++)
            print_long(names[i]);
    } else if (display_mode == 2) {
        print_horizontal(names, n, maxlen);
    } else {
        print_default(names, n, maxlen);
    }

    // ---- Free memory ----
    for (int i = 0; i < n; i++)
        free(names[i]);

    return 0;
}
