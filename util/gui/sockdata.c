#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
//#include <malloc.h>
#include <fcntl.h>
#include <string.h>

#include "sockdata.h"


#define LA_OFF  8
#define LP_OFF  6
#define RA_OFF  2
#define RP_OFF  0

struct ProcCid *CidList;
struct ProcTcp *TcpList;
struct ProcFd  *FdList;
struct SockData *SockPuppet;

int arraysize = 128;
int arraylength;

void fill_socklist(void)
{
  struct ProcCid *cidlist;
  struct ProcTcp *tcplist;
  struct ProcFd  *fdlist;
  struct SockData *socklist;
  char *result;
  int ii = 0;
  int tcp_entry, fd_entry;

  arraysize = 128;

  fill_cidlist(); 
  fill_tcplist();
  fill_fdlist();

//  if(SockPuppet) free(SockPuppet);
  SockPuppet = calloc(arraysize, sizeof(struct SockData));

  for(cidlist=CidList; cidlist!=NULL; cidlist=cidlist->next){ 
    tcp_entry = 0;
    for(tcplist=TcpList;tcplist!=NULL;tcplist=tcplist->next){
      if(tcplist->localport == cidlist->localport &&
	 tcplist->remoteadd == cidlist->remoteadd &&
         tcplist->remoteport == cidlist->remoteport){
	tcp_entry = 1;
	fd_entry = 0;
	for(fdlist=FdList; fdlist!=NULL; fdlist=fdlist->next){
	  if(fdlist->ino == tcplist->ino){
	    // then create a new entry
	    fd_entry = 1;
	    if(ii == arraysize){
	      arraysize *= 2;
	      SockPuppet = realloc(SockPuppet, (sizeof(struct SockData)*arraysize));
	    } 
	    strncpy(SockPuppet[ii].cmdline, fdlist->cmdline, MAXCMDNAME);
            SockPuppet[ii].pid = fdlist->pid;

	    strncpy(SockPuppet[ii].cid, cidlist->cid, MAXCIDNAME);
	    SockPuppet[ii].localadd = cidlist->localadd;
	    SockPuppet[ii].localport = cidlist->localport;
	    SockPuppet[ii].remoteadd = cidlist->remoteadd;
	    SockPuppet[ii].remoteport = cidlist->remoteport;
	    SockPuppet[ii].ino = tcplist->ino;
	    SockPuppet[ii].state = tcplist->state; 
	    SockPuppet[ii].uid = tcplist->uid;
	    arraylength = ++ii;
	  } 
	}
	if(!fd_entry){ // then user lacked permissions; add entry w/out cmdline
	  if(ii == arraysize){
	    arraysize *= 2;
	    SockPuppet = realloc(SockPuppet, (sizeof(struct SockData)*arraysize));            }

	    strncpy(SockPuppet[ii].cid, cidlist->cid, MAXCIDNAME);
	    SockPuppet[ii].localadd = cidlist->localadd;
	    SockPuppet[ii].localport = cidlist->localport;
	    SockPuppet[ii].remoteadd = cidlist->remoteadd;
	    SockPuppet[ii].remoteport = cidlist->remoteport;
	    SockPuppet[ii].ino = tcplist->ino;
	    SockPuppet[ii].state = tcplist->state; 
	    arraylength = ++ii;
	}
      }
    }
    if(!tcp_entry){ // then connection has vanished; add residual cid info
                    // (only for consistency with entries in /proc/web100)
      strncpy(SockPuppet[ii].cid, cidlist->cid, MAXCIDNAME);
      SockPuppet[ii].localadd = cidlist->localadd;
      SockPuppet[ii].localport = cidlist->localport;
      SockPuppet[ii].remoteadd = cidlist->remoteadd;
      SockPuppet[ii].remoteport = cidlist->remoteport; 
      SockPuppet[ii].state = 0;
      arraylength = ++ii;
    }
  } 
  while(CidList != NULL){
    cidlist = CidList; 
    CidList = CidList->next;
    if(CidList) CidList->previous = NULL; 
    free(cidlist); 
  }
  while(TcpList != NULL){
    tcplist = TcpList;
    TcpList = TcpList->next;
    if(TcpList) TcpList->previous = NULL;
    free(tcplist);
  }
  while(FdList != NULL){
    fdlist = FdList;
    FdList = FdList->next;
    if(FdList) FdList->previous = NULL;
    free(fdlist);
  }
}

void free_socklist(void)
{
    free(SockPuppet);
}

void fill_cidlist(void)
{
  struct ProcCid *cidlist;
  DIR *dirp;
  struct dirent *direntp;
  char path[100];
  int fd;

  if((dirp = opendir("/proc/web100")) == NULL){
      perror("/proc/web100");
      exit(1);
  }
  while((direntp = readdir(dirp)) != NULL){
    if(direntp->d_name[0] == '.' || direntp->d_name[0] == 'h') continue;
    sprintf(path, "%s/%s/%s", "/proc/web100", direntp->d_name, "spec");

    if((fd = open(path, O_RDONLY)) < 0){
      printf("what\n");
      continue; 
    }


    cidlist = malloc(sizeof(struct ProcCid));

    if(lseek(fd, LA_OFF, SEEK_SET) < 0) goto punt;
    if(read(fd, &(cidlist->localadd), 4) != 4) goto punt;
    if(lseek(fd, LP_OFF, SEEK_SET) < 0) goto punt;
    if(read(fd, &(cidlist->localport), 2) != 2) goto punt;
    if(lseek(fd, RA_OFF, SEEK_SET) < 0) goto punt;
    if(read(fd, &(cidlist->remoteadd), 4) != 4) goto punt;
    if(lseek(fd, RP_OFF, SEEK_SET) < 0) goto punt;
    if(read(fd, &(cidlist->remoteport), 2) != 2) goto punt; 

    strncpy(cidlist->cid, direntp->d_name, MAXCIDNAME);

    if(CidList) CidList->previous = cidlist;
    cidlist->next = CidList; 
    cidlist->previous = NULL;
    CidList = cidlist;
    close(fd);
    continue;

punt: close(fd);
      free(cidlist); 
  }
  closedir(dirp); 
}

void fill_tcplist(void)
{
  char buf[256];
  FILE *file;
  struct ProcTcp *tcplist;
  int scan;

  file = fopen("/proc/net/tcp", "r");

  while(fgets(buf, sizeof(buf), file) != NULL){  
    tcplist = malloc(sizeof(struct ProcTcp));

    if((scan = sscanf(buf, "%*u: %x:%hx %x:%hx %x %*x:%*x %*x:%*x %*x %u %*u %u",
	  (u_int32_t *) &(tcplist->localadd),
	  (u_int16_t *) &(tcplist->localport),
	  (u_int32_t *) &(tcplist->remoteadd),
	  (u_int16_t *) &(tcplist->remoteport),
	  (u_int *) &(tcplist->state),
	  (u_int *) &(tcplist->uid),
	  (u_int *) &(tcplist->ino))) == 7){


      if(TcpList) TcpList->previous = tcplist;
      tcplist->next = TcpList; 
      tcplist->previous = NULL;
      TcpList = tcplist;
    }
    else free(tcplist); 
  }
  fclose(file);
}

void fill_fdlist(void)
{
  struct ProcFd *fdlist;
  int cmd;
  DIR *dir, *fddir;
  FILE *file;
  struct dirent *direntp, *fddirentp;
  pid_t pid; 
  char path[128], temp[128];
  char temp2[128];
  struct stat st;
  int stno;

  char buff[100];

  if(!(dir = opendir("/proc")))
    {
      perror("/proc");
      exit(1);
    }
  while((direntp = readdir(dir)) != NULL)
    if((pid = atoi(direntp->d_name)) != 0)
      {
        sprintf(path, "%s/%d/%s/", "/proc", pid, "fd"); 
	if(fddir = opendir(path)) //else lacks permissions 
        while((fddirentp = readdir(fddir)) != NULL) 
        { 
          strcpy(temp, path);
          strcat(temp, fddirentp->d_name); 
          stno = stat(temp, &st); 
          if(S_ISSOCK(st.st_mode)) // new list entry
          { 
	    if((fdlist = malloc(sizeof(struct ProcFd))) == NULL){
	      fprintf(stderr, "Out of memory\n");
	      exit(1);
	    } 

	    fdlist->ino = st.st_ino;
            fdlist->pid = pid;
/*
	    sprintf(temp2, "%s/%d/%s", "/proc", pid, "cmdline");

	    if((cmd = open(temp2, O_RDONLY)) < 0)
	      perror("cmdline");
	    read(cmd, fdlist->cmdline, 100);
	    close(cmd);
*/
	    if(FdList) FdList->previous = fdlist;
	    fdlist->next = FdList;
	    fdlist->previous = NULL;
	    FdList = fdlist;



	    sprintf(temp, "/proc/%d/status", pid);

	    if((file = fopen(temp, "r")) == NULL) continue;

	    if(fgets(temp, sizeof(temp), file) == NULL){
	      fclose(file);
	      continue;
	    }


	    if(sscanf(temp, "Name: %s\n", &(fdlist->cmdline)) != 1){
	      fclose(file);
	      continue;
	    }



	    fclose(file); 

/*	    if(FdList) FdList->previous = fdlist;
	    fdlist->next = FdList;
	    fdlist->previous = NULL;
	    FdList = fdlist;
	    fclose(file);
	    continue;
punt: 
	    free(fdlist); */
          }
        }
	closedir(fddir);
      } 
  closedir(dir);
}

void sort_socklist(int (*compar)(const void *, const void *))
{
  if(compar == NULL) return;
  qsort(SockPuppet, arraylength, sizeof(struct SockData), compar); 
}

void list_socklist(void (*callback)(struct SockData *, void *), void *userdata)
{
  int ii;

  for(ii=0; ii<arraylength; ii++){
    callback(&SockPuppet[ii], userdata);
  }
}

