#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "mtversion.h"

static mtv_opt_t g_options;

static inline void mtv_increase_version(const char *path, mtv_version_t *ver)
{
  FILE *f;

  f = fopen(path, "r+");
  if(likely(f)) {
    if(fread(ver, sizeof(*ver), 1, f) != 0) {
      rewind(f);
      ver->fourth++;
      fwrite(ver, sizeof(*ver), 1, f);
      fclose(f);
      return;
    }
  }
  fclose(f);
  memset(ver, 0, sizeof(*ver));
}


static inline const mtv_version_t* mtv_get_version(const mtv_netmsg_t *request)
{
  int rc;
  uint32_t startp;
  const char *type;
  char filename[64];
  static mtv_version_t version;

  switch(request->type) {
    case MTV_TYPE_DEV:
      type = "develop.mt";
      startp = 5000;
      break;
    case MTV_TYPE_REL:
      type = "release.mt";
      startp = 1;
      break;
    default:
      memset(&version, 0, sizeof(version));
      return &version;
  }
  sprintf(filename, "%hhu.%hu.%hu.%s", request->mjver, request->mnver, request->rlver, type);

  rc = access(filename, F_OK);
  if(rc == 0) {
    mtv_increase_version(filename, &version);
  } else if(rc == -1) {
    version.first = request->mjver;
    version.second = request->mnver;
    version.third = request->rlver;
    version.fourth = startp;
    FILE *f = fopen(filename, "w");
    if(likely(f)) {
      fwrite(&version, sizeof(version), 1, f);
      fclose(f);
    } else {
      memset(&version, 0, sizeof(version));
    }
  }
  return &version;
}


int main(int argc, char *argv[])
{
  int sock;
  const mtv_version_t *version;
  struct sockaddr_in peer = {0};
  socklen_t peerlen = sizeof(peer);
  char __attribute__((aligned(8))) iobuf[64];

  mtv_init(argc, argv, &g_options);

  while((sock = accept(g_options.sockfd, (struct sockaddr*)&peer, &peerlen)) != -1) {
    ssize_t recvbytes, accum = 0;

/*
    char *peerip = inet_ntoa(peer.sin_addr);
    mtv_logger(INFO, "connected: %s", peerip);
*/

    do {
      recvbytes = recv(sock, iobuf + accum, sizeof(iobuf) - accum, 0);
      if(unlikely(recvbytes == -1)) {
        if(errno == EINTR) {
          continue;
        }
        mtv_logger(ERROR, "recv error: %s", strerror(errno));
        goto CLOSECONN;
      }
      if(unlikely(recvbytes == 0)) {
        mtv_logger(WARN, "connection closed. incomplite message recievied @ %s", inet_ntoa(peer.sin_addr));
        goto CLOSECONN;
      }
      accum += recvbytes;
    } while(accum < sizeof(mtv_netmsg_t));

    version = mtv_get_version((mtv_netmsg_t*)iobuf);
    if(send(sock, version, sizeof(*version), 0) == -1) {
      mtv_logger(ERROR, "send error: %s", strerror(errno));
    }
CLOSECONN:
    close(sock);
    peerlen = sizeof(peer);
  }

  return 0;
}



const mtv_opt_t* mtv_get_options(void)
{
  return &g_options;
}
