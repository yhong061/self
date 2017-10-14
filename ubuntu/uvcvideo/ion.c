/*
 *  ion.c
 *
 * Memory Allocator functions for ion
 *
 *   Copyright 2011 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#define LOG_TAG "ion"

//#include <cutils/log.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/ion.h>
//#include <linux/owl_ion.h>
//#include <ion.h>
struct ion_handle;

enum {
	OWL_ION_GET_PHY = 0,
};

struct owl_ion_phys_data {
	ion_user_handle_t handle;
	unsigned long phys_addr;
	size_t size;
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
        *handle = (struct ion_handle*)data.handle;
        return ret;
}

int ion_free(int fd, struct ion_handle *handle)
{
        struct ion_handle_data data = {
                .handle = (unsigned int)handle,
        };
        return ion_ioctl(fd, ION_IOC_FREE, &data);
}

int ion_map(int fd, struct ion_handle *handle, size_t length, int prot,
            int flags, off_t offset, unsigned char **ptr, int *map_fd)
{
        struct ion_fd_data data = {
                .handle = (unsigned int)handle,
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
                .handle = (unsigned int)handle,
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
        *handle = (struct ion_handle *)data.handle;
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
        struct owl_ion_phys_data phys_data = {
                .handle = (unsigned int)handle,
        };

        struct ion_custom_data data = {
                .cmd = OWL_ION_GET_PHY,
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

static unsigned int sCount = 0;
static int sFd = -1;

int system_mmap(unsigned int size, int prot, int flag, struct ion_handle **handle, unsigned char **vir, int *phy)
{

	int ret;
	int map_fd = -1;

	if(sFd < 0) {
		sFd  = ion_open();
		if(sFd < 0) {
			printf("ion open fail, return \n");
			return sFd;
		}
	}
	
	ret = ion_alloc(sFd, size, 0, 1, 3, handle);
	if(ret < 0) {
		printf("ion alloc fail: ret = %d\n", ret);
		return ret;
	}

	ret = ion_map(sFd, *handle, size, prot, flag, 0, vir, &map_fd);
	if(ret < 0) {
		printf("ion map fail: ret = %d\n", ret);
		return ret;
	}

	ret = ion_phys(sFd, *handle, (unsigned long *)phy);
	if(ret < 0) {
		printf("ion phys fail: ret = %d\n", ret);
	}
	sCount++;

	return map_fd;
}

void system_munmap(struct ion_handle *handle, unsigned int size, void *vir, int *phy, int map_fd)
{
	int ret = 0;

	if(sFd < 0)
		return;

	if(sCount) {
//		free((void *)phy);
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