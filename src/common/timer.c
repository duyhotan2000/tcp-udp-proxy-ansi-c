#include "timer.h"
#if defined(__MACH__)
#include <stdlib.h>
#else 
#include <malloc.h>
#endif
#include <unistd.h>
#include "util.h"

unsigned int isFileExist = 0;
extern int sv_global_state;

void timer_init(server_state_t *state)
{
	uv_timer_init(state->loop, &state->timerSC);
	uv_timer_init(state->loop, &state->timerGetFile);
	state->indexTimeGetFile = 0;
	state->indexTimeGetSC = 0;

	unsigned int i;
	for(i=0; i < state->config.ntimeGetFile; i++)
	{
		if (getIntervalFromNowNew(&state->config.timeGetFile[i]) >= 0 )
		{
			state->indexTimeGetFile = i;
			state->timerGetFile.data = state;
			uv_timer_start(&state->timerGetFile, timer_getFile_cb,
					   getIntervalFromNowNew(&state->config.timeGetFile[i])*1000, 0);
			break;
		}
	}

	for(i=0; i < state->config.ntimeGetSC; i++)
	{
		if (getIntervalFromNowNew(&state->config.timeGetSC[i]) >= 0 )
		{
			state->indexTimeGetSC = i;
			state->timerSC.data = state;
			uv_timer_start(&state->timerSC, timer_getSC_cb,
					   getIntervalFromNowNew(&state->config.timeGetSC[i])*1000 , 0);
			break;
		}
	}
}

int getIntervalFromNowNew(const struct tm * timeStamp)
{
    time_t rawtime = time(NULL);
    struct tm *now = malloc(sizeof(struct tm));
    now = localtime(&rawtime);

    int hour ;
    int minute ;
    time_t end, start;
    double diff;
    size_t seconds;

    start = (time_t)((now->tm_hour * 60 + now->tm_min) * 60) ;
    end = (time_t)((timeStamp->tm_hour * 60 + timeStamp->tm_min) * 60) ;

    if( end < start ){
        end += 24 * 60 * 60 ;
    }

    diff = difftime(end, start);

    hour = (int) diff / 3600;
    minute = (int) diff / 60;
    seconds = (size_t)(hour * 3600) + (size_t)(minute * 60) - now->tm_sec;

    return seconds;
}

void timer_getSC_cb (uv_timer_t* timer) {
	logger_info("Get SC Message!\n");

	if(isFileExist == 1){
		sv_global_state = ss_parse;
	}

	isFileExist = 0;

	server_state_t *state = timer->data;
	if(state->indexTimeGetSC == state->config.ntimeGetSC - 1)
	{
		state->indexTimeGetSC = 0;
	}
	else
	{
		state->indexTimeGetSC += 1;
	}
	unsigned int index = state->indexTimeGetSC;
	uv_timer_start(&state->timerSC, timer_getSC_cb,
			   getIntervalFromNowNew(&state->config.timeGetSC[index])*1000, 0);

	//uv_timer_stop(timer);
}

void timer_getFile_cb (uv_timer_t* timer) {
	server_state_t *state = timer->data;

	if( access( state->config.filePath, F_OK ) != -1 ) {
		// file exists
		logger_info("Time to get file\n");
	}

//    uv_timer_stop(timer);
	uv_fs_open(state->loop, &state->fs_req, state->config.filePath, O_RDONLY, 0, fs_open_cb);

    if(state->indexTimeGetFile == state->config.ntimeGetFile - 1)
    	state->indexTimeGetFile = 0;
    else
    	state->indexTimeGetFile += 1;

    unsigned int index = state->indexTimeGetFile;
    uv_timer_start(timer, timer_getFile_cb, getIntervalFromNowNew(&state->config.timeGetFile[index])*1000, 0);
}

void fs_open_cb(uv_fs_t *req)
{
	if (req->result >= 0) {
		server_state_t *state = CONTAINER_OF(req, server_state_t, fs_req);
		state->fs_req.data = (void*)req->result;
		uv_fs_fstat(state->loop, req, (uv_file)req->result, fs_fstat_cb);
	}
	else {
		logger_error("Error opening file: %s\n", uv_strerror((int)req->result));
	}
}

void fs_fstat_cb(uv_fs_t *req)
{
	if(req->result)
	{
		logger_error("Error opening file: %s\n", uv_strerror((int)req->result));
		return;
	}
	server_state_t *state = CONTAINER_OF(req, server_state_t, fs_req);
	ssize_t file_size = req->statbuf.st_size;
	state->fileBuf = (char*) xmalloc(file_size);
	state->fileLen = file_size;
	uv_buf_t buf = uv_buf_init(state->fileBuf, state->fileLen);
	uv_fs_read(state->loop, req, (uv_file)(uintptr_t)req->data,
			   &buf, 1, -1, fs_read_cb);
}

void fs_read_cb(uv_fs_t *req)
{
    if (req->result < 0) {
    	logger_error("Read error: %s\n", uv_strerror(req->result));
    }
    else {
		isFileExist = 1;
		server_state_t *state = CONTAINER_OF(req, server_state_t, fs_req);
        uv_fs_close(state->loop, req, (uv_file)(uintptr_t)req->data, fs_close_cb);
    }
}

void fs_close_cb(uv_fs_t *req)
{
    if (req->result < 0) {
    	logger_error("File close error: %s\n", uv_strerror(req->result));
    }
    else if (req->result == 0) {
    	logger_info("File closed\n");
    }
}

