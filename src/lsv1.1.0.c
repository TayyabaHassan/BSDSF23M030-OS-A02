#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>   // for mode_t, DIR
#include <unistd.h>      // must come before sys/stat.h for lstat()
#include <sys/stat.h>    // for stat, lstat
#include <pwd.h>         // for getpwuid
#include <grp.h>         // for getgrgid
#include <time.h>        // for strftime
#include <errno.h>       // for perror

// ------------------------------------------------------------
// Function to generate permission string like "drwxr-xr-x"
// ------------------------------------------------------------
void permissions_str(mode_t m, char *out) {
    strcpy(out, "----------");
    if (S_ISDIR(m)) out[0] = 'd';
    else if (S_ISLNK(m)) out[0] = 'l';
    if (m & S_IRUSR) out[1] = 'r';
    if (m & S_IWUSR) out[2] = 'w';
    if (m & S_IXUSR) out[3] = 'x';
    if (m & S_IRGRP) out[4] = 'r';
    if (m & S_IWGRP) out[5] = 'w';
    if (m & S_IXGRP) out[6] = 'x';
    if (m & S_IROTH) out[7] = 'r';
    if (m & S_IWOTH) out[8] = 'w';
    if (m & S_IXOTH) out[9] = 'x';
    out[10] = '\0';
}

// ------------------------------------------------------------
// Print details of one file in long-listing (-l) format
// ------------------------------------------------------------
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

    char timebuf[64];
    struct tm *tm = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm);

    printf("%s %2lu %s %s %6lld %s %s\n",
           perm,
           (unsigned long)st.st_nlink,
           pw ? pw->pw_name : "?",
           gr ? gr->gr_name : "?",
           (long long)st.st_size,
           timebuf,
           name);
}

// ------------------------------------------------------------
// Main program
// ------------------------------------------------------------
int main(int argc, char *argv[]) {
    int long_flag = 0;
    const char *target_dir = "."; // default current directory

    // parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            long_flag = 1;
        } else {
            target_dir = argv[i];
        }
    }

    DIR *dir = opendir(target_dir);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (long_flag) {
            // build full path
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", target_dir, entry->d_name);
            print_long(path);
        } else {
            printf("%s\n", entry->d_name);
        }
    }

    closedir(dir);
    return 0;
}

