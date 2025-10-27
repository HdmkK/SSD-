# 🧠 SSD Simulator & Test Shell

## 📦 개요
이 프로젝트는 간단한 NAND 플래시 시뮬레이터(`ssd`)와 이를 제어하는 셸 프로그램(`testShell`)로 구성되어 있습니다.  
`ssd`는 `nand.txt` 파일을 메모리 매핑하여 논리 블록(LBA) 단위로 데이터를 읽고 쓰며,  
`testShell`은 이를 명령어 기반으로 제어할 수 있도록 제공합니다.


## 구조도
<p align="center">
  <img src="./SSD프로젝트/images/구조도.png" alt="구조도 이미지" width="500">
</p>


## 차별점

nand.txt 접근 시, mmap으로 파일을 특정 가상주소공간에 매핑하여 nand에 read/write하는 것을 가상으로 구현

<p align="center">
  <img src="./SSD프로젝트/images/mmap.png" alt="mmap 이미지" width="500">
</p>

장점 : 
1. read/write 시스템 콜을 통해 파일에 접근 X -> 가상주소에 random access하므로 overhead감소.
2. 바이너리 형태로 데이터를 저장함에도 불구하고 가상주소로 접근하므로, 특정 logical block에 접근하기 용이하다.
ex) mappedAddress[logical block 주소] = 0x12345678;


mmap은 가상주소일부분과 물리메모리의 페이지를 매핑한다. 하지만 처음부터 모든 페이지가 메모리에 로딩되진 않는다. 접근하려는 페이지가 메모리에 없을 때, page fault발생한다.
이때 일반 페이지는 swap영역에서 페이지를 가져오는데 반해, mmap 페이지는 파일에서 직접 페이지를 가져온다.

중요한 점은 mmap으로 매핑된 가상주소에 write를 한다고 해서, 즉시 파일에 반영되는 것은 아니라는 것이다. write를 하는건 물리메모리의 페이지에 write하는 것이다. 이후 커널이 적절한 시간에 물리메모리 페이지와 디스크의 파일을 동기화한다.(즉시 동기화 하고 싶다면 msync() 사용)

---

## ⚙️ SSD Simulator (`ssd`)

### 🔹 실행 형식
`./ssd <command> <arguments...>`

### 🔹 개요
`nand.txt` 파일을 메모리 맵핑(`mmap`)하여 100개의 논리 블록(0~99)을 시뮬레이션합니다.  
각 블록은 32비트(4바이트) 크기를 가지며, write 결과는 nand.txt, read 결과는 result.txt에 기록됩니다.

---

### 1️⃣ **write**
- **설명:** 지정한 논리 블록 주소(LBA)에 32비트 16진수 데이터를 기록합니다.  
- **사용법:** `./ssd W <LBA> <hex32>`  
- **예시:** `./ssd W 10 0x1234ABCD`  
- **동작:**  
  - `nand.txt`를 `mmap()`으로 열어 해당 LBA 위치에 데이터를 기록  
  - `msync()`를 호출하여 파일에 즉시 반영

<p align="center">
  <img src="./SSD프로젝트/images/ssd 캡처.png" alt="write 이미지" width="500">
</p>

---

### 2️⃣ **read**
- **설명:** 지정한 LBA의 데이터를 읽어 `result.txt`에 출력합니다.  
- **사용법:** `./ssd R <LBA>`  
- **예시:** `./ssd R 10`  
- **출력(`result.txt`):**
  ```
  0x1234ABCD
  ```

<p align="center">
  <img src="./SSD프로젝트/images/ssd 캡처.png" alt="read 이미지" width="500">
</p>
---

## 💻 Test Shell (`testShell`)

### 🔹 실행 형식
`./testShell`

### 🔹 개요
`testShell`은 명령 기반으로 `ssd` 프로그램을 호출하는 셸입니다.  
각 명령은 `fork()` + `exec()`를 통해 `ssd`를 실행하며, 결과는 `result.txt`로부터 읽어옵니다.  
입력 명령을 통해 블록 단위 읽기/쓰기, 전체 테스트 수행이 가능합니다.

---

### 1️⃣ **write**
- **설명:** 지정한 LBA에 32비트 16진수 값을 기록합니다.  
- **사용법:** `write <LBA> <hex32>`  
- **예시:** `write 5 0xAABBCCDD`  

<p align="center">
  <img src="./SSD프로젝트/images/testShell_readWrite.png" alt="write 이미지" width="500">
</p>
---

### 2️⃣ **read**
- **설명:** 지정한 LBA의 값을 읽어 콘솔에 출력합니다.  
- **사용법:** `read <LBA>`  


<p align="center">
  <img src="./SSD프로젝트/images/testShell_readWrite.png" alt="read 이미지" width="500">
</p>
---

### 3️⃣ **fullwrite**
- **설명:** 전체 100개 LBA(0~99)에 랜덤 데이터를 씁니다.  
- **사용법:** `fullwrite`  


<p align="center">
  <img src="./SSD프로젝트/images/testShell_fullwrite.png" alt="fullwrite 이미지" width="500">
</p>
---

### 4️⃣ **fullread**
- **설명:** 전체 LBA의 데이터를 읽어 콘솔에 출력합니다.  
- **사용법:** `fullread`  


<p align="center">
  <img src="./SSD프로젝트/images/testShell_fullread.png" alt="fullread 이미지" width="500">
</p>
---

### 5️⃣ **testapp1**
- **설명:**  
  모든 LBA(0~99)에 랜덤 값을 기록한 뒤 다시 읽어와 일치 여부를 검증합니다.  
- **사용법:** `testapp1`  

<p align="center">
  <img src="./SSD프로젝트/images/testShell_testapp1.png" alt="testapp1 이미지" width="500">
</p>
---

### 6️⃣ **testapp2**
- **설명:**  
  LBA 0~5에 고정된 값을 기록한 후, 다른 값으로 덮어써서 데이터가 정상적으로 갱신되는지 확인합니다.  
- **사용법:** `testapp2`  

<p align="center">
  <img src="./SSD프로젝트/images/testShell_testapp2.png" alt="testapp2 이미지" width="500">
</p>
---

### 7️⃣ **help**
- **설명:** 사용 가능한 명령 목록을 표시합니다.  
- **사용법:** `help`  

<p align="center">
  <img src="./SSD프로젝트/images/testShell_help.png" alt="help 이미지" width="500">
</p>
---

### 8️⃣ **exit**
- **설명:** 셸 프로그램을 종료합니다.  
- **사용법:** `exit`


<p align="center">
  <img src="./SSD프로젝트/images/testShell_exit.png" alt="exit 이미지" width="500">
</p>
---

"# SSD-" 
