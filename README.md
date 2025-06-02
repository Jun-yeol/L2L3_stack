# **가상 L2 스위치 통신 개발**
<br>

### 🛠️프로젝트 개요
이 프로젝트는 가상 L2 스위치 통신 시뮬레이터 개발을 목표로합니다.
네트워크 내 노드 간 통신을 시뮬레이션하기 위해 Ethernet 및 ARP 프로토콜을 구현하였고 UDP 소켓, loopback 주소,
select() 모델을 활용하여 데이터 전송을 구현합니다.

### 🛠️기술 스택
- C
- Vim
- Make
- GDB
---
<br>


### 🏗️아키텍처
#### 네트워크 구조체

1. `interface_t` (인터페이스 구조체): 노드의 네트워크 인터페이스

  - 필드:

    if_name: 인터페이스의 이름

    att_node: 소속 노드

    link: 연결된 링크

    intf_nw_props: 네트워크 속성 구조체 (예: IP 주소, 서브넷 마스크 등)
<br>


2. `link_t` (링크 구조체): 두 인터페이스 간의 연결

  - 필드:

    intf1, intf2: 연결된 두 인터페이스

    cost: 링크의 비용 (네트워크 경로 계산 시 사용)
<br>

3. `node_t` (노드 구조체): 네트워크 내의 하나의 노드

  - 필드:
  
    node_name: 노드의 이름

    intf[]: 노드에 연결된 인터페이스 배열

    graph_glue: 노드를 그래프 내에 연결하기 위한 리스트 노드

    udp_port_number: 해당 노드의 UDP 포트 번호

    udp_sock_fd: UDP 소켓 파일 디스크립터

    node_nw_props: 노드의 네트워크 속성 (예: IP주소 등)
<br>

4. `graph_t` (그래프 구조체): 네트워크 전체를 나타내는 구조체

  - 필드:

    topology_name: 토폴로지 이름

    node_list: 토폴로지에 포함된 노드의 리스트 (링크드 리스트 형태)
<br>

5. 구조체들간에 연결 관계 시각화
<img src="https://github.com/user-attachments/assets/5fe0cba9-8007-479e-aa1f-56cd7728e45a" width="600px">
<br>

  이 구조체들을 기반으로 한 네트워크 시뮬레이터를 생성.

---

### 🔗통신 모델
<img src="https://github.com/user-attachments/assets/a6f144d8-cdeb-4afe-8129-e211157e67ac" width="600px">


#### 데이터 전송 시 select() 모델 과 loopback 주소를 활용

**select() 모델**을 사용하여 한 노드에서 목적지 노드로 데이터를 전송할 때, loopback 주소를 사용하여 데이터를 전송하면,
목적지 노드는 자체 UDP 소켓을 통해 해당 이벤트를 감지하고 데이터를 수신/처리할 수 있습니다.

이 과정에서 논리적으로 노드 간의 데이터 통신이 이루어짐.

<br>
<br>

---


### 📦프로토콜 설계

#### Ethernet 프로토콜

![스크린샷 2025-05-19 164117](https://github.com/user-attachments/assets/c49f96da-3151-4635-89f1-55db7fe15c63)

- 출발지 MAC, 목적지 MAC, 페이로드와 같은 필드로 구성된 Ethernet 프레임 구조를 구현

<br>

#### ARP 프로토콜

![image](https://github.com/user-attachments/assets/7f0b8598-e49f-42c8-bae0-3c8c811ccedb)

- IP주소와 MAC 주소 간의 매핑을 처리
- ARP 요청/응답 처리 및 ARP 테이블 관리

<br>

---

### ⚡구현 세부사항

<br>

#### 1. 네트워크 토폴로지 생성

![image](https://github.com/user-attachments/assets/f74f3c2c-2e67-45d9-8f39-c9bd0f7d987a)

- 그래프, 노드 생성.
- 각 노드의 인터페이스 설정, 노드끼리 연결.
- 네트워크 패킷 수신 thread생성
<br>

#### 2. 네트워크 수신 thread 시작점

- 노드별 udp소켓 생성
- 이벤트 발생까지 대기
- 파일 디스크립터가 활성화 되면 데이터 recevie
<br>

#### 3. 데이터 링크 계층 진입점

![image](https://github.com/user-attachments/assets/6e5111ef-758e-48f8-94ce-ea0b0c4295f0)

- 수신한 패킷을 overflow를 방지하기 위해 recv 버퍼에 오른쪽 정렬하여 저장
- layer 2계층으로 진입
<br>

#### 4. 이더넷 헤더의 Type 필드에 따라 ARP 메시지로 분기

![image](https://github.com/user-attachments/assets/cac807bb-5c6b-449f-8566-d52071a152fd)

##### ARP 메시지 루틴
- send_arp_broadcast_request():
  <br>
  ① 이더넷 패킷헤더의 FF:FF:FF:FF:FF 목적지 주소로 생성
  <br>
  ② ARP Broadcast Request 메시지 생성
  <br>
  ③ 패킷 데이터 전송
  <br>
- process_arp_broadcast_request():
  <br>
  ① broadcast요청으로 목적지 ip 비교
  <br>
  ② 이더넷 패킷의 payload(ARP 헤더)추출
  <br>
  ③ ARP헤더의 목적지 mac가 broadcast인지 확인
  <br>
  ④ send_arp_reply() 호출
  <br>
- process_arp_reply_mg
  <br>
  ① 응답 arp 패킷 받음
  <br>
  ② arp table 업데이트
  <br>

<br>


#### 5. 실행 결과

![스크린샷 2025-05-22 013850](https://github.com/user-attachments/assets/56876e18-bb34-4c55-b20c-7a5fd32927f8)

- 

![스크린샷 2025-05-22 014010](https://github.com/user-attachments/assets/0e0dfe32-cf98-4c3a-9c73-7acc3921dadc)


![스크린샷 2025-05-22 014021](https://github.com/user-attachments/assets/e865f481-2f33-4487-89d3-0a6f874e9ee7)

---

#### 📚 참고 자료 및 목적

본 프로젝트는 Udemy 강의  
[Networking Projects - Implement TCP/IP Stack in C][(https://www.udemy.com/course/tcpipstack/?couponCode=KEEPLEARNING)]  
을 기반으로 구현된 시뮬레이터입니다.

- UDP 기반의 가상 네트워크 통신 흐름
- 이더넷 헤더 구조 및 ARP 프로토콜 처리
- select() 기반 다중 I/O 처리 방식

등을 직접 분석하고 재현해보며 **네트워크 계층 간의 상호작용**에 대한 이해를 높이는 데 목적을 두었습니다.
