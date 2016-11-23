#ifndef MTVERSION_H
#define MTVERSION_H
#if !defined(__GNUC__)
#error Unsupported compiler!
#endif /* __GNUC__ */
#include <stdint.h>

#define likely(x) \
  __builtin_expect(!!(x), 1)
#define unlikely(x) \
  __builtin_expect(!!(x), 0)

#define MTV_VERSION     "0.0.2"

#define MTV_DIRECTORY   "/etc/mtversion"
#define MTV_FILE_LOG    "log_mtv.log"

#define MTV_TYPE_DEV    1
#define MTV_TYPE_REL    2

typedef enum {
  ERROR,
  WARN,
  INFO
} log_level_t;

typedef struct {
  uint16_t id;
  uint8_t  type;
  uint8_t  mjver;
  uint16_t mnver;
  uint16_t rlver;
} mtv_netmsg_t;

typedef struct {
  uint32_t first;
  uint32_t second;
  uint32_t third;
  uint32_t fourth;
} mtv_version_t;

typedef struct {
  const char *pidfile;
  const char *port;
  int sockfd;
  int pidfd;
} mtv_opt_t;




void mtv_init(int argc, char *argv[], mtv_opt_t *options);
void mtv_deinit(void);
int mtv_logger(log_level_t lvl, const char *msg, ...);
const mtv_opt_t* mtv_get_options(void);


#endif // MTVERSION_H
