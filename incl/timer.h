#include "type.h"

int getIntervalFromNowNew(const struct tm * timeStamp);
void timer_getSC_cb (uv_timer_t* timer);
void timer_getFile_cb (uv_timer_t* timer);
void timer_init(server_state_t *state);
void fs_fstat_cb(uv_fs_t *req);
void fs_read_cb(uv_fs_t *req);
void fs_close_cb(uv_fs_t *req);
void fs_open_cb(uv_fs_t *req);
