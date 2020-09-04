//
// Created by 听见 on 2020/8/30.
//
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
int T_ID = 1;

#define    DNS_TYPE_A                1
#define    DNS_TYPE_NS                2
#define    DNS_TYPE_CNAME            5
#define    DNS_TYPE_SOA            6
#define    DNS_TYPE_PTR            12
#define    DNS_TYPE_MX                15
#define    DNS_TYPE_TXT            16
#define    DNS_TYPE_AAAA            28

#define    DNS_CLASS_IN            1
#define STR_P 0b11000000u

#define STR_ADDR_MASK (~(0b1100000000000000u))
typedef struct {
  uint32_t ID: 16;
  uint32_t QR: 1;//0表示查询报文，1表示响应报文。
  uint32_t OpCode: 4;//通常值为0（标准查询），其他值为1（反向查询）和2（服务器状态请求）。
  uint32_t AA: 1;// 表示“授权回答(authoritative answer)”。该名字服务器是授权于该域的。
  uint32_t TC: 1;// 表示“可截断的(truncated)”。使用UDP时，它表示当应答的总长度超过512字节时，只返回前512个字节。
  uint32_t RD:
      1;// “期望递归（recursion desired）”。该比特能在一个查询中设置，并在响应中返回。这个标志告诉名字服务器必须处理这个查询，也称为一个递归查询。如果该位为0，且被请求的名字服务器没有一个授权回答，它就返回一个能解答该查询的其他名字服务器列表，这称为迭代查询。
  uint32_t RA: 1;// 表示“可用递归”。如果名字服务器支持递归查询，则在响应中将该比特设置为1。
  uint32_t Z: 1;// 必须为0。
  uint32_t AD: 1;// 必须为0。
  uint32_t CD: 1;// 必须为0。
  uint32_t RCODE: 4;// 通常的值为0（没有差错）和3（名字差错）。名字差错只有从一个授权名字服务器上返回，它表示在查询中制定的域名不存在。
  uint32_t queries: 16;//对于查询报文，问题(question)数通常是1   应答报文
  uint32_t answers: 16;//查询报文均为0                          应答报文，回答数至少是1
  uint32_t auth_rr: 16;//查询报文均为0                          均为0
  uint32_t add_rr: 16;//查询报文均为0                           均为0
} proto_dns_header;

typedef struct {
  char *name;
  uint16_t type;
  uint16_t class;
} proto_dns_query;

typedef struct {
  char *name;
  uint16_t type;
  uint16_t class;
  uint32_t ttl;
  uint16_t dataLen;
  union {
    char *cname;
    char *addrName;
  } data;
} proto_dns_answer;

void parse_dns(uint8_t buf[1024], size_t size);

void build_dns_request(uint8_t *buf, size_t *size, const char *query);

static int parse_dns_header(uint8_t *buf, size_t size, proto_dns_header *header);

static uint16_t readShort(uint8_t *start, size_t *curPos);
static char *readIP(uint8_t *start, size_t *curPos);
static uint32_t readInt(uint8_t *start, size_t *curPos);
static void readStr(uint8_t *start, size_t *curPos, char *buf, uint16_t maxLen);

int parse_dns_header(uint8_t *buf, size_t size, proto_dns_header *header) {
  uint8_t *cur = buf;
  if (header == NULL || size < sizeof(proto_dns_header)) {
    return -1;
  }
  memcpy(header, buf, sizeof(proto_dns_header));
  header->ID = ntohs(header->ID);
  header->queries = ntohs(header->queries);
  header->answers = ntohs(header->answers);
  return 0;
}
char *readIP(uint8_t *start, size_t *curPos) {
  struct in_addr addr;
  addr.s_addr = *(uint32_t *) (start + *curPos);
  char *tmp = inet_ntoa(addr);
  *curPos += 4;
  return tmp;
}

uint32_t readInt(uint8_t *start, size_t *curPos) {
  uint16_t tmp = ntohl(*(uint32_t *) (start + *curPos));
  *curPos += 4;
  return tmp;
}
uint16_t readShort(uint8_t *start, size_t *curPos) {
  uint16_t tmp = ntohs(*(uint16_t *) (start + *curPos));
  *curPos += 2;
  return tmp;
}
/*
   DNS name 格式 :

   無壓縮, 範例 : tw.yahoo.com
   | 2 | t | w | 5 | y | a | h | o | o | 3 | c | o | m | \0 |

   使用壓縮時, 使用指針指向之前出現的域名的位置.
   指針的大小為 2byte, 最高 2bit 為 11 (11xxxxxxxxxxxxxx),
   剩餘的 10bit 紀錄以 DNS 封包開頭為基底往後偏移,

   解壓縮時, 將 2byte 資料和 0x3FFF 做 AND 運算, 得到的值再以 DNS 封包開頭為基底往後偏移,
   就是之前出現的域名的位置.

   範例-01 (全部壓縮) :
   第一個域名為 tw.yahoo.com, 則域名欄位為 :
   | 2 | t | w | 5 | y | a | h | o | o | 3 | c | o | m | \0 |
   假設 | 2 | 的偏移植是 0x0E
   第二個域名和第一個域名相同, 則域名欄位為 :
   | 0xC00E |
   (0xC000 | 0x000E = 0xC00E)

   範例-02 (部分壓縮) :
   第一個域名為 tw.yahoo.com, 則域名欄位為 :
   | 2 | t | w | 5 | y | a | h | o | o | 3 | c | o | m | \0 |
   假設 | 5 | 的偏移植是 0x0F
   第二個域名為 www.yahoo.com, 壓縮 yahoo.com, 則域名欄位為 :
   | 3 | w | w | w | 0xC00F |

   範例-03 (多重壓縮) :
   第一個域名為 tw.yahoo.com, 則域名欄位為 :
   | 2 | t | w | 5 | y | a | h | o | o | 3 | c | o | m | \0 |
   假設 | 5 | 的偏移植是 0x000F
   第二個域名為 www.yahoo.com, 壓縮 yahoo.com, 則域名欄位為 :
   | 3 | w | w | w | 0xC00F |
   假設 | 3 | 的偏移植是 0x001D
   第三個域名和第二個相同, 則域名欄位為 :
   | 0xC01D |

   注意事項 :
   一個域名欄位只能有一個指針, 指針後面不可有域名字串, 指針後面不需要加上終止字元 '\0'.
   範例 :
   | 0xC01D | = 正確.
   | 3 | w | w | w | 0xC00F | = 正確.
   | 0xC00E | 4 | r | o | o | t | \0 | = 錯誤 (指針後面不可有域名字串).
   */
void readStr(uint8_t *start, size_t *curPos, char *buf, uint16_t maxLen) {
  uint8_t flag = 0;
  size_t count = 0;
  size_t bufCur = 0;
  memset(buf, 0, sizeof(char));
  uint8_t *curStart = start + *curPos;
  size_t curReadCount = 0;
  while (*(curStart + count)) {
    //超过长度了
    if (bufCur == maxLen) {
      buf[maxLen - 1] = '\0';
      return;
    }
    if (curReadCount == 0) {
      if (*(curStart + count) & STR_P) {
        uint16_t addr = ntohs(*((uint16_t *) (curStart + count))) & STR_ADDR_MASK;
        if (flag == 0) {
          //第一层 有指针 这样已经结束了
          flag = 1;
          *curPos += count + 2;
        }
        count = 0;
        curStart = start + addr;
      } else {
        curReadCount = *(curStart + count++);
        if (bufCur != 0) {
          buf[bufCur++] = '.';
        }
      }

    } else {
      buf[bufCur++] = *(curStart + count++);
      curReadCount--;
    }

  }
  if (flag == 0) {
    //跳过0
    (*curPos)++;
    *curPos += count;
  }
}

void parse_dns(uint8_t *readBuf, size_t readSize) {
  size_t curPos = 0;
  proto_dns_header header;
  parse_dns_header(readBuf, readSize, &header);
  curPos += sizeof(proto_dns_header);
  uint16_t queryCount = header.queries;
  uint16_t queryCur = 0;
  proto_dns_query queries[queryCount];
  uint16_t answerCount = header.answers;
  uint16_t answerCur = 0;
  proto_dns_answer answers[answerCount];
  char tmpName[256] = {0};
  while (curPos < readSize) {
    //解析query
    if (queryCount > 0) {
      readStr(readBuf, &curPos, tmpName, sizeof(tmpName));
      char *name = strdup(tmpName);
      queries[queryCur].name = name;
      queries[queryCur].type = readShort(readBuf, &curPos);
      queries[queryCur].class = readShort(readBuf, &curPos);
      queryCur++;
      queryCount--;
    }
    //解析answer
    if (answerCount > 0) {
      readStr(readBuf, &curPos, tmpName, sizeof(tmpName));
      char *name = strdup(tmpName);
      answers[answerCur].name = name;
      answers[answerCur].type = readShort(readBuf, &curPos);
      answers[answerCur].class = readShort(readBuf, &curPos);
      answers[answerCur].ttl = readInt(readBuf, &curPos);
      answers[answerCur].dataLen = readShort(readBuf, &curPos);
      if (answers[answerCur].type == DNS_TYPE_A) {
        answers[answerCur].data.addrName = readIP(readBuf, &curPos);
      } else if (answers[answerCur].type == DNS_TYPE_CNAME) {
        readStr(readBuf, &curPos, tmpName, sizeof(tmpName));
        answers[answerCur].data.cname = strdup(tmpName);
      }
      answerCur++;
      answerCount--;
    } else {
      break;
    }
  }
}
void build_dns_request(uint8_t *buf, size_t *size, const char *query) {
  proto_dns_header header;
  uint8_t *cur = buf;
  memset(&header, 0, sizeof(proto_dns_header));
  header.ID = htons(T_ID++);
  header.QR = 0;
  header.queries = htons(1);
  memcpy(cur, &header, sizeof(proto_dns_header));
  cur += sizeof(proto_dns_header);
  uint32_t i = 0;
  uint8_t *start = cur;
  cur++;
  uint32_t count = 0;
  while (query[i]) {
    //处理点
    if (query[i] == '.') {
      *start = count;
      count = 0;
      start = cur;
      cur++;
      i++;
      continue;
    }
    //其他
    *(cur++) = query[i];
    i++;
    count++;
  }
  *start = count;
  *(cur++) = '\0';
  // 查询类型
  *((uint16_t *) cur) = htons(1);
  cur += 2;
  *((uint16_t *) cur) = htons(1);
  cur += 2;
  *size = cur - buf;
}

int main() {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in sockaddr_bind;
  bzero(&sockaddr_bind, sizeof(sockaddr_bind));
  sockaddr_bind.sin_family = AF_INET;
  sockaddr_bind.sin_port = htons(30000);
  sockaddr_bind.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(fd, (const struct sockaddr *) &sockaddr_bind, sizeof(sockaddr_bind));
  struct sockaddr_in sockaddr_dns;
  bzero(&sockaddr_dns, sizeof(sockaddr_dns));
  sockaddr_dns.sin_family = AF_INET;
  sockaddr_dns.sin_port = htons(53);
  inet_pton(AF_INET, "114.114.114.114", &sockaddr_dns.sin_addr);
  uint8_t sendBuf[1024];
  size_t sendSize = 0;
  build_dns_request(sendBuf, &sendSize, "www.qq.com");
  sendto(fd, sendBuf, sendSize, 0, (const struct sockaddr *) &sockaddr_dns, sizeof(sendBuf));

  struct sockaddr_in sockaddr_in;
  socklen_t socklen_in = sizeof(sockaddr_in);
  uint8_t readBuf[1024];
  size_t readSize = 1024;
  while (1) {
    readSize = recvfrom(fd, readBuf, readSize, 0, (struct sockaddr *) &sockaddr_in, &socklen_in);
    printf("[%s:%d]\n", inet_ntoa(sockaddr_in.sin_addr), ntohs(sockaddr_in.sin_port));
    parse_dns(readBuf, readSize);
  }

  close(fd);
  return 0;
}

