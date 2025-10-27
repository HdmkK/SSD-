#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define MAX_LEN 1024
#define LB_SIZE 100
#define READ_FILE "result.txt"

void readLine(char line[]){

    // 입력 받기
    if (fgets(line, MAX_LEN, stdin) == NULL) {
        printf("\n");
        return; // Ctrl+D 등으로 입력 종료 시
    }

    // 개행 문자 제거
    line[strcspn(line, "\n")] = '\0';
}


bool check_LB_Address(const char* LB_Address_str){

    errno = 0;
    char* endptr = NULL;
    (void)strtol(LB_Address_str, &endptr, 10);

    if (LB_Address_str == endptr) return false;
    if (*endptr != '\0') return false;
    if (errno == ERANGE) return false;

    return true;
}

bool check_value(const char* value_str){

    errno = 0;
    char* endptr = NULL;
    (void)strtol(value_str, &endptr, 16);

    if (value_str == endptr) return false;
    if (*endptr != '\0') return false;
    if (errno == ERANGE) return false;

    return true;
}

void doWriteToSSD(){
    const char* LB_Address_str = strtok(NULL, " ");
    const char* value_str = strtok(NULL, " ");
    const char* extra = strtok(NULL, " ");

    if (extra != NULL || !check_LB_Address(LB_Address_str) || !check_value(value_str)) {
        printf("[ERROR] : write <logical block address(0 - 99)> <value(hex32)>\n");
        return;
    }


    pid_t pid = fork();
    if (pid == 0){
        const char * const args[] = {"./ssd", "W", LB_Address_str, value_str, NULL};
        execvp(args[0], (char *const *)args);
        perror("exec failed");
        exit(1);
    }
    else if (pid > 0){
        wait(NULL);
    }
}

void doReadFromSSD(){
    const char* LB_Address_str = strtok(NULL, " ");
    const char* extra = strtok(NULL, " ");

    if (extra != NULL || !check_LB_Address(LB_Address_str)) {
        printf("[ERROR] : read <logical block address(0 - 99)>\n");
        return;
    }


    pid_t pid = fork();
    if (pid == 0){
        const char * const args[] = {"./ssd", "R", LB_Address_str, NULL};
        execvp(args[0], (char *const *)args);
        perror("exec failed");
        exit(1);
    }
    else if (pid > 0){
        wait(NULL);

        uint8_t fd = open(READ_FILE, O_RDONLY, 0666);
        uint8_t buf[MAX_LEN];
        uint8_t n = read(fd, buf, MAX_LEN);
        buf[n] = '\0';
        
        printf("%s\n", buf);

        close(fd);
    }
}

void doFullWriteToSSD(){

    const char* value_str = strtok(NULL, " ");
    const char* extra = strtok(NULL, " ");

    if (extra != NULL || !check_value(value_str)) {
        printf("[ERROR] : fullwrite <value(hex32)>\n");
        return;
    }


    for (uint8_t LB_Address = 0; LB_Address < LB_SIZE; ++LB_Address){
        uint8_t LB_Address_str[MAX_LEN];
        sprintf(LB_Address_str, "%d", LB_Address);

        pid_t pid = fork();
        if (pid == 0){
            const char * const args[] = {"./ssd", "W", LB_Address_str, value_str, NULL};
            execvp(args[0], (char *const *)args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0){
            wait(NULL);
        }
    }
}

void doFullReadFromSSD(){

    const char* extra = strtok(NULL, " ");

    //쓸데없는 추가 인자
    if (extra != NULL) {
        printf("[ERROR] : fullread\n");
        return;
    }

    uint8_t fd = open(READ_FILE, O_RDONLY, 0666);

    for (uint8_t LB_Address = 0; LB_Address < LB_SIZE; ++LB_Address){
        char LB_Address_str[MAX_LEN];
        sprintf(LB_Address_str, "%d", LB_Address);

        pid_t pid = fork();
        if (pid == 0){
            const char * const args[] = {"./ssd", "R", LB_Address_str, NULL};
            execvp(args[0], (char *const *)args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0){
            wait(NULL);

            lseek(fd, 0, SEEK_SET);
            uint8_t buf[MAX_LEN];
            uint8_t n = read(fd, buf, MAX_LEN);
            buf[n] = '\0';
            
            printf("[%d] : %s\n", LB_Address, buf);
        }
    }

    close(fd);
}

void doHelp(){

    const char* extra = strtok(NULL, " ");

    //쓸데없는 추가 인자
    if (extra != NULL) {
        printf("[ERROR] : help\n");
        return;
    }

    printf("Available commands:\n");
    printf("  write <logical_block_address(0 - 99)> <value(hex32)>      : Write 32-bit value to given logical block.\n");
    printf("  read <logical_block_address(0 - 99)>                      : Read 32-bit value from given logical block.\n");
    printf("  fullwrite <value(hex32)>                          : Write the same 32-bit value to all logical blocks.\n");
    printf("  fullread                                          : Read and print all logical blocks' value.\n");
    printf("  help                                              : Show this help message.\n");
    printf("  exit                                              : Exit the shell.\n");
}

uint32_t createRandNum(){
    return (uint32_t)((rand() << 1) | (rand() & 0x1));
}

void doTestApp1(){

    bool total_result = true;
    
    printf("-------------------------------------------------------------\n");
    printf(" LBA   |  Written Value  |  Read Value   | Result\n");
    printf("-------------------------------------------------------------\n");

    int8_t fd = open(READ_FILE, O_RDONLY | O_CREAT | O_TRUNC, 0644);

    uint32_t testWriteValues[LB_SIZE];
    uint32_t testReadValues[LB_SIZE];

    const char* extra = strtok(NULL, " ");

    //쓸데없는 추가 인자
    if (extra != NULL) return;

    /* fullwrite*/
    for (uint8_t LB_Address = 0; LB_Address < LB_SIZE; ++LB_Address){

        uint8_t LB_Address_str[MAX_LEN];
        sprintf(LB_Address_str, "%d", LB_Address);


        uint8_t value_str[MAX_LEN];
        uint32_t value = createRandNum();
        sprintf(value_str, "0x%08X", value);

        testWriteValues[LB_Address] = value;

        pid_t pid = fork();
        if (pid == 0){
            const char * const args[] = {"./ssd", "W", LB_Address_str, value_str, NULL};
            execvp(args[0], (char *const *)args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0){
            wait(NULL);
        }


        pid = fork();
        if (pid == 0){
            const char * const args[] = {"./ssd", "R", LB_Address_str, NULL};
            execvp(args[0], (char *const *)args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0){
            wait(NULL);

            lseek(fd, 0, SEEK_SET);
            uint8_t buf[MAX_LEN];
            uint8_t n = read(fd, buf, MAX_LEN);
            buf[n] = '\0';
            
            
            bool flag = true;
            uint8_t* endptr = NULL;
            uint32_t readValue = strtol(buf, (char**)&endptr, 16);
            
            if (buf == endptr) flag = false;
            if (*endptr != '\0') flag = false;
            if (errno == ERANGE) flag = false;

            testReadValues[LB_Address] = readValue;

            if (testWriteValues[LB_Address] != testReadValues[LB_Address]){
                flag = false;
                total_result = false;
            }

            printf(" %4u  |  0x%08X     |  0x%08X    |  %s\n", 
                LB_Address, testWriteValues[LB_Address], testReadValues[LB_Address], (flag ? "Success" : "Fail"));
        }
    }

    

    printf("-------------------------------------------------------------\n");
    printf("total result : %s\n", (total_result ? "SUCCESS" : "FAIL") );
    close(fd);
}

void doTestApp2(){

    printf("-------------------------------------------------------------\n");
    printf(" LBA  |  Prev Value   |  Written Value  |  Read Value   | Result\n");
    printf("-------------------------------------------------------------\n");

    bool total_result = true;

    const uint8_t TEST_RANGE = 6;
    const char* TEST_VALUE = "0xAAAABBBB";
    const char* TEST_OVERWRITE_VALUE = "0x12345678";

    /* first write */
    for (uint8_t LB_Address = 0; LB_Address < 6; ++LB_Address){
        uint8_t LB_Address_str[MAX_LEN];
        sprintf(LB_Address_str, "%d", LB_Address);

        pid_t pid = fork();
        if (pid == 0){
            const char * const args[] = {"./ssd", "W", LB_Address_str, (char* const)TEST_VALUE, NULL};
            execvp(args[0], (char *const *)args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0){
            wait(NULL);
        }
    }

    /* second write */
    for (uint8_t LB_Address = 0; LB_Address < 6; ++LB_Address){
        uint8_t LB_Address_str[MAX_LEN];
        sprintf(LB_Address_str, "%d", LB_Address);

        pid_t pid = fork();
        if (pid == 0){
            const char * const args[] = {"./ssd", "W", LB_Address_str, (char* const)TEST_OVERWRITE_VALUE, NULL};
            execvp(args[0], (char *const *)args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0){
            wait(NULL);
        }
    }


    /* read */
    int8_t fd = open(READ_FILE, O_RDONLY | O_CREAT | O_TRUNC, 0644);
    for (uint8_t LB_Address = 0; LB_Address < 6; ++LB_Address){


        uint8_t LB_Address_str[MAX_LEN];
        sprintf(LB_Address_str, "%d", LB_Address);

        pid_t pid = fork();
        if (pid == 0){
            const char * const args[] = {"./ssd", "R", LB_Address_str, NULL};
            execvp(args[0], (char *const *)args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0){
            wait(NULL);

            lseek(fd, 0, SEEK_SET);
            uint8_t buf[MAX_LEN];
            uint8_t n = read(fd, buf, MAX_LEN);
            buf[n] = '\0';
            
            
            bool flag = true;
            uint8_t* endptr = NULL;
            uint32_t readValue = strtol(buf, (char**)&endptr, 16);
            
            if (buf == endptr) flag = false;
            if (*endptr != '\0') flag = false;
            if (errno == ERANGE) flag = false;

            uint32_t overwrite_value = strtol(TEST_OVERWRITE_VALUE, NULL, 16);

            if (readValue != overwrite_value){
                flag = false;
                total_result = false;
            }
            printf(" %4u  |  %s   |  %s     |  %s   |  %s\n",
               LB_Address, TEST_VALUE, TEST_OVERWRITE_VALUE, buf, (flag ? "SUCCESS" : "FAIL"));
        }
    }

    printf("-------------------------------------------------------------\n");
    printf("total result : %s\n", (total_result ? "SUCCESS" : "FAIL") );
}


int main() {

    srand(time(NULL));
    
    char line[MAX_LEN];

    while (1) {
        printf(">> ");
        fflush(stdout);

         readLine(line);

        // 아무 입력도 없을 때는 무시
        if (strlen(line) == 0)
            continue;

        const char* cmd_token = strtok(line, " ");

        if (strcmp(cmd_token, "write") == 0){
            doWriteToSSD();
        }
        else if (strcmp(cmd_token, "read") == 0){
            doReadFromSSD();
        }
        else if (strcmp(cmd_token, "fullwrite") == 0){
            doFullWriteToSSD();
        }
        else if (strcmp(cmd_token, "fullread") == 0){
            doFullReadFromSSD();
        }
        else if (strcmp(cmd_token, "help") == 0){
            doHelp();
        }
        else if (strcmp(cmd_token, "exit") == 0) {
            printf("Bye!\n");
            break;
        }
        else if (strcmp(cmd_token, "testapp1") == 0){
            doTestApp1();
        }
        else if (strcmp(cmd_token, "testapp2") == 0){
            doTestApp2();
        }
        else{
            printf("INVALID COMMAND\n");
        }
    }

    return 0;
}
