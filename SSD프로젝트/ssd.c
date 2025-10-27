/*
* 1. 어느 운영체제에서든 동일하게 실행되야 함.
* 2. 파일을 mmap으로 read/write
* 장점에 대해서 확실히 알아가야 할 듯
* 
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define LB_SIZE 100
#define NAND_FILE "./nand.txt"
#define READ_FILE "./result.txt"


uint32_t* mappedAddress = NULL;


/* 인자 파싱 */
bool checkArg_LBA(char* arg) {

	char* endptr = NULL;
	errno = 0;

	long num = strtoul(arg, &endptr, 10);

	if (arg == endptr) {
		//printf("정수로 변환 불가\n");
		return false;
	}
	if (*endptr != '\0') {
		//printf("완전한 정수가 아님\n");
		return false;
	}
	if (errno == ERANGE) {
		//printf("변환한 정수가 범위 초과\n");
		return false;
	}

	if (num < 0 || num >= LB_SIZE) {
		//printf("Logical Block 주소는 0 ~ 99\n");
		return false;
	}

	return true;
}

bool checkArg_LBValue(char* arg) {

	char* endptr = NULL;
	errno = 0;
	
	long value = strtoul(arg, &endptr, 16);

	if (arg == endptr) {
		//printf("16진수로 파싱할 수 없는 문자열\n");
		return false;
	}

	if (*endptr != '\0') {
		//printf("완전한 16진수가 아닙니다.\n");
		return false;
	}

	if (errno == ERANGE) {
		//printf("long 범위 초과\n");
		return false;
	}

	return true;
}

bool checkReadArgs(int argc, char* argv[]) {

	if (argc != 3) return false;

	return checkArg_LBA(argv[2]);
}

bool checkWriteArgs(int argc, char* argv[]) {

	if (argc != 4) return false;

	/* LBA 인자 검사 */
	if (checkArg_LBA(argv[2]) == false) return false;

	/* Value 인자 검사 */
	if (checkArg_LBValue(argv[3]) == false) return false;

	return true;
}

bool checkArgs(int argc, char* argv[]) {


	if (argc < 2) {
		printf("[ERROR] : too few arguments\n");
		return false;
	}

	if (strcmp(argv[1], "R") == 0) {
		if (checkReadArgs(argc, argv) == false){
			printf("[ERROR] : ssd R <logical block address(0 - 99)>\n");
			return false;
		}
	}
	else if (strcmp(argv[1], "W") == 0) {
		if (checkWriteArgs(argc, argv) == false){
			printf("[ERROR] : ssd W <logical block address(0 - 99)> <value(hex32)>\n");
			return false;
		}
	}
	else {
		printf("[ERROR] : invalid command\n");
		return false;
	}

	return true;
}


/* 명령 실행 */
void writeToSSD(uint8_t LB_Address, uint32_t value) {
    mappedAddress[LB_Address] = value;
}

uint32_t readFromSSD(uint8_t LB_Address) {
   return mappedAddress[LB_Address];
}


int openNandFile(const char* fileName){

    int fd = open(fileName, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd >= 0){
        ftruncate(fd, LB_SIZE * sizeof(uint32_t));

        void* mappedAddress = mmap(NULL, LB_SIZE * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        memset(mappedAddress, 0x00, LB_SIZE * sizeof(uint32_t));

        munmap(mappedAddress, LB_SIZE * sizeof(uint32_t));
    }
    else if (fd == -1 && errno == EEXIST){
        fd = open(fileName, O_RDWR);
    }

    
    if (fd == -1){
        perror("[error] : open");
        return -1;
    }

    return fd;
}

int main(int argc, char* argv[]) {


	if (checkArgs(argc, argv) == false) {
		return 1;
	}

	int fd = openNandFile(NAND_FILE);
    mappedAddress = (uint32_t*)mmap(NULL, LB_SIZE * sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

	if (strcmp(argv[1], "R") == 0) {
        int32_t fd = open(READ_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        uint8_t buf[1024];
        uint8_t LB_Address = strtoul(argv[2], NULL, 10);
        sprintf(buf, "0x%08X", readFromSSD(LB_Address));
        int ret = write(fd, buf, strlen(buf));

        close(fd);
	}
	else if (strcmp(argv[1], "W") == 0) {
        uint8_t LB_Address = strtoul(argv[2], NULL, 10);
        uint32_t value = strtoul(argv[3], NULL, 16);
        writeToSSD(LB_Address, value);
	}



    msync(mappedAddress, LB_SIZE * sizeof(uint32_t), MS_SYNC);
    munmap(mappedAddress, LB_SIZE * sizeof(uint32_t));
    
   
	return 0;
}