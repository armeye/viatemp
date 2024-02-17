#include <u.h>
#include <ureg.h>
#include <libc.h>

enum {
	ShaBufSZ = IOUNIT - 64,
};

void viasha(uchar *, uint, u32int *);

uchar pad[128];

#define SWAP(x) (((x) >> 24) & 0x000000ff) |  (((x) >> 8) & 0x0000ff00) | \
	(((x) << 8) & 0x00ff0000) | (((x) << 24) & 0xff000000)


static int
digestfmt(Fmt *fmt)
{
	char buf[40+1];
	uchar *p;
	int i;

	p = va_arg(fmt->args, uchar*);
	for(i = 0; i < 20; i++)
		sprint(buf + 2*i, "%.2ux", p[i]);
	return fmtstrcpy(fmt, buf);
}

int handlefault(void *rr, char *msg)
{
	struct Ureg *r = rr;

	if(strstr(msg, "sys: trap: fault")){
		r->pc+=4;
		return 1;
	}
	return 0;
}

void main(int argc, char **argv)
{
	int fd, n, s, l, pl;
	uchar *v;
	u32int *state;
	u64int total = 0;
	u64int tbe = 0;

	pad[0] = 0x80;

	state = mallocalign(128, 16, 0, 0);
	memset(state, 0, 128);
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;
	state[4] = 0xc3d2e1f0;

	v = segattach(0, "memory", (void *)0xde000000, IOUNIT);

	fmtinstall('M', digestfmt);

	fd = open(argv[1], OREAD);

	while((n = read(fd, v, ShaBufSZ)) > 0){
		if(n == ShaBufSZ){
			atnotify(handlefault, 1);
			viasha(v, IOUNIT, state);
			atnotify(handlefault, 0);
			total+=n;
		}
		else
			break;
	}

	s = ShaBufSZ - n;
	
	total+=n;
	l = total & 63;
	total*=8;
	pl = (l < 56) ? (56 - l) : (120 - l);

	s = ShaBufSZ - (n+pl+8);

	memmove(v+s, v, n);
	memmove(v+s+n, pad, pl);

	((u32int *)&tbe)[1] = SWAP(((u32int *)&total)[0]);
	((u32int *)&tbe)[0] = SWAP(((u32int *)&total)[1]);

	memmove(v+s+n+pl, &tbe, 8);
	atnotify(handlefault, 1);
	viasha(v+s, n+pl+8+64, state);
	atnotify(handlefault, 0);

	state[0] = SWAP(state[0]);
	state[1] = SWAP(state[1]);
	state[2] = SWAP(state[2]);
	state[3] = SWAP(state[3]);
	state[4] = SWAP(state[4]);
	
	print("%M\n", state);

	exits(0);
}
