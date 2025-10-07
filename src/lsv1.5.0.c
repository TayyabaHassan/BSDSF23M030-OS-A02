#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>    // for strncasecmp
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

// ANSI color codes
#define COLOR_BLUE     "\033[0;34m"
#define COLOR_GREEN    "\033[0;32m"
#define COLOR_RED      "\033[0;31m"
#define COLOR_MAGENTA  "\033[0;35m"
#define COLOR_REVERSE  "\033[7m"
#define COLOR_RESET    "\033[0m"

// ---- Function Prototypes ----
void permissions_str(mode_t m, char *out);
void print_long(const char *fullpath, const char *name);
void print_default(char **names, int n, int maxlen, const char *basepath);
void print_horizontal(char **names, int n, int maxlen, const char *basepath);
int compare_names(const void *a, const void *b);
const char *color_for_file(const char *fullpath, const char *name);
void print_colored_padded(const char *basepath, const char *name, int pad_width);

// ---- Permission Helper ----
void permissions_str(mode_t m, char *out) {
    // out must have space for at least 11 chars
    strcpy(out, "----------");

    if (S_ISDIR(m)) out[0] = 'd';
    else if (S_ISLNK(m)) out[0] = 'l';
    else if (S_ISCHR(m)) out[0] = 'c';
    else if (S_ISBLK(m)) out[0] = 'b';
    else if (S_ISFIFO(m)) out[0] = 'p';
    else if (S_ISSOCK(m)) out[0] = 's';

    // user
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    if (m & S_ISUID) out[3] = (m & S_IXUSR) ? 's' : 'S';
    else out[3] = (m & S_IXUSR) ? 'x' : '-';

    // group
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    if (m & S_ISGID) out[6] = (m & S_IXGRP) ? 's' : 'S';
    else out[6] = (m & S_IXGRP) ? 'x' : '-';

    // others
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    if (m & S_ISVTX) out[9] = (m & S_IXOTH) ? 't' : 'T';
    else out[9] = (m & S_IXOTH) ? 'x' : '-';

    out[10] = '\0';
}

// ---- Determine color for a file based on type/extension ----
static int has_suffix(const char *name, const char *suf) {
    size_t n = strlen(name), m = strlen(suf);
    if (n < m) return 0;
    return strncasecmp(name + n - m, suf, m) == 0;
}

const char *color_for_file(const char *fullpath, const char *name) {
    struct stat st;
    if (lstat(fullpath, &st) < 0) {
        // if we can't stat, return default color
        return COLOR_RESET;
    }

    if (S_ISLNK(st.st_mode)) {
        return COLOR_MAGENTA; // symbolic links: pink/magenta
    }

    if (S_ISDIR(st.st_mode)) {
        return COLOR_BLUE; // directories: blue
    }

    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) ||
        S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode)) {
        return COLOR_REVERSE; // special files: reverse video
    }

    // archives / tarballs
    if (has_suffix(name, ".tar") || has_suffix(name, ".tar.gz") ||
        has_suffix(name, ".tgz") || has_suffix(name, ".gz") ||
        has_suffix(name, ".zip") || has_suffix(name, ".bz2") ||
        has_suffix(name, ".xz")) {
        return COLOR_RED; // tarballs/archives: red
    }

    // executables
    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        return COLOR_GREEN; // executable: green
    }

    return COLOR_RESET; // default
}

// ---- Helper to print a colored, padded filename using basepath for stat ----
void print_colored_padded(const char *basepath, const char *name, int pad_width) {
    char full[PATH_MAX];
    if (strcmp(basepath, ".") == 0)
        snprintf(full, sizeof(full), "%s", name);
    else
        snprintf(full, sizeof(full), "%s/%s", basepath, name);

    const char *col = color_for_file(full, name);
    // print color, then left-justified padded name, then reset
    printf("%s%-*s%s", col, pad_width, name, COLOR_RESET);
}

// ---- Long Listing ----
void print_long(const char *fullpath, const char *name) {
    struct stat st;
    if (lstat(fullpath, &st) < 0) {
        perror(fullpath);
        return;
    }

    char perm[12];
    permissions_str(st.st_mode, perm);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);

    char timebuf[64];
    struct tm *tm = localtime(&st.st_mtime);
    if (tm)
        strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm);
    else
        strncpy(timebuf, "??? ?? ????", sizeof(timebuf));

    // print metadata, then colored name
    printf("%s %3lu %-8s %-8s %8lld %s ",
           perm,
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "unknown",
           gr ? gr->gr_name : "unknown",
           (long long)st.st_size,
           timebuf);

    const char *col = color_for_file(fullpath, name);
    printf("%s%s%s", col, name, COLOR_RESET);

    // if symlink, also show "-> target"
    if (S_ISLNK(st.st_mode)) {
        char target[PATH_MAX];
        ssize_t tlen = readlink(fullpath, target, sizeof(target) - 1);
        if (tlen >= 0) {
            target[tlen] = '\0';
            printf(" -> %s", target);
        }
    }

    printf("\n");
}

// ---- Default Column Display (down then across) ----
void print_default(char **names, int n, int maxlen, const char *basepath) {
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
                print_colored_padded(basepath, names[i], col_width);
        }
        printf("\n");
    }
}

// ---- Horizontal (row-major) Display ----
void print_horizontal(char **names, int n, int maxlen, const char *basepath) {
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

        print_colored_padded(basepath, names[i], col_width);
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

    // determine base path
    const char *basepath = (optind < argc) ? argv[optind] : ".";
    DIR *dp = opendir(basepath);
    if (!dp) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    char *names[4096];   // increased capacity
    int n = 0, maxlen = 0;

    while ((entry = readdir(dp)) != NULL) {
        // skip hidden files (no -a implemented in this feature)
        if (entry->d_name[0] == '.') continue;

        if (n >= (int)(sizeof(names) / sizeof(names[0]))) {
            fprintf(stderr, "Too many entries\n");
            break;
        }

        names[n] = strdup(entry->d_name);
        if (!names[n]) { perror("strdup"); break; }

        int len = strlen(names[n]);
        if (len > maxlen) maxlen = len;
        n++;
    }
    closedir(dp);

    // sort alphabetically (Feature-5 behavior)
    if (n > 1)
        qsort(names, n, sizeof(char *), compare_names);

    // display
    if (display_mode == 1) {
        for (int i = 0; i < n; ++i) {
            char full[PATH_MAX];
            if (strcmp(basepath, ".") == 0) snprintf(full, sizeof(full), "%s", names[i]);
            else snprintf(full, sizeof(full), "%s/%s", basepath, names[i]);
            print_long(full, names[i]);
        }
    } else if (display_mode == 2) {
        print_horizontal(names, n, maxlen, basepath);
    } else {
        print_default(names, n, maxlen, basepath);
    }

    // free strings
    for (int i = 0; i < n; ++i) free(names[i]);

    return 0;
}
