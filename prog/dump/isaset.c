/*
    isaset.c - isaset, a user-space program to write ISA registers
    Copyright (C) 2000  Frodo Looijaard <frodol@dds.nl>, and
                        Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004  The lm_sensors group

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
	Typical usage:
	isaset 0x295 0x296 0x10 0x12	Write 0x12 to address 0x10 using address/data registers
	isaset -f 0x5010 0x12		Write 0x12 to location 0x5010
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/* To keep glibc2 happy */
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ >= 0
#include <sys/io.h>
#else
#include <asm/io.h>
#endif

#ifdef __powerpc__
unsigned long isa_io_base = 0; /* XXX for now */
#endif /* __powerpc__ */

void help(void)
{
	fprintf(stderr,
	        "Syntax for I2C-like access:\n"
	        "  isaset ADDRREG DATAREG ADDRESS VALUE\n"
	        "Syntax for flat address space:\n"
	        "  isaset -f ADDRESS VALUE\n");
}

int main(int argc, char *argv[])
{
	int addrreg, datareg = 0, value, addr = 0;
	unsigned char res;
	int flat = 0;
	char *end;

	if (argc < 4) {
		help();
		exit(1);
	}

	if (!strcmp(argv[1], "-f")) {
		flat = 1;
	}

	if (argc != 5-flat) {
		help();
		exit(1);
	}

	addrreg = strtol(argv[1+flat], &end, 0);
	if (*end) {
		fprintf(stderr, "Error: Invalid address!\n");
		help();
		exit(1);
	}
	if (addrreg < 0 || addrreg > (flat?0xffff:0x3fff)) {
		fprintf(stderr,
		        "Error: Address out of range (0x0000-0x%04x)!\n",
			flat?0xffff:0x3fff);
		help();
		exit(1);
	}

	if (!flat) {
		datareg = strtol(argv[2], &end, 0);
		if (*end) {
			fprintf(stderr, "Error: Invalid data register!\n");
			help();
			exit(1);
		}
		if (datareg < 0 || datareg > 0x3fff) {
			fprintf(stderr, "Error: Data register out of range "
			        "(0x0000-0x3fff)!\n");
			help();
			exit(1);
		}

		addr = strtol(argv[3], &end, 0);
		if (*end) {
			fprintf(stderr, "Error: Invalid address!\n");
			help();
			exit(1);
		}
		if (addr < 0 || addr > 0xff) {
			fprintf(stderr, "Error: Address out of range "
			        "(0x00-0xff)!\n");
			help();
			exit(1);
		}
	}

	value = strtol(argv[4-flat], &end, 0);
	if (*end) {
		fprintf(stderr, "Error: Invalid value!\n");
		help();
		exit(1);
	}
	if (value < 0 || value > 0xff) {
		fprintf(stderr, "Error: Value out of range "
			"(0x00-0xff)!\n");
		help();
		exit(1);
	}

	if (getuid()) {
		fprintf(stderr, "Error: Can only be run as root "
		        "(or make it suid root)\n");
		exit(1);
	}

	fprintf(stderr, "WARNING! Running this program can cause "
	        "system crashes, data loss and worse!\n");
	if (flat)
		fprintf(stderr, "I will write value 0x%02x to address "
		        "0x%04x\n", value, addrreg);
	else
		fprintf(stderr, "I will write value 0x%02x to address "
		        "0x%02x of chip with address register 0x%04x\n"
		        "and data register 0x%04x.\n",
		        value, addr, addrreg, datareg);
	fprintf(stderr, "You have five seconds to reconsider and press "
	        "CTRL-C!\n\n");
	sleep(5);

#ifndef __powerpc__
	if (!flat && datareg < 0x400 && addrreg < 0x400) {
		if (ioperm(datareg, 1, 1)) {
			fprintf(stderr, "Error: Could not ioperm() data "
			        "register!\n");
			exit(1);
		}
		if (ioperm(addrreg, 1, 1)) {
			fprintf(stderr, "Error: Could not ioperm() address "
		        	"register!\n");
			exit(1);
		}
	} else {
		if (iopl(3)) {
			fprintf(stderr, "Error: Could not do iopl(3)!\n");
			exit(1);
		}
	}
#endif

	/* write */
	if (flat) {
		outb(value, addrreg);
	} else {	
		outb(addr, addrreg);
		outb(value, datareg);
	}

	/* readback */
	if (flat) {
		res = inb(addrreg);
	} else {	
		outb(addr, addrreg);
		res = inb(datareg);
	}

	if (res != value) {
		fprintf(stderr, "Warning: Data mismatch, wrote 0x%02x, "
		        "read back 0x%02x\n", value, res);
	} else {
		fprintf(stderr, "Value 0x%02x written, readback matched\n",
		        value);
	}

	exit(0);
}