#ifndef __SOCKDATA_H__
#define __SOCKDATA_H__

#define MAXCIDNAME 20
#define MAXCMDNAME 128

struct ProcCid{
  char cid[MAXCIDNAME];
  u_int32_t localadd, remoteadd;
  u_int16_t localport, remoteport;
  struct ProcCid *next, *previous;
};

struct ProcTcp{ 
  u_int32_t localadd, remoteadd;
  u_int16_t localport, remoteport;
  ino_t ino;
  uid_t uid;
  int state;
  struct ProcTcp *next, *previous;
};

struct ProcFd{
  ino_t ino;
  pid_t pid;
  char cmdline[MAXCMDNAME];  
  struct ProcFd *next, *previous;
};

struct SockData{
  char cid[MAXCIDNAME];
  u_int32_t localadd, remoteadd;
  u_int16_t localport, remoteport;
  ino_t ino;
  uid_t uid;
  pid_t pid;
  int state;
  char cmdline[MAXCMDNAME];
//  struct SockData *next, *previous;
};


void fill_cidlist(void);
void fill_tcplist(void);
void fill_fdlist(void);
void fill_socklist(void);
void sort_socklist(int (*compar)(const void *, const void *));
void search_socklist(int (*compar)(const void *, const void *));
void list_socklist(void (*callback)(struct SockData *, void *),
                   void *userdata);

#endif  /* __SOCKDATA_H__ */
