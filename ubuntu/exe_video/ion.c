#include "video.h"

#define USER_ION_H
#ifdef USER_ION_H
struct ion_handle;
enum {
	//OWL_ION_GET_PHY = 0,
	ASOC_ION_GET_PHY = 0,
};

struct asoc_ion_phys_data {
        struct ion_handle *handle;
        unsigned long phys_addr;
        size_t size;
};

struct ion_handle_data {
        struct ion_handle *handle;
};

struct ion_fd_data {
        struct ion_handle *handle;
        int fd; 
};

struct ion_custom_data {
        unsigned int cmd;
        unsigned long arg;
};

struct ion_allocation_data {
        size_t len;
        size_t align;
        unsigned int heap_id_mask;
        unsigned int flags;
        struct ion_handle *handle;
};

#define ION_IOC_MAGIC           'I'
#define ION_IOC_ALLOC           _IOWR(ION_IOC_MAGIC, 0, \
                                      struct ion_allocation_data)
#define ION_IOC_FREE            _IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)
#define ION_IOC_MAP             _IOWR(ION_IOC_MAGIC, 2, struct ion_fd_data)
#define ION_IOC_CUSTOM          _IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)
#endif

int ion_open()
{
        int fd = open("/dev/ion", O_RDWR);
        if (fd < 0)
                DBG("open /dev/ion failed!\n");
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
                DBG("ioctl %x failed with code %d: %s\n", req,
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
        *handle = (struct ion_handle*)data.handle;
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
                DBG("map ioctl returned negative fd\n");
                return -EINVAL;
        }
        *ptr = mmap(NULL, length, prot, flags, *map_fd, offset);
        if (*ptr == MAP_FAILED) {
                DBG("mmap failed: %s\n", strerror(errno));
                return -errno;
        }
        return ret;
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


static unsigned int sCount = 0;
static int sFd = -1;

int system_mmap(unsigned int size, int prot, int flag, struct ion_handle **handle, unsigned char **vir, int *phy)
{

	int ret;
	int map_fd = -1;

	if(sFd < 0) {
		sFd  = ion_open();
		if(sFd < 0) {
			DBG("ion open fail, return \n");
			return sFd;
		}
	}
	
	ret = ion_alloc(sFd, size, 0, 1, 3, handle);
	if(ret < 0) {
		DBG("ion alloc fail: ret = %d\n", ret);
		return ret;
	}

	ret = ion_map(sFd, *handle, size, prot, flag, 0, vir, &map_fd);
	if(ret < 0) {
		DBG("ion map fail: ret = %d\n", ret);
		return ret;
	}

	ret = ion_phys(sFd, *handle, (unsigned long *)phy);
	if(ret < 0) {
		DBG("ion phys fail: ret = %d\n", ret);
	}
	sCount++;

	return map_fd;
}

void system_munmap(struct ion_handle *handle, unsigned int size, void *vir, int map_fd)
{
	int ret = 0;

	if(sFd < 0)
		return;

	if(sCount) {
		munmap(vir, size);
		close(map_fd);
		ion_free(sFd, handle);
		sCount--;
	}

	if(sCount == 0) {
		ion_close(sFd);
		sFd = -1;
	}

	return;
}

int ion_buf_alloc(struct ion_buf *buf, unsigned int size)
{
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_SHARED;
	struct ion_handle *handle = NULL;
	int map_fd = -1;
	int ret;
    	void *vir = NULL;
	int phy = 0;

	memset(buf, 0, sizeof(struct ion_buf));

	if (size & ALIGN_MASK){
		size += (ALIGN_BYTES - (size & ALIGN_MASK));
	}

	map_fd = system_mmap(size, prot, flags, &handle, (unsigned char **)&vir, &phy);
	if(map_fd < 0) {
		DBG("system map fail: map_fd = %d\n", map_fd);
		system_munmap(handle, size, vir, map_fd);
		return -1;
	}
	buf->size = size;
	buf->handle = handle;
	buf->vir = vir;
	buf->phy = phy;
	buf->map_fd = map_fd;

	return 0;
}

int ion_buf_free(struct ion_buf *buf)
{
	system_munmap(buf->handle, buf->size, buf->vir, buf->map_fd);
	return 0;
}
