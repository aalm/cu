/* $OpenBSD: command.c,v 1.17 2019/03/22 07:03:23 nicm Exp $ */

/*
 * Copyright (c) 2012 Nicholas Marriott <nicm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <event.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "compat.h"
#include "cu.h"

void	pipe_command(void);
void	connect_command(void);
void	send_file(void);
void	send_xmodem(void);
void	set_speed(void);
void	start_record(void);

void
pipe_command(void)
{
	const char	*cmd;
	pid_t		 pid;
	int		 fd;

	cmd = get_input("Local command?");
	if (cmd == NULL || *cmd == '\0')
		return;

	restore_termios();
	set_blocking(line_fd, 1);

	switch (pid = fork()) {
	case -1:
		cu_err(1, "fork");
		return;
	case 0:
		fd = open(_PATH_DEVNULL, O_RDWR);
		if (fd < 0 || dup2(fd, STDIN_FILENO) == -1)
			_exit(1);
		close(fd);

		if (signal(SIGINT, SIG_DFL) == SIG_ERR)
			_exit(1);
		if (signal(SIGQUIT, SIG_DFL) == SIG_ERR)
			_exit(1);

		/* attach stdout to line */
		if (dup2(line_fd, STDOUT_FILENO) == -1)
			_exit(1);

#if 0
		if (closefrom(STDERR_FILENO + 1) != 0)
			_exit(1);
#else
		closefrom(STDERR_FILENO + 1);
#endif

		execl(_PATH_BSHELL, "sh", "-c", cmd, (char *)NULL);
		_exit(1);
	default:
		while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
			/* nothing */;
		break;
	}

	set_blocking(line_fd, 0);
	set_termios();
}

void
connect_command(void)
{
	const char	*cmd;
	pid_t		 pid;

	/*
	 * Fork a program with:
	 *  0 <-> remote tty in
	 *  1 <-> remote tty out
	 *  2 <-> local tty stderr
	 */

	cmd = get_input("Local command?");
	if (cmd == NULL || *cmd == '\0')
		return;

	restore_termios();
	set_blocking(line_fd, 1);

	switch (pid = fork()) {
	case -1:
		cu_err(1, "fork");
		return;
	case 0:
		if (signal(SIGINT, SIG_DFL) == SIG_ERR)
			_exit(1);
		if (signal(SIGQUIT, SIG_DFL) == SIG_ERR)
			_exit(1);

		/* attach stdout and stdin to line */
		if (dup2(line_fd, STDOUT_FILENO) == -1)
			_exit(1);
		if (dup2(line_fd, STDIN_FILENO) == -1)
			_exit(1);
#if 0
		if (closefrom(STDERR_FILENO + 1) != 0)
			_exit(1);
#else
		closefrom(STDERR_FILENO + 1);
#endif
		execl(_PATH_BSHELL, "sh", "-c", cmd, (char *)NULL);
		_exit(1);
	default:
		while (waitpid(pid, NULL, 0) == -1 && errno == EINTR)
			/* nothing */;
		break;
	}

	set_blocking(line_fd, 0);
	set_termios();
}

void
send_file(void)
{
	const char	*file;
	FILE		*f;
	char		 buf[BUFSIZ], *expanded;
	size_t		 len;

	file = get_input("Local file?");
	if (file == NULL || *file == '\0')
		return;

	expanded = tilde_expand(file);
	f = fopen(expanded, "r");
	if (f == NULL) {
		cu_warn("%s", file);
		return;
	}

	while (!feof(f) && !ferror(f)) {
		len = fread(buf, 1, sizeof(buf), f);
		if (len != 0)
			bufferevent_write(line_ev, buf, len);
	}

	fclose(f);
	free(expanded);
}

void
send_xmodem(void)
{
	const char	*file;
	char		*expanded;

	file = get_input("Local file?");
	if (file == NULL || *file == '\0')
		return;

	expanded = tilde_expand(file);
	xmodem_send(expanded);
	free(expanded);
}

void
set_speed(void)
{
	const char	*s, *errstr;
	int		 speed;

	s = get_input("New speed?");
	if (s == NULL || *s == '\0')
		return;

	speed = strtonum(s, 0, UINT_MAX, &errstr);
	if (errstr != NULL) {
		cu_warnx("speed is %s: %s", errstr, s);
		return;
	}

	if (set_line(speed) != 0)
		cu_warn("tcsetattr");
}

void
start_record(void)
{
	const char	*file;

	if (record_file != NULL) {
		fclose(record_file);
		record_file = NULL;
	}

	file = get_input("Record file?");
	if (file == NULL || *file == '\0')
		return;

	record_file = fopen(file, "a");
	if (record_file == NULL)
		cu_warnx("%s", file);
}

#if 1
#else
/*
 * Drop DTR line and raise it again.
 */
void m_dtrtoggle(int fd, int sec)
{
#ifdef TIOCSDTR
    /* Use the ioctls meant for this type of thing. */
    ioctl(fd, TIOCCDTR, 0);
    sleep(sec);
    ioctl(fd, TIOCSDTR, 0);
#elif defined (TIOCMBIS) && defined (TIOCMBIC) && defined (TIOCM_DTR)
    int modembits = TIOCM_DTR;
    ioctl(fd, TIOCMBIC, &modembits);
    sleep (sec);
    ioctl(fd, TIOCMBIS, &modembits);
#endif
#if 1
#else /* TIOCSDTR */
#  if defined (POSIX_TERMIOS)
    /* Posix - set baudrate to 0 and back */
    struct termios tty, old;
    tcgetattr(fd, &tty);
    tcgetattr(fd, &old);
    cfsetospeed(&tty, B0);
    cfsetispeed(&tty, B0);
    tcsetattr(fd, TCSANOW, &tty);
    sleep(sec);
    tcsetattr(fd, TCSANOW, &old);
#  else /* POSIX */
#    ifdef _V7
    /* Just drop speed to 0 and back to normal again */
    struct sgttyb sg, ng;
    ioctl(fd, TIOCGETP, &sg);
    ioctl(fd, TIOCGETP, &ng);
    ng.sg_ispeed = ng.sg_ospeed = 0;
    ioctl(fd, TIOCSETP, &ng);
    sleep(sec);
    ioctl(fd, TIOCSETP, &sg);
#    endif /* _V7 */
#  endif /* POSIX */
#endif /* TIOCSDTR */
}
#endif

void
do_command(char c)
{
	char esc[4 + 1];

	if (restricted && strchr("CRX$>", c) != NULL) {
		cu_warnx("~%c command is not allowed in restricted mode", c);
		return;
	}

	switch (c) {
	case '.':
	case '\004': /* ^D */
		event_loopexit(NULL);
		break;
	case '\032': /* ^Z */
		restore_termios();
		kill(getpid(), SIGTSTP);
		set_termios();
		break;
	case 'C':
		connect_command();
		break;
	case 'D':
#if 0
		ioctl(line_fd, TIOCCDTR, NULL);
		sleep(1);
		ioctl(line_fd, TIOCSDTR, NULL);
#else
#ifdef TIOCSDTR
		/* Use the ioctls meant for this type of thing. */
		ioctl(line_fd, TIOCCDTR, 0);
		sleep(1);
		ioctl(line_fd, TIOCSDTR, 0);
#elif defined (TIOCMBIS) && defined (TIOCMBIC) && defined (TIOCM_DTR)
	{
		int modembits = TIOCM_DTR;
		ioctl(line_fd, TIOCMBIC, &modembits);
		sleep(1);
		ioctl(line_fd, TIOCMBIS, &modembits);
	}
#endif
#endif
		break;
	case 'R':
		start_record();
		break;
	case 'S':
		set_speed();
		break;
	case 'X':
		send_xmodem();
		break;
	case '$':
		pipe_command();
		break;
	case '>':
		send_file();
		break;
	case '#':
		ioctl(line_fd, TIOCSBRK, NULL);
		sleep(1);
		ioctl(line_fd, TIOCCBRK, NULL);
		break;
	default:
		if ((u_char)c == escape_char)
			bufferevent_write(line_ev, &c, 1);
		break;
	case '?':
		vis(esc, escape_char, VIS_WHITE | VIS_NOSLASH, 0);
		printf("\r\n"
		    "%s#      send break\r\n"
		    "%s$      pipe local command to remote host\r\n"
		    "%s>      send file to remote host\r\n"
		    "%sC      connect program to remote host\r\n"
		    "%sD      de-assert DTR line briefly\r\n"
		    "%sR      start recording to file\r\n"
		    "%sS      set speed\r\n"
		    "%sX      send file with XMODEM\r\n"
		    "%s?      get this summary\r\n",
		    esc, esc, esc, esc, esc, esc, esc, esc, esc
		);
		break;
	}
}
