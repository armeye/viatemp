#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

int msrfd;
uint tempmsr;

enum {
	C7Msr = 0x1169,
	CentMsr = 0x1423
};

int
cpuidcheck(void)
{
	int pfd[2];
	Biobuf *bp;
	char *l, *f[64];
	int i, rc;
	u64int reg;
	uchar family, model;
	
	pipe(pfd);
	switch(rfork(RFPROC|RFFDG)){
	case 0:
		close(pfd[1]);
		close(0);
		open("/dev/null", OREAD);
		dup(pfd[0], 1);
		execl("/bin/aux/cpuid", "cpuid", nil);
		sysfatal("execl: %r");
		break;
	case -1: sysfatal("rfork: %r");
	}
	close(pfd[0]);
	bp = Bfdopen(pfd[1], OREAD);
	if(bp == nil) sysfatal("Bfdopen: %r");
	for(; l = Brdstr(bp, '\n', 10), l != nil; free(l)){
		rc = tokenize(l, f, nelem(f));
		if(rc < 1) continue;
		if(strcmp(f[0], "procmodel") != 0) continue;
		reg = strtoull(f[1], nil, 16);
		model = reg >> 4 & 0xf;
		switch(model){
			case 0xa:
			case 0xd:
				tempmsr = C7Msr;
				break;
			case 0xf:
				tempmsr = CentMsr;
				break;
			default:
				goto out;
		}
		Bterm(bp);
		close(pfd[1]);
		waitpid();
		return 0;
	}
out:
	Bterm(bp);
	close(pfd[1]);
	waitpid();
	return -1;
}

u64int
rdmsr(u32int addr)
{
	u64int rv;

	if(pread(msrfd, &rv, 8, addr) < 0) sysfatal("pread: %r");
	return rv;
}

static void
fsread(Req *r)
{
	u64int msr;
	char msg[8+1];
	
	*msg = 0;
	msr = rdmsr(tempmsr);

	snprint(msg, sizeof(msg), "%ulld.0\n", msr);

	readstr(r, msg);
	respond(r, nil);
}

static Srv fs = {
	.read = fsread,
};

void
threadmain(int argc, char **argv)
{
	msrfd = open("#P/msr", ORDWR);

	if(cpuidcheck() == -1)
		sysfatal("cpu not supported");

	fs.tree = alloctree(getuser(), getuser(), DMDIR|0555, nil);
	createfile(fs.tree->root, "cputemp", getuser(), 0444, nil);
	threadpostmountsrv(&fs, nil, "/dev", MAFTER);
	threadexits(0);
}
