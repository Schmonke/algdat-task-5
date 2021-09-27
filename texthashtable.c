#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Create hash function that converts text keys to integers
// in the range [0, m-1] where m is the length of the array.

// "Lastfaktor" for an array with size m and n elements, is 
// given as a = n / m.



int convert_text_to_int(char* text)
{
    int length = strlen(text);
    int result = 0;

    for(int i = 0; i < length; i++)
    {
        //subtracting char A and adding 1 gives ASCII int value of char
        result += text[i] - 'A' + 1; 
    }

    return result;
}

// Temporary hashing func
int hashfunc(int key, int tablesize) 
{
    return key % tablesize;
}

int run_with_file(FILE *fp)
{
    char *line = NULL;
    size_t len = 0; //Will be update by getline
    int m = 42069; 
    char *table[m];
    int n, a;

    while(getline(&line, &len, fp) != -1)
    {
        n = convert_text_to_int(line);
        a = hashfunc(n, m);
        table[a] = line;
        //fputs(table[a], stdout); //for debugging.
    }

    free(line);

    return 0;
}

int handle_file(const char *file_path)
{
    FILE *file = fopen(file_path, "r");

    if(file == NULL) 
    {
        perror("Unable to open file.");
        return 1;
    }
    return run_with_file(file);
}

void print_help()
{
    printf(
        "You must specify which file to read from as an argument to the program.\n"
        "Usage: texthashtable <file_name>\n"
        "E.g. ./texthashtable ~/navn.txt\n"
    );
}

int main(int argc, char const *argv[])
{
    if (argc < 2) 
    {
        print_help();
        return 1;
    }
    handle_file(argv[1]);
}
