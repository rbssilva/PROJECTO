#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
char buffer[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;
int fileSize = 0;

//pthread_mutex_t mutexnode;
pthread_mutex_t mutexcommand;
//pthread_rwlock_t rwnode;
pthread_rwlock_t rwcommand;


static void checkArgs (long argc, char* const argv[]){
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        exit(EXIT_FAILURE);
    }
    numberThreads = atoi(argv[3]);
    if((strcmp(argv[4], "nosync")== 0) && numberThreads != 1){
      fprintf(stderr, "Invalid format, use only 1 thread for nosync (sync strategy)\n");
      exit(EXIT_FAILURE);
    }
}

//uploads inputcommands string array with data from file
int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

//removes commands that were processed from inputcommands string array
char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void uploadBuffer(void* _in_file){
  int i = 0;

  while(fgets(buffer[i], MAX_INPUT_SIZE/sizeof(char), _in_file)){
    int len = strlen(buffer[i]);
    if((buffer[i][len-1] == '\n'))
      buffer[i][len-1] = '\0';
    i++;
  }

  fileSize = i;
}

void processInput(void* _in_file){
    int i = 0;
    int numTokens = 0;
    char token, type;
    char name[MAX_INPUT_SIZE];

    uploadBuffer(_in_file);

    /* break loop with ^Z or ^D */
    while (i < fileSize) {
        //printf("%d\n", i);
        numTokens = sscanf(buffer[i], "%c %s %c", &token, name, &type);
        /* perform minimal validation */
        if(numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(buffer[i]))
                    break;
                return;

            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(buffer[i]))
                    break;
                return;

            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(buffer[i]))
                    break;
                return;

            case '#':
                break;

            default: { /* error */
                errorParse();
            }
        }
        i++;
    }
}

void* applyCommands(){
    while (1){
        //trinco_lock
        if(numberCommands > 0){
          const char* command = removeCommand();
          if (command == NULL){
            //trinco_unlock
              continue;
          }
          //trinco_unlock

          char token, type;
          char name[MAX_INPUT_SIZE];
          int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
          if (numTokens < 2) {
              fprintf(stderr, "Error: invalid command in Queue\n");
              exit(EXIT_FAILURE);
          }

          int searchResult;
          switch (token) {
              case 'c':
                  switch (type) {
                      case 'f':
                          printf("Create file: %s\n", name);
                          sync_create(name, T_FILE);
                          break;
                      case 'd':
                          printf("Create directory: %s\n", name);
                          sync_create(name, T_DIRECTORY);
                          break;
                      default:
                          fprintf(stderr, "Error: invalid node type\n");
                          exit(EXIT_FAILURE);
                  }
                  break;
              case 'l':
            //---lock
                  searchResult = lookup(name);
                  if (searchResult >= 0)
                      printf("Search: %s found\n", name);
                  else
                      printf("Search: %s not found\n", name);
                //--unlock
                  break;
              case 'd':
              //---lock
                  printf("Delete: %s\n", name);
                  delete(name);
                //---unlock
                  break;
              default: { /* error */
                  fprintf(stderr, "Error: command to apply\n");
                  exit(EXIT_FAILURE);
              }
          }
      }
      else if( numberCommands == 0)
          break;
  }
  return 0;
}

void createThreads(pthread_t* tid){
  int i = 0;
  for(i = 0; i < numberThreads; i++){
    if(pthread_create(&tid[i], 0, applyCommands, NULL) != 0){
      fprintf(stderr, "Error: couldn't created thread\n");
      exit(EXIT_FAILURE);
    }
  }
  for(i = 0; i < numberThreads; i++){
    if(pthread_join(tid[i], NULL)!= 0){
      fprintf(stderr, "Error: Couldn't join thread\n");
      exit(EXIT_FAILURE);
    }
  }
}

//Prints the execution time
void get_time_and_print(struct timeval end, struct timeval start){
  double seconds = end.tv_sec - start.tv_sec;
  double micros = end.tv_usec - start.tv_usec;
  double time = micros/1000000 + seconds;

  printf("TecnicoFs completed in %.4lf seconds\n", time);
}

FILE* checkedFopen(char* name, char* mode){
  FILE *myFile = NULL;
  myFile = fopen(name, mode);
  if((myFile == NULL)){
    fprintf(stderr, "Couldn't open file %s\n", name);
    exit(EXIT_FAILURE);
  }


  return myFile;
}

void pth_init_all(char* const argv[]){
  if(strcmp(argv[4], "mutex")== 0){
    //pthread_mutex_init(&mutexnode, NULL);
    pthread_mutex_init(&mutexcommand, NULL);
  }
  else if(strcmp(argv[4], "rwlock")== 0){
    //pthread_rwlock_init(&rwnode, NULL);
    pthread_rwlock_init(&rwcommand, NULL);
  }
}

int main(int argc, char* argv[]) {

    /* init filesystem */



    init_fs();
    pth_init_all(argv);
    pthread_t tid[numberThreads];
    struct timeval start, end;

    checkArgs(argc, argv);

    //opening file
    FILE *in_file  = checkedFopen(argv[1], "r");
    FILE *out_file = checkedFopen(argv[2], "w");

    /* process input and print tree */
    processInput(in_file);

    gettimeofday(&start, NULL);

    createThreads(tid);

    print_tecnicofs_tree(out_file);

    gettimeofday(&end, NULL);
    get_time_and_print(end, start);

    /* release allocated memory */
    fclose(in_file);
    fclose(out_file);

    destroy_fs();
    exit(EXIT_SUCCESS);
}
