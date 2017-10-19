/*
 * Copyright (C) 2017  Francis Deslauriers <francis.deslauriers@efficios.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#define _GNU_SOURCE

#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "lttng-elf.h"

#define MAX_PATH 1024
struct dl_iterate_data {
	int exec_found;
	char *exec_path;
};


struct target_probe {
	char *provider;
	char *probe_name;
	struct target_probe *next;
};

struct loaded_oject {
	char *path;
	struct loaded_oject *next;
};

static struct target_probe *target_list;

struct target_probe *create_target_probe_list(char *prov, char *prob)
{
	target_list = malloc(sizeof(struct target_probe));
	if (!target_list) {
		return NULL;
	}
	target_list->provider = prov;
	target_list->probe_name = prob;
	target_list->next = NULL;
	return target_list;
}

int add_target_probe(struct target_probe *list, char *prov, char *prob)
{
	int ret;
	struct target_probe *new, *curr;

	/*
	 * Find the end of the list.
	 */

	curr = list;
	while (curr->next != NULL) {
		curr = curr->next;
	}

	new = malloc(sizeof(struct target_probe));
	if (!new) {
		ret = -1;
		goto end;
	}
	new->provider = prov;
	new->probe_name = prob;
	new->next = NULL;

	curr->next = new;

end:
	return ret;
}

void print_list()
{
	struct target_probe *curr;
	curr=target_list;
	while (curr != NULL) {
		printf("curr --> %s:%s\n", curr->provider, curr->probe_name);
		curr = curr->next;
	}
}

int instrument_object(const char *path)
{
	int fd;
	int ret;
	uint64_t addr;
	struct target_probe *curr;
	if (!path) {
		ret = -1;
		goto end;
	}
	printf("Opening object: %s\n", path);
	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		printf("Failed to open file\n");
		ret = -1;
		goto end;
	}
	curr = target_list;
	while (curr != NULL) {
		addr = 0;
		ret = lttng_elf_get_sdt_sema_addr(fd, curr->provider, curr->probe_name, &addr);
		curr = curr->next;
		if (ret != 0) {
			printf("Sema extraction failure\n");
			continue;
		}
		if (addr == 0) {
			printf("****No semaphore fpr probe %s:%s in %s\n",
				   curr->provider, curr->probe_name, path);
			continue;
		}

		printf("****Enabling semaphore of probe %s:%s at 0x%lx in %s\n",
			   	   curr->provider, curr->probe_name, addr, path);

		*((uint64_t *) addr) = 1;
	}
end:
	return ret;
}

int elf_program_header_cb(struct dl_phdr_info* info, size_t info_size, void* priv)
{
	int ret;
	char *exec_path = NULL;

	/*
	 * If the  program header name is null, we assume that it's the main binary.
	 * If not, it's a shared object loaded in the address space.
	 * */
    if (info->dlpi_name == NULL || info->dlpi_name[0] == '\0') {
		char executable_path[MAX_PATH];
		int path_len = readlink("/proc/self/exe", executable_path, MAX_PATH - 1);
		executable_path[path_len] = '\0';
		exec_path = strdup(executable_path);
    } else {
    	/* Removing const */
    	exec_path = (char *) info->dlpi_name;
    }
    ret = instrument_object(exec_path);

	return ret;
}
static void *(*libc_dlopen)(const char *filename, int flags);

/*
 * Overrides the dlopen symbol to track newly loaded shared object that might
 * have SDT probes of interest
 */
void *dlopen(const char *filename, int flags)
{
	libc_dlopen = dlsym(RTLD_NEXT, "dlopen");
	int ret = instrument_object(filename);
	if (ret) {
		printf("\n");
	}
	return libc_dlopen(filename, flags);
}


void __attribute__((constructor)) so_ctor(void)
{

	/* Create an adhoc list of SDT probes for testing*/
	target_list = create_target_probe_list("hello", "tp");
	add_target_probe(target_list, "hello", "lol");
	add_target_probe(target_list, "rtld", "init_start");
	add_target_probe(target_list, "libpthread", "mutex_init");

	dl_iterate_phdr(elf_program_header_cb, NULL);

	return;
}
void __attribute__((destructor)) so_dtor(void)
{
	 printf("%s: Destructor\n", __FILE__);
}
