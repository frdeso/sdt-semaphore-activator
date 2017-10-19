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
#include <stdio.h>

#include "hello_provider.h"

int main(int argc, char *argv[])
{
	if(HELLO_TP1_ENABLED()) {
		printf("In TP_ENABLED()\n");
		HELLO_TP1();
	}
	if(HELLO_TP2_ENABLED()) {
		printf("In LOL_ENABLED()\n");
		HELLO_TP2();
	}
	dlopen("/lib/x86_64-linux-gnu/libpthread-2.23.so", RTLD_LAZY);
	printf("Running main executable\n");

	return 0;
}

