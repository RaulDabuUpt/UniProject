#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <time.h>
#include <wait.h>

void print_error_message(char* message) {
    fprintf(stderr, "Error: %s\n", message);
}

void execute_script(const char* filename) {
    char *script = "./compile.sh";
    int pipefd[2];
    int status;
    pid_t child_pid2;

    // Create a pipe for communication between child and parent
    if (pipe(pipefd) == -1) {
        print_error_message("pipe");
        return;
    }

    // Fork the process
    child_pid2 = fork();

    if (child_pid2 == -1) {
        print_error_message("fork");
        return;
    }

    if (child_pid2 == 0) {
        // Child process
        // Redirect the standard output to the write end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // Execute the compile_stats.sh script
        execlp(script, script, filename, NULL);

        // If execlp() returns, an error occurred
        print_error_message("execlp");
        exit(1);
    } else {
        // Parent process
        close(pipefd[1]);
        char buffer[1024];
        int num_bytes = read(pipefd[0], buffer, sizeof(buffer));
        buffer[num_bytes] = '\0';

        // Wait for the child process to finish
        wait(&status);

        int score;
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            int errors, warnings;
            sscanf(buffer, "Errors: %d\nWarnings: %d", &errors, &warnings);

            if (errors == 0 && warnings == 0) {
                score = 10;
            } else if (errors > 0) {
                score = 1;
            } else if (warnings > 10) {
                score = 2;
            } else {
                score = 2 + 8 * (10 - warnings) / 10;
            }
        } else {
            score = -1; // Compilation failed
        }

        // Write the score to grades.txt
        FILE *file = fopen("grades.txt", "a");
        if (file == NULL) {
            print_error_message("fopen");
            return;
        }

        fprintf(file, "%s: %d\n", filename, score);
        fclose(file);
    }
}

void print_access_rights(mode_t mode) {
    printf("User:\n");
    printf("Read - %s\n", (mode & S_IRUSR) ? "yes" : "no");
    printf("Write - %s\n", (mode & S_IWUSR) ? "yes" : "no");
    printf("Exec - %s\n", (mode & S_IXUSR) ? "yes" : "no");
    printf("Group:\n");
    printf("Read - %s\n", (mode & S_IRGRP) ? "yes" : "no");
    printf("Write - %s\n", (mode & S_IWGRP) ? "yes" : "no");
    printf("Exec - %s\n", (mode & S_IXGRP) ? "yes" : "no");
    printf("Others:\n");
    printf("Read - %s\n", (mode & S_IROTH) ? "yes" : "no");
    printf("Write - %s\n", (mode & S_IWOTH) ? "yes" : "no");
    printf("Exec - %s\n", (mode & S_IXOTH) ? "yes" : "no");
}

void create_symbolic_link(char* path) {
    char link_name[100];
    printf("Enter a name for the symbolic link: ");
    if (fgets(link_name, sizeof(link_name), stdin) == NULL) {
        print_error_message("Failed to read input\n");
        return;
    }
    // Remove newline character from link_name if present
    link_name[strcspn(link_name, "\n")] = '\0';
    if (symlink(path, link_name) == -1) {
        print_error_message("Failed to create symbolic link\n");
        return;
    }
    printf("Symbolic link created successfully\n");
}

void execute_regular_file_option(char option, char* path) {
	
	struct stat sb;
    if (stat(path, &sb) == -1) {
        print_error_message("Failed to get file information");
    }
    switch (option) {
    case 'n' : printf("Name (-n): %s\n", path);
    break;
	case 'd' : printf("Size (-d): %ld bytes\n", sb.st_size);
    break;
	case 'h' : printf("Hard link count (-h): %ld\n", sb.st_nlink);
    break;
	case 'm' : printf("Time of last modification (-m): %s", ctime(&sb.st_mtime));
    break;
	case 'a' : printf("Access rights (-a):\n");
		print_access_rights(sb.st_mode);
	break;
	case 'l' : printf("Create symbolic link (-l): ");
	    create_symbolic_link(path);
	break;
	default: break;   
    }
}

void execute_directory_option(char option, char* path) {
    struct dirent *dp;
    struct stat sb;
    DIR *dir = opendir(path);
    int c_count = 0;

    if (!dir) {
        print_error_message("Couldn't open directory.");
        return;
    }

    while ((dp = readdir(dir)) != NULL) {
        char file_path[PATH_MAX];
        snprintf(file_path, PATH_MAX, "%s/%s", path, dp->d_name);

        if (lstat(file_path, &sb) == -1) {
            print_error_message("Couldn't get file statistics for file in directory.");
            continue;
        }

        switch (option) {
            case 'n':
                printf("%s\n", dp->d_name);
                break;

            case 'd':
                printf("%ld\n", sb.st_size);
                break;

            case 'a':
                print_access_rights(sb.st_mode);
                break;

            case 'c':
                if (S_ISREG(sb.st_mode) && strstr(dp->d_name, ".c") != NULL) {
                    c_count++;
                }
                break;

            default:
                printf("Invalid option: %c\n", option);
                break;
        }
    }

    if (option == 'c') {
        printf("Total .c files: %d\n", c_count);
    }

    closedir(dir);
}

void execute_symbolic_link_option(char option, char* path) {
    struct stat sb;
    char target_path[PATH_MAX];
    ssize_t target_size;
    int delete_flag = 0;

    if (lstat(path, &sb) == -1) {
        print_error_message("lstat");
        return;
    }

    if (S_ISLNK(sb.st_mode)) {
        // Get target path and size if symbolic link
        target_size = readlink(path, target_path, PATH_MAX - 1);
        if (target_size == -1) {
            print_error_message("readlink");
            return;
        }
        target_path[target_size] = '\0';
    }

    switch (option) {
        case 'n':
			//Name of file
            printf("%s\n", path);
            break;

        case 'l':
            // Delete symbolic link
            if (unlink(path) == -1) {
                print_error_message("unlink");
            }
            delete_flag = 1;
            break;

        case 'd':
            // Size of symbolic link
            printf("%ld\n", target_size);
            break;

        case 't':
            // Size of target file
            if (!S_ISLNK(sb.st_mode)) {
                printf("Not a symbolic link\n");
                return;
            }
            if (lstat(target_path, &sb) == -1) {
                print_error_message("lstat");
                return;
            }
            printf("%ld\n", sb.st_size);
            break;

        case 'a':
            // Access rights
            print_access_rights(sb.st_mode);
            break;

        default:
            printf("Invalid option: %c\n", option);
            break;
    }

    if (!delete_flag && option != '\0') {
        printf("Additional options not supported\n");
    }
}

void display_regular_file_menu(char* path) {
    printf("Options:\n");
    printf("-n: display name of file\n");
    printf("-h: display hard link count\n");
    printf("-d: display file size\n");
    printf("-m: display time of last modification\n");
    printf("-a: display access rights\n");
    printf("-l: create symbolic link\n");
    printf("Enter options as a single string (e.g., -nhd): ");
    char options[10];
    fgets(options, sizeof(options), stdin);
    options[strcspn(options, "\n")] = '\0'; // remove newline character
    int i;
    int check = 1;
    char option;
	//check '-' is first character here.
    if(options[0]!= '-') check = 0;
    for(i = 1; i <strlen(options); i++) {
        option = options[i];
        if(!(strchr("nhdmal", option))){
            check = 0;
            break;
        }
    }
    
    if(check == 1){
    for (i = 1; i < strlen(options); i++) {
        option = options[i];
        execute_regular_file_option(option, path);
    }
     
     }else {
            print_error_message("Invalid option");
            display_regular_file_menu(path);
            return;
        }
    }

void display_directory_menu(char* path) {
    printf("Options:\n");
    printf("-n: display name of file\n");
    printf("-d: display file size\n");
    printf("-a: display access rights\n");
    printf("-c: number of files with .c extension\n");
    printf("Enter options as a single string (e.g., -nhd): ");
    char options[10];
    fgets(options, sizeof(options), stdin);
    options[strcspn(options, "\n")] = '\0'; // remove newline character
    int i;
    int check = 1;
    char option;
    if(options[0]!= '-') check = 0;
    for(i = 1; i <strlen(options); i++) {
        option = options[i];
        if(!(strchr("ndac", option))){
            check = 0;
            break;
        }
    }
    
    if(check == 1){
    for (i = 1; i < strlen(options); i++) {
        option = options[i];
        execute_directory_option(option, path);
    }
     
     }else {
            print_error_message("Invalid option");
            display_directory_menu(path);
            return;
        }
    }

void display_symbolic_link_menu(char* path) {
    printf("Options:\n");
    printf("-n: display name of file\n");
    printf("-d: display symbolic link size\n");
    printf("-a: display access rights\n");
    printf("-t: display size of target file\n");
	printf("-l: delete symbolic link\n");
    printf("Enter options as a single string (e.g., -nhd): ");
    char options[10];
    fgets(options, sizeof(options), stdin);
    options[strcspn(options, "\n")] = '\0'; // remove newline character
    int i;
    int check = 1;
    char option;
    if(options[0]!= '-') check = 0;
    for(i = 1; i <strlen(options); i++) {
        option = options[i];
        if(!(strchr("nldta", option))){
            check = 0;
            break;
        }
    }
    
    if(check == 1){
    for (i = 1; i < strlen(options); i++) {
        option = options[i];
        execute_symbolic_link_option(option, path);
    }
     
     }else {
            print_error_message("Invalid option");
            display_symbolic_link_menu(path);
            return;
        }
    }

void display_file_info(char* path) {
    struct stat sb;
    if (stat(path, &sb) == -1) {
        print_error_message("Failed to get file information");
        return;
    }
    int status;
    switch (sb.st_mode & S_IFMT) {
        case S_IFREG: // regular file
            printf("File type: regular file\n");
			display_regular_file_menu(path);
            pid_t child_pid2 = fork();
            if (child_pid2 < 0) {
                print_error_message("Failed to create child process.\n");
                return;
            } else if (child_pid2 == 0) {
                // Child process
				if (strstr(path, ".c") != NULL) {
					execute_script(path);
					exit(0);
				} else {
					// Count number of lines
					FILE* file = fopen(path, "r");
					if (file == NULL) {
						print_error_message("Failed to open the file.\n");
						return;
					}

					int lineCount = 0;
					char ch;
					while ((ch = fgetc(file)) != EOF) {
						if (ch == '\n') {
							lineCount++;
						}
					}

					fclose(file);

					printf("Number of lines: %d\n", lineCount);
				}
                // Parent process
                waitpid(child_pid2, &status, 0);
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);
                    printf("The process with PID %d has ended with exit code %d.\n", child_pid2, exit_code);
                }
            }
            break;
        case S_IFDIR: // directory
            printf("File type: directory\n");
            display_directory_menu(path);
			pid_t child_pid3 = fork();
            if (child_pid3 < 0) {
                print_error_message("Failed to create child process.\n");
                return;
            } else if (child_pid3 == 0) {
                // Child process
                char filename[100];
                snprintf(filename, sizeof(filename), "%s_file.txt", path);
                char command[100];
                snprintf(command, sizeof(command), "touch %s", filename);
                system(command);
                exit(0);
            }

            // Parent process
            waitpid(child_pid3, &status, 0);
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                printf("The process with PID %d has ended with exit code %d.\n", child_pid3, exit_code);
            }
            break;
        case S_IFLNK: // symbolic link
            printf("File type: symbolic link\n");
            display_symbolic_link_menu(path);
			pid_t child_pid4 = fork();
            if (child_pid4 < 0) {
                print_error_message("Failed to create child process.\n");
                return;
            } else if (child_pid4 == 0) {
                // Child process
                char command[100];
                snprintf(command, sizeof(command), "chmod 760 %s", path);
                system(command);
                exit(0);
            }

            // Parent process
            waitpid(child_pid4, &status, 0);
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                printf("The process with PID %d has ended with exit code %d.\n", child_pid4, exit_code);
            }

            break;
        default:
            printf("File type: unknown\n");
    }
}

int main(int argc, char* argv[]) {
        for (int i = 1; i < argc; i++) {
            pid_t child_pid1 = fork();
            if (child_pid1 < 0) {
                print_error_message("Failed to create child process.\n");
                return 1;
            } else if (child_pid1 == 0) {
                // Child process
                display_file_info(argv[i]);
                exit(0);
            }
        }
        // Parent process
        int status;
        pid_t pid;
        while ((pid = wait(&status)) > 0) {
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                printf("The process with PID %d has ended with exit code %d.\n", pid, exit_code);
            }
        }
    
        return 0;
    }
