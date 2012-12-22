#include <stddef.h>
#include <unistd.h>

size_t ngx_pagesize_shift = 12;
size_t ngx_pagesize = (1 << 12);

void ngx_os_init() {
	size_t i, shift = 0;
	ngx_pagesize = sysconf(_SC_PAGESIZE);
	for (i = ngx_pagesize; i >>= 1; ++shift) {/*void*/}
	ngx_pagesize_shift = shift;
}

