#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>


#include <linux/ion.h>
#include <linux/asoc_ion.h>

#define ALIGN_BYTES         4096
#define ALIGN_MASK          (ALIGN_BYTES - 1)

struct actal_mem  {
	int fd;
	struct ion_handle * handle;
	int len;
	void *ptr;
	int map_fd;
	int phy_add;
	struct actal_mem *next;
};



int ion_open()
{
        int fd = open("/dev/ion", O_RDWR);
        if (fd < 0)
                printf("open /dev/ion failed!\n");
        return fd;
}

int ion_close(int fd)
{
        return close(fd);
}

static int ion_ioctl(int fd, int req, void *arg)
{
        int ret = ioctl(fd, req, arg);
        if (ret < 0) {
                printf("ioctl %x failed with code %d: %s\n", req,
                       ret, strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_alloc(int fd, size_t len, size_t align, unsigned int heap_mask,
	      unsigned int flags, struct ion_handle **handle)
{
        int ret;
        struct ion_allocation_data data = {
                .len = len,
                .align = align,
		.heap_id_mask = heap_mask, //ActionsCode(authro:songzhining, comment: update ion.h from kernel)
                .flags = flags,
        };

        ret = ion_ioctl(fd, ION_IOC_ALLOC, &data);
        if (ret < 0)
                return ret;
        *handle = data.handle;
        return ret;
}

int ion_free(int fd, struct ion_handle *handle)
{
        struct ion_handle_data data = {
                .handle = handle,
        };
        return ion_ioctl(fd, ION_IOC_FREE, &data);
}

int ion_map(int fd, struct ion_handle *handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd)
{
        struct ion_fd_data data = {
                .handle = handle,
        };

        int ret = ion_ioctl(fd, ION_IOC_MAP, &data);
        if (ret < 0)
                return ret;
        *map_fd = data.fd;
        if (*map_fd < 0) {
                printf("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        *ptr = mmap(NULL, length, prot, flags, *map_fd, offset);
        if (*ptr == MAP_FAILED) {
                printf("mmap failed: %s\n", strerror(errno));
                return -errno;
        }
        return ret;
}

int ion_share(int fd, struct ion_handle *handle, int *share_fd)
{
        int map_fd;
        struct ion_fd_data data = {
                .handle = handle,
        };

        int ret = ion_ioctl(fd, ION_IOC_SHARE, &data);
        if (ret < 0)
                return ret;
        *share_fd = data.fd;
        if (*share_fd < 0) {
                printf("share ioctl returned negative fd\n");
                return -EINVAL;
        }
        return ret;
}

int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask,
		 unsigned int flags, int *handle_fd) {
	struct ion_handle *handle;
	int ret;

	ret = ion_alloc(fd, len, align, heap_mask, flags, &handle);
	if (ret < 0)
		return ret;
	ret = ion_share(fd, handle, handle_fd);
	ion_free(fd, handle);
	return ret;
}

int ion_import(int fd, int share_fd, struct ion_handle **handle)
{
        struct ion_fd_data data = {
                .fd = share_fd,
        };

        int ret = ion_ioctl(fd, ION_IOC_IMPORT, &data);
        if (ret < 0)
                return ret;
        *handle = data.handle;
        return ret;
}

int ion_sync_fd(int fd, int handle_fd)
{
    struct ion_fd_data data = {
        .fd = handle_fd,
    };
    return ion_ioctl(fd, ION_IOC_SYNC, &data);
}

int ion_phys(int fd, struct ion_handle *handle, unsigned long *phys)
{
	      int ret; 
        struct asoc_ion_phys_data phys_data = {
                .handle = handle,
        };

        struct ion_custom_data data = {
                .cmd = ASOC_ION_GET_PHY,
                .arg = (unsigned long)&phys_data,
        };
        
        ret = ion_ioctl(fd, ION_IOC_CUSTOM, &data);

        if (ret < 0)
            return ret;
        *phys = phys_data.phys_addr;
        return ret;
}

int ion_cache(int fd, struct ion_handle *handle, int cmd, void *vaddr, unsigned int offset,
    unsigned int length)
{
        int ret;
        struct ion_flush_data data = {
                .handle = handle,
                .fd = fd,
                .vaddr = vaddr,
                .offset = offset,
                .length = length,
        };

        ret = ion_ioctl(fd, cmd, &data);
        if (ret < 0)
                return ret;

        return ret;
}


int main(int argc, char* argv[])
{
	int prot = PROT_READ | PROT_WRITE;
	int map_flags = MAP_SHARED;
	struct ion_handle *handle;
	int map_fd, ret;
    void *ptr;
	
	int *phy_add = malloc(sizeof(int));


	int s_fd = 0;
	unsigned int width = 1920;//也应该16字节对齐
	unsigned int hight = 1080;
	unsigned int hight_16bytes_align = (((hight+15)/16)*16);

	int size = width*hight_16bytes_align*1.5;


	///////////////////////////////
	///1
	///////////////////////////////
	s_fd = ion_open();


	///////////////////////////////
	///2
	///////////////////////////////
	if (size & ALIGN_MASK){
		//4k对齐
		size += (ALIGN_BYTES - (size & ALIGN_MASK));

		printf("(size-1920*1080)=%d\n",size-1920*1080);
	}

	struct actal_mem * user_p;

	user_p = (struct actal_mem*)malloc(sizeof(struct actal_mem));
	user_p->next = NULL;

	ret = ion_alloc(s_fd, size, 0, 1, 3, &handle);
	if(ret < 0) {
		printf("actal_malloc_wt: ion_alloc(size: %d)  failed(%d)!\n", size, ret);
		return ret;
	}

	ret = ion_map(s_fd , handle, size, prot, map_flags, 0, (unsigned char **)&ptr, &map_fd);

	user_p->handle = handle;
    user_p->len = size;
    user_p->fd = s_fd;
	user_p->ptr = ptr;
	user_p->map_fd = map_fd;


	ret = ion_phys(s_fd, handle, (unsigned long *)phy_add);
	if(ret < 0)
	{
		printf("actal_malloc_wt: get phy_addr error!\n");
		return ret;
	}
	user_p->phy_add = *phy_add;

	printf("len = %d\n", user_p->len);
	printf("ptr = %x\n", user_p->ptr);
	printf("phy = %x\n", user_p->phy_add);

	///////////////////////////////
	///4
	///////////////////////////////
	{
		ret = ion_free(user_p->fd, user_p->handle);
		munmap(ptr, user_p->len);
		close(user_p->map_fd);

		free(user_p);

	}

	///////////////////////////////
	///5
	///////////////////////////////
	ion_close(s_fd);

	if(phy_add) {
		free(phy_add);
	}
	printf("done\n");

	return 0;

}
