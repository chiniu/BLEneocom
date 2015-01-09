/*
 * neocon.c - An interface for changing tty devices
 *
 * Copyright (C) 2007, 2008 by OpenMoko, Inc.
 * Written by Werner Almesberger <werner@openmoko.org>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>


#define MAX_BUF 2048
#define BT_BUF 20

static char *const *ttys;
static int num_ttys;
static speed_t speed = B115200;
static struct termios console, tty;
static FILE *log = NULL;
static int timestamp = 0;
static char escape = '~';


static struct bps {
    speed_t speed;
    int bps;
} bps_tab[] = {
    { B300,       300 },
    { B1200,     1200 },
    { B2400,     2400 },
    { B9600,     9600 },
    { B19200,   19200 },
    { B38400,   38400 },
    { B115200, 115200 },
    { B460800, 460800 },
    { 0, 0 }
};


static speed_t bps_to_speed(int bps)
{
    const struct bps *p;

    for (p = bps_tab; p->bps; p++)
	if (p->bps == bps)
	    return p->speed;
    fprintf(stderr, "no such speed: %d bps\n", bps);
    exit(1);
}

int setRTS(int fd, int level)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1) {
        perror("setRTS(): TIOCMGET");
        return 0;
    }
    if (level)
        status |= TIOCM_RTS;
    else
        status &= ~TIOCM_RTS;

    if (ioctl(fd, TIOCMSET, &status) == -1) {
        perror("setRTS(): TIOCMSET");
        return 0;
    }
    if (ioctl(fd, TIOCMGET, &status) == -1) {
        perror("setRTS(): TIOCMGET");
        return 0;
    }

    return 1;
}


int setDTR(int fd, int level)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1) {
        perror("setDTR(): TIOCMGET");
        return 0;
    }
    if (level)
        status &= ~TIOCM_DTR;
    else
        status |= TIOCM_DTR;

    if (ioctl(fd, TIOCMSET, &status) == -1) {
        perror("setDTR(): TIOCMSET");
        return 0;
    }
    if (ioctl(fd, TIOCMGET, &status) == -1) {
        perror("setDTR(): TIOCMGET");
        return 0;
    }

    return 1;
}


static void make_raw(int fd, struct termios *old)
{
    struct termios t;
    long flags;

    if (tcgetattr(fd, &t) < 0) {
	perror("tcgetattr");
	exit(1);
    }
    if (old)
	*old = t;
    cfmakeraw(&t);
    if (fd) {
	t.c_iflag  &= ~(IXON | IXOFF);
	t.c_cflag |= CLOCAL;
	t.c_cflag &= ~CRTSCTS;

	if (cfsetispeed(&t, speed) < 0) {
	    perror("cfsetispeed");
	    exit(1);
	}
	if (cfsetospeed(&t, speed) < 0) {
	    perror("cfsetospeed");
	    exit(1);
	}
    }
    if (tcsetattr(fd, TCSANOW, &t) < 0) {
	perror("tcsetattr");
	exit(1);
    }
    flags = fcntl(fd,F_GETFL);
    if (flags < 0) {
	perror("fcntl F_GETFL");
	exit(1);
    }
    if (fcntl(fd,F_SETFL,flags & ~O_NONBLOCK) < 0) {
	perror("fcntl F_GETFL");
	exit(1);
    }
}


static int open_first_tty(void)
{
    int i, fd = -1;

    for (i = 0; i != num_ttys; i++) {
	fd = open(ttys[i], O_RDWR | O_NDELAY);
	if (fd >= 0)
	    break;
    }
    if (fd >= 0)
	make_raw(fd, &tty);
    return fd;
}

static void write_string(const char *s)
{
    int len = strlen(s);

    while (len) {
	ssize_t wrote;

	wrote = write(1, s, len);
	if (wrote < 0) {
	    perror("write");
	    exit(1);
	}
	s += wrote;
	len -= wrote;
    }
}



static int write_log(const char *buf, ssize_t len)
{
    size_t wrote;

    wrote = fwrite(buf, 1, len, log);
    if (wrote == len)
	return 1;
    fprintf(stderr, "write failed. closing log file.\n");
    fclose(log);
    log = NULL;
    return 0;
}

static int scan(const char *s, size_t len)
{
    static int state = 0;
    size_t i;


    for (i = 0; i != len; i++){
        if(s[i] == 0xD){
            return 1;
        }
	switch (state) {
	    case 0:
		if (s[i] == escape)
		    state++;
		else
		    state = 0;
		break;
	    case 1:
		if (s[i] == '.')
		    exit(0);
		state = 0;
		break;
	}
    }
    return 0;
}



static int add_timestamp(void)
{
    struct timeval tv;
    char buf[40]; /* be generous */
    int len;

    if (gettimeofday(&tv, NULL) < 0) {
	perror("gettimeofday");
	exit(1);
    }
    len = sprintf(buf, "%lu.%06lu ",
      (unsigned long) tv.tv_sec, (unsigned long) tv.tv_usec);
    return write_log(buf, len);
}


static void do_log(const char *buf, ssize_t len)
{
    static int nl = 1; /* we're at the beginning of a new line */
    char tmp[MAX_BUF];
    const char *from;
    char *to;

    assert(len <= MAX_BUF);
    from = buf;
    to = tmp;
    while (from != buf+len) {
	if (*from == '\r') {
	    from++;
	    continue;
	}
	if (nl && timestamp)
	    if (!add_timestamp())
		return;
	nl = 0;
	if (*from == '\n') {
	    *to++ = *from++;
	    if (!write_log(tmp, to-tmp))
		return;
	    to = tmp;
	    nl = 1;
	    continue;
	}
	*to++ = *from < ' ' || *from > '~' ? '#' : *from;
	from++;
    }
    write_log(tmp, to-tmp);
}

static int sendBT(int out,  char *cmd, int len)
{
    ssize_t  wrote, pos, total;

    total = 0;

    write_string("Send: ");
    write_string(cmd);
 
    for (pos = 0; pos != len; pos += wrote) {
        setRTS(out, 0);
        usleep(2);
	wrote = write(out, cmd+pos, len-pos);
        total += wrote;
        usleep(5);
        setRTS(out, 1);

	if (wrote < 0)
	    return 0;
    }
    return wrote;
}

static char btbuf[BT_BUF];
static int btidx = 0;

static int bufcopy(int in, int out, int single)
{
    char buffer[MAX_BUF];
    ssize_t got, pos, wrote;
    int send = 0;
 

/*    printf("btidx %d\n", btidx); */
    if(btidx == 0)
        got = read(in, buffer, single ? 1 : sizeof(buffer));
    else{
        memcpy(buffer, btbuf, btidx);
        got = read(in, buffer+btidx, single ? 1 : sizeof(buffer));
    } 
    got += btidx;
    btidx = got;

    if (got < 0)
	return 0;
    send = scan(buffer, got);

    if(got >= BT_BUF)
        send = 1;

/*    printf("send %d\n", send); */

    if(send){
        for (pos = 0; pos != got; pos += wrote) {
            if(got >= (pos + BT_BUF))
                wrote = sendBT(out, buffer+pos, BT_BUF);
            else
                wrote = sendBT(out, buffer+pos, got-pos);
            if (wrote <= 0)
	        return 0;
        }
        btidx = 0;
    }else{
        memcpy(btbuf, buffer, got);
    }
    return 1;
}


static int copy(int in, int out, int from_user, int single)
{
    char buffer[MAX_BUF];
    ssize_t got, wrote, pos;
 
    got = read(in, buffer, single ? 1 : sizeof(buffer));
    if (got < 0)
	return 0;
    if (from_user) 
	scan(buffer, got);
    else{
	if (log)
	    do_log(buffer, got);
    }
    for (pos = 0; pos != got; pos += wrote) {
        if(from_user) {
            setRTS(out, 0);
            usleep(2);
        }
	wrote = write(out, buffer+pos, got-pos);
        if(from_user) {
            usleep(5);
            setRTS(out, 1);
        }

	if (wrote < 0)
	    return 0;
    }
    return 1;
}



static void cleanup(void)
{
    if (tcsetattr(0, TCSANOW, &console) < 0)
	perror("tcsetattr");
}


static void usage(const char *name)
{
    fprintf(stderr,
"usage: %s [-b bps] [-e escape] [-l logfile [-a] [-T]] [-t delay_ms] tty ...\n\n"
"  -a           append to the log file if it already exists\n"
"  -b bps       set the TTY to the specified bit rate\n"
"  -e escape    set the escape character (default: ~)\n"
"  -l logfile   log all output to the specified file\n"
"  -t delay_ms  wait the specified amount of time between input characters\n"
"  -T           add timestamps to the log file\n"
      , name);
    exit(1);
}


int main(int argc, char *const *argv)
{
    char *end;
    int c, bps;
    int fd = -1;
    int append = 0;
    const char *logfile = NULL;
    int throttle_us = 0;
    int throttle = 0;

    while ((c = getopt(argc, argv, "ab:e:l:t:T")) != EOF)
	switch (c) {
	    case 'a':
		append = 1;
		break;
	    case 'b':
		bps = strtoul(optarg, &end, 0);
		if (*end)
		    usage(*argv);
		speed = bps_to_speed(bps);
		break;
	    case 'e':
		if (strlen(optarg) != 1)
		    usage(*argv);
		escape = *optarg;
		break;
	    case 'l':
		logfile = optarg;
		break;
	    case 't':
		throttle_us = strtoul(optarg, &end, 0)*1000;
		if (*end)
		    usage(*argv);
		break;
	    case 'T':
		timestamp = 1;
		break;
	    default:
		usage(*argv);
	}
    num_ttys = argc-optind;
    ttys = argv+optind;

    if (logfile) {
	log = fopen(logfile, append ? "a" : "w");
	if (!log) {
	    perror(logfile);
	    exit(1);
	}
	setlinebuf(log);
    }

    make_raw(0, &console);
    atexit(cleanup);
    while (1) {
	struct timeval tv;
	fd_set set;
	int res;

	if (fd < 0) {
	    fd = open_first_tty();
	    if (fd > 0) {
		write_string("\r\n[Open]\r\n");
                setDTR(fd, 1);
                setRTS(fd, 1);
            }

	}
	FD_ZERO(&set);
	if (!throttle)
	    FD_SET(0, &set);
	if (fd >= 0)
	    FD_SET(fd, &set);
	tv.tv_sec = 0;
	tv.tv_usec = throttle ? throttle_us : 100000;
#if 1
	res = select(fd < 0 ? 1 : fd+1, &set, NULL, NULL, &tv);
#else
	res = select(fd < 0 ? 1 : fd+1, &set, NULL, NULL, NULL);
#endif
	if (res < 0) {
	    perror("select");
	    return 1;
	}
	if (!res)
	    throttle = 0;
	if (FD_ISSET(0, &set)) {
	    if (throttle_us)
		throttle = 1;
	    if (!bufcopy(0, fd,  throttle_us != 0))
		goto failed;
	}
	if (fd >= 0 && FD_ISSET(fd, &set))
	    if (!copy(fd, 1, 0, 0))
		goto failed;

	continue;

failed:
	write_string("\r\n[Closed]\r\n");
	(void) close(fd);
	fd = -1;
    }
    return 0;
}
