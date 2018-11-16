#include "strm.h"

/* TODO: Use sensible values. */
#define CMD_MAX 100

static int
proc_spawn(strm_stream* strm, int argc, strm_value* args, strm_value* ret)
{
	const char *strcmd;
	strm_string cmd;
	char buf[CMD_MAX];
	FILE *fp;
	int fd;
	char *mode;
	mode = "r+";
	int strm_mode;

	strm_get_args(strm, argc, args, "S", &cmd);
	strcmd = strm_str_cstr(cmd, buf);

	fp = popen(strcmd, mode);
	if (fp == NULL) {
		strm_raise(strm, "spawn error - popen.");
		return STRM_NG;
	}

	fd = fileno(fp);
	if (fd < 0) return STRM_NG;

	strm_mode = 2; //<- TODO: Use stream's flags.
	*ret = strm_io_new(fd, strm_mode);
	return STRM_OK;
}

void
strm_proc_init(strm_state* state)
{
	// spawn("ls *") | stdout # producer.
	strm_var_def(state, "spawn", strm_cfunc_value(proc_spawn));

}
