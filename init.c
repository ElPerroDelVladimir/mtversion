#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "mtversion.h"


static void __attribute__((noreturn)) mtv_usage(const char *name)
{
  printf("MT version handler server. Version %s\n", MTV_VERSION);
  printf("Usage: %s [-p port] [-l pid_file]\n", name);
  exit(EXIT_FAILURE);
}

static void mtv_signal_handler(int signum)
{
  switch(signum) {
    case SIGINT:
      shutdown(mtv_get_options()->sockfd, 0);
      break;
  }
}


void mtv_init(int argc, char *argv[], mtv_opt_t *options)
{
  int opt;
  char pidbuf[16];
  struct sockaddr_in serv_addr = {0};

  memset(options, 0, sizeof(*options));

  while((opt = getopt(argc, argv, "l:p:h")) != -1) {
    switch(opt) {
      case 'l':
        options->pidfile = strdup(optarg);
        break;
      case 'p':
        options->port = strdup(optarg);
        break;
      case 'h':
        /* Fall through */
      case '?':
        mtv_usage(argv[0]);
    }
  }

  if(options->pidfile == NULL || options->port == NULL) {
    fprintf(stderr, "Error: some parameters are missing\n");
    mtv_logger(ERROR, "some parameters are missing. Could not start the daemon");
    mtv_usage(argv[0]);
  }

  if(daemon(1, 0) == -1) {
    perror("daemonize failed");
    mtv_logger(ERROR, "start as daemon failed: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  umask(022);

  if(chdir(MTV_DIRECTORY) == -1) {
   mtv_logger(ERROR, "chdir failed: %s", strerror(errno));
   exit(EXIT_FAILURE);
  }

  if(atexit(mtv_deinit)) {
    mtv_logger(ERROR, "cannot register atexit function: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(signal(SIGINT, mtv_signal_handler) == SIG_ERR) {
    mtv_logger(ERROR, "cannot register signal handler function: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  options->pidfd = open(options->pidfile, O_CREAT | O_WRONLY | O_TRUNC | O_DSYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
  if(options->pidfd == -1) {
    mtv_logger(ERROR, "cannot create pid file: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  sprintf(pidbuf, "%d", getpid());
  if(write(options->pidfd, pidbuf, strlen(pidbuf)) == -1) {
    mtv_logger(ERROR, "write to pid file failed: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  int port = atoi(options->port);
  if(port < 0 || port > 65535) {
    mtv_logger(WARN, "invalid port value. Will use by default (31337)");
    port = 31337;
  }

  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_family = PF_INET;
  serv_addr.sin_port = htons(port);

  options->sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if(options->sockfd == -1) {
    mtv_logger(ERROR, "create socket failed: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(bind(options->sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
    mtv_logger(ERROR, "bind failed: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(listen(options->sockfd, 10) == -1) {
    mtv_logger(ERROR, "listen failed: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }



  mtv_logger(INFO, "server started as daemon");

}


void mtv_deinit(void)
{
  const mtv_opt_t *opt = mtv_get_options();

  close(opt->sockfd);
  close(opt->pidfd);
  unlink(opt->pidfile);
  free((char*)opt->pidfile);
  free((char*)opt->port);

  mtv_logger(INFO, "daemon stopped");
}
