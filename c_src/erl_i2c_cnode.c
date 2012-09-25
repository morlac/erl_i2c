/*
 * erl_i2c_cnode.c
 *
 *  Created on: 09.09.2012
 *      Author: Christian Adams <morlac78@googlemail.com>
 *   Copyright: (c) 2012 by Christian Adams
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *   MA 02110-1301 USA.
 *
 */
#define _GNU_SOURCE

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "erl_interface.h"
#include "ei.h"

#include <sys/ioctl.h>

#include "include/linux/i2c-dev.h"

#ifndef __u8
typedef unsigned char __u8;
#endif

#define BUFSIZE 4096
#define PORTBASE 4200

typedef struct s_i2c_bus {
	int bus_number;
	char* bus_device;
	int bus_fd;
	char device_address;
	char device_register;
	struct s_i2c_bus *next;
} t_i2c_bus;

int erl_i2c_listen(int port) {
	int listen_fd;
	struct sockaddr_in addr;
	int on = 1;

	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return (-1);
	}

	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset((void*) &addr, 0, (size_t) sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listen_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		return (-1);
	}

	listen(listen_fd, 5);

	return listen_fd;
}

t_i2c_bus* get_bus(int bus_number, t_i2c_bus* i2c_bus_list) {
	t_i2c_bus* i2c_bus = i2c_bus_list;

	while (i2c_bus) {
		if (i2c_bus->bus_number == bus_number) {
			return i2c_bus;
		}
		i2c_bus = i2c_bus->next;
	}

	return NULL;
}

t_i2c_bus* open_bus(int bus_number) {
	int bus_fd = -1;
	t_i2c_bus *i2c_bus = NULL;
	char* bus_device;

	asprintf(&bus_device, "/dev/i2c-%d", bus_number);

	if ((bus_fd = open(bus_device, O_RDWR)) > 0) {
		i2c_bus = (t_i2c_bus*)malloc(sizeof(t_i2c_bus));

		i2c_bus->bus_device = bus_device;
		i2c_bus->bus_fd = bus_fd;
		i2c_bus->bus_number = bus_number;
		i2c_bus->device_address = 0;
		i2c_bus->device_register = 0;
		i2c_bus->next = NULL;
	} else {
		free(bus_device);
	}

	return i2c_bus;
}

void close_bus(int bus_number, t_i2c_bus* i2c_bus_list) {
	t_i2c_bus * i2c_bus = NULL;

	if (i2c_bus_list != NULL) {
		if ((i2c_bus = get_bus(bus_number, i2c_bus_list))){

			i2c_bus->bus_fd = close(i2c_bus->bus_fd);
		}
	}
}

t_i2c_bus* append_bus(t_i2c_bus* i2c_bus, t_i2c_bus* i2c_bus_list) {
	if (i2c_bus_list == NULL) {
		return i2c_bus;
	} else {
		i2c_bus->next = i2c_bus_list;
		return i2c_bus;
	}
}

t_i2c_bus* remove_bus(int bus_number, t_i2c_bus* i2c_bus_list) {
	t_i2c_bus *i2c_bus_last = NULL, *i2c_bus;

	// remove first i2c-bus - lazy lazy lazy .. but works
	if (i2c_bus_list->bus_number == bus_number) {
		i2c_bus = i2c_bus_list->next;

		free(i2c_bus_list->bus_device);
		free(i2c_bus_list);

		return i2c_bus;
	}

	i2c_bus = i2c_bus_list;

	while (i2c_bus->next != NULL) {
		if (i2c_bus->bus_number == bus_number) {
			if (i2c_bus_last == NULL) {
				i2c_bus_last = i2c_bus->next;

				free(i2c_bus->bus_device);
				free(i2c_bus);

				return i2c_bus_last;
			} else {
				i2c_bus_last->next = i2c_bus->next;

				free(i2c_bus->bus_device);
				free(i2c_bus);

				return i2c_bus_list;
			}
		}
		i2c_bus_last = i2c_bus;
		i2c_bus = i2c_bus->next;
	}

	return i2c_bus_list;
}

int get_bus_fd(int bus_number, t_i2c_bus* i2c_bus_list) {
	t_i2c_bus* i2c_bus = get_bus(bus_number, i2c_bus_list);

	if (i2c_bus) {
		return i2c_bus->bus_fd;
	}

	return -1;
}

int i2c_set_address(int bus_fd, int device_address) {
	return ioctl(bus_fd, I2C_SLAVE, device_address);
}

int get_bus_device_address(int bus_number, t_i2c_bus* i2c_bus_list) {
	t_i2c_bus* i2c_bus = get_bus(bus_number, i2c_bus_list);

	if (i2c_bus) {
		return i2c_bus->device_address;
	}

	return -1;
}

ETERM* get_bus_info(t_i2c_bus* i2c_bus) {
	return erl_format(
			"[{bus_number, ~i},"\
			" {bus_device, ~s},"\
			" {bus_fd, ~i},"\
			" {device_address, ~i}"\
			" {device_register, ~i}]",
			i2c_bus->bus_number,
			i2c_bus->bus_device,
			i2c_bus->bus_fd,
			i2c_bus->device_address,
			i2c_bus->device_register);
}

int main(int argc, char **argv) {
	// erlang c-node vars
	int erl_port = -1;
	int erl_listen = -1;
	int erl_fd = -1;
	int erl_got = -1;
	unsigned char erl_buf[BUFSIZE] __attribute__ ((aligned));
	char* erl_cookie;
	ErlConnect erl_conn;
	ErlMessage emsg;
	ETERM *fromp, *tuplep, *fnp, *argp, *resp;
	t_i2c_bus *i2c_bus_list = NULL, *i2c_bus = NULL;

	ETERM *Bus_Num, *Dev_Addr, *Dev_Reg, *Dev_Data, *Dev_Data_Len,
			*Pat1, *Pat2, *Pat3, *Pat4;

	unsigned char current_bus = 0;
	unsigned char current_address = 0;
	unsigned char current_register = 0;

	unsigned char bus_number;
	unsigned char device_address;
	unsigned char device_register;
	char *device_data = NULL;
	unsigned char device_data_len = 0;
	int device_data_read;

	bool cont = true;
	bool got_data = false;
	bool mainloop = true;

	// first setup erlang-node and connection to epmd
	erl_port = PORTBASE;

	erl_init(NULL, 0);

	erl_cookie = argv[1];

	if (!erl_connect_init(0, erl_cookie, 0)) {
		erl_err_quit("\nerl_connect_init");
	}

	// make a listen socket
	if ((erl_listen = erl_i2c_listen(erl_port)) <= 0) {
		erl_err_quit(
				"error during erl_i2c_listen\nunable to create listen-socket\n"
				"already running for this port?");
	}

	// publish listen port via epmd
	if (erl_publish(erl_port) == -1) {
		erl_err_quit("error during erl_publish - epmd not running?");
	}

	// to tell calling erlang our nodename
	fprintf(stderr, "this.nodename: %s\n", erl_thisnodename());

	// erlang.cookie _must_ be set properly
	if ((erl_fd = erl_accept(erl_listen, &erl_conn)) == ERL_ERROR) {
		erl_err_quit("error on erl_accept - erlang-cookie properly set?");
	}

	while (mainloop) {
		erl_got = erl_receive_msg(erl_fd, erl_buf, BUFSIZE, &emsg);

		if (erl_got == ERL_TICK) {
			// got an ERL_TICK .. and ignoring it silently
			continue;
		} else if (erl_got == ERL_ERROR) {
			mainloop = false;
		} else if (erl_got == ERL_EXIT) {
			mainloop = false;
		} else {
			if (emsg.type == ERL_SEND) {
			} else if (emsg.type == ERL_REG_SEND) {
				fromp = erl_element(2, emsg.msg);
				tuplep = erl_element(3, emsg.msg);

				fnp = erl_element(1, tuplep);
				resp = NULL;

/*************
 * open_bus
 * {open_bus, Bus_Number}
 */
				if (strncmp(ERL_ATOM_PTR(fnp), "open_bus", 8) == 0) {
					if ((argp = erl_element(2, tuplep)) && ERL_IS_INTEGER(argp)) {
						bus_number = ERL_INT_VALUE(argp);

						if (get_bus_fd(bus_number, i2c_bus_list) < 0) {
							if ((i2c_bus = open_bus(bus_number)) != NULL) {
								i2c_bus_list = append_bus(i2c_bus, i2c_bus_list);
								current_bus = bus_number;

								resp = erl_format(
										"{erl_i2c_cnode, {open_bus, ok, ~i}}",
										bus_number);
							} else {
								resp = erl_format(
										"{erl_i2c_cnode, {open_bus, error, ~s}}",
										strerror(errno));
							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {open_bus, error, already_open}}");
						}
					} else {
						resp = erl_format(
								"{erl_i2c_cnode, {open_bus, error, badarg}}");
					}

					erl_free_term(argp);

					erl_send(erl_fd, fromp, resp);
				}
/**************
 * close bus
 * {close_bus, Bus_Number}
 */
				else if (strncmp(ERL_ATOM_PTR(fnp), "close_bus", 9) == 0) {
					if (!i2c_bus_list) {
						resp = erl_format(
								"{erl_i2c_cnode, {close_bus, error, no_open_bus}}");
					} else {
						if ((argp = erl_element(2, tuplep)) && ERL_IS_INTEGER(argp)) {
							bus_number = ERL_INT_VALUE(argp);

							if (!get_bus(bus_number, i2c_bus_list)) {
								resp = erl_format(
										"{erl_i2c_cnode, {close_bus, error, bus_not_open}}");
							} else {
								close_bus(bus_number, i2c_bus_list);
								i2c_bus_list = remove_bus(bus_number, i2c_bus_list);

								resp = erl_format(
										"{erl_i2c_cnode, {close_bus, ok}}");
							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {close_bus, error, badarg}}");
						}

						erl_free_term(argp);
					}

					erl_send(erl_fd, fromp, resp);
				}
/**************
 * read byte
 * {read_byte, Data_Len}
 * {read_byte, Register, Data_Len}
 * {read_byte, Device_Address, Register, Data_Len}
 * {read_byte, Bus_Number, Device_Address, Register, Data_Len}
 **************/
				else if (strncmp(ERL_ATOM_PTR(fnp), "read_byte", 9) == 0) {
					if (i2c_bus_list) {
						got_data = false;

						Bus_Num = NULL;
						Dev_Addr = NULL;
						Dev_Reg = NULL;
						Dev_Data_Len = NULL;

						Pat1 = erl_format("{read_byte, Bus_Num, Dev_Addr, Dev_Reg, Dev_Data_Len}");
						Pat2 = erl_format("{read_byte, Dev_Addr, Dev_Reg, Dev_Data_Len}");
						Pat3 = erl_format("{read_byte, Dev_Reg, Dev_Data_Len}");
						Pat4 = erl_format("{read_byte, Dev_Data_Len}");

						bus_number      = current_bus;
						device_address  = current_address;
						device_register = current_register;

						if (erl_match(Pat1, tuplep)) {
							Bus_Num = erl_var_content(Pat1, "Bus_Num");
							Dev_Addr = erl_var_content(Pat1, "Dev_Addr");
							Dev_Reg = erl_var_content(Pat1, "Dev_Reg");
							Dev_Data_Len = erl_var_content(Pat1, "Dev_Data_Len");

							if (ERL_IS_INTEGER(Bus_Num) &&
									ERL_IS_INTEGER(Dev_Addr) &&
									ERL_IS_INTEGER(Dev_Reg) &&
									ERL_IS_INTEGER(Dev_Data_Len)) {
								bus_number      = (unsigned char)ERL_INT_UVALUE(Bus_Num);
								device_address  = (unsigned char)ERL_INT_UVALUE(Dev_Addr);
								device_register = (unsigned char)ERL_INT_UVALUE(Dev_Reg);
								device_data_len = (unsigned char)ERL_INT_UVALUE(Dev_Data_Len);

								got_data = true;
							}
						} else if (erl_match(Pat2, tuplep)) {
							Dev_Addr = erl_var_content(Pat2, "Dev_Addr");
							Dev_Reg = erl_var_content(Pat2, "Dev_Reg");
							Dev_Data_Len = erl_var_content(Pat2, "Dev_Data_Len");

							if (ERL_IS_INTEGER(Dev_Addr) &&
									ERL_IS_INTEGER(Dev_Reg) &&
									ERL_IS_INTEGER(Dev_Data_Len)) {
								device_address  = (unsigned char)ERL_INT_UVALUE(Dev_Addr);
								device_register = (unsigned char)ERL_INT_UVALUE(Dev_Reg);
								device_data_len = (unsigned char)ERL_INT_UVALUE(Dev_Data_Len);

								got_data = true;
							}
						} else if (erl_match(Pat3, tuplep)) {
							Dev_Reg = erl_var_content(Pat3, "Dev_Reg");
							Dev_Data_Len = erl_var_content(Pat3, "Dev_Data_Len");

							if (ERL_IS_INTEGER(Dev_Reg) &&
									ERL_IS_INTEGER(Dev_Data_Len)) {
								device_register = (unsigned char)ERL_INT_UVALUE(Dev_Reg);
								device_data_len = (unsigned char)ERL_INT_UVALUE(Dev_Data_Len);

								got_data = true;
							}
						} else if (erl_match(Pat4, tuplep)) {
							Dev_Data_Len = erl_var_content(Pat4, "Dev_Data_Len");

							if (ERL_IS_INTEGER(Dev_Data_Len)) {
								device_data_len = (unsigned char)ERL_INT_UVALUE(Dev_Data_Len);

								got_data = true;
							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {read_byte, error, badarg}}");
						}

						if (got_data) {
							cont = true;

							if ((i2c_bus = get_bus(bus_number, i2c_bus_list))) {
								if (device_address != i2c_bus->device_address) {
									if (i2c_set_address(i2c_bus->bus_fd, device_address) < 0) {
										resp = erl_format(
												"{erl_i2c_cnode, {read_byte, address_error, ~s}}",
												strerror(errno));
										cont = false;
									} else {
										current_address = device_address;
										i2c_bus->device_address = device_address;
									}
								}

								if (cont) {
									if (device_data_len <= 32) {
										device_data = calloc(device_data_len, sizeof(char));

										if ((device_data_read =
												i2c_smbus_read_i2c_block_data(
														i2c_bus->bus_fd,
														device_register,
														device_data_len,
														(__u8*) device_data)) < 0) {
											resp = erl_format(
													"{erl_i2c_cnode, {read_byte, i2c_error, ~s}}",
													strerror(errno));
										} else {
											current_register = device_register;
											i2c_bus->device_register = device_register;
											resp = erl_format(
													"{erl_i2c_cnode, {read_byte, ok, ~i, ~w}}",
													device_data_read, erl_mk_binary(device_data, device_data_read));
										}

										free(device_data);
									} else {
										resp = erl_format(
												"{erl_i2c_cnode, {read_byte, error, too_much_data_requested}}");
									}
								}

								// at the end
								current_bus = bus_number;
								current_address = device_address;
								current_register = device_register;
							} else {
								resp = erl_format(
										"{erl_i2c_cnode, {read_byte, error, bus_not_open}}");
							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {read_byte, error, badarg}}");
						}

						erl_free_term(Bus_Num);
						erl_free_term(Dev_Addr);
						erl_free_term(Dev_Reg);
						erl_free_term(Dev_Data_Len);
						erl_free_term(Pat1);
						erl_free_term(Pat2);
						erl_free_term(Pat3);
						erl_free_term(Pat4);
					} else {
						resp = erl_format(
								"{erl_i2c_cnode, {read_byte, error, no_open_bus}}");
					}
					erl_send(erl_fd, fromp, resp);
				}
/**************
 * write byte
 * {write_byte, Data_Byte}
 * {write_byte, Register, Data_Byte}
 * {write_byte, Device_Address, Register, Data_Byte}
 * {write_byte, Bus_Number, Device_Address, Register, Data_Byte}
 */
				else if (strncmp(ERL_ATOM_PTR(fnp), "write_byte", 10) == 0) {
					if (i2c_bus_list) {
						got_data = false;

						Bus_Num = NULL;
						Dev_Addr = NULL;
						Dev_Reg = NULL;
						Dev_Data = NULL;

						Pat1 = erl_format("{write_byte, Bus_Num, Dev_Addr, Dev_Reg, Dev_Data}");
						Pat2 = erl_format("{write_byte, Dev_Addr, Dev_Reg, Dev_Data}");
						Pat3 = erl_format("{write_byte, Dev_Reg, Dev_Data}");
						Pat4 = erl_format("{write_byte, Dev_Data}");

						bus_number = current_bus;
						device_address = current_address;
						device_register = current_register;

						if (erl_match(Pat1, tuplep)) {
							Bus_Num = erl_var_content(Pat1, "Bus_Num");
							Dev_Addr = erl_var_content(Pat1, "Dev_Addr");
							Dev_Reg = erl_var_content(Pat1, "Dev_Reg");
							Dev_Data = erl_var_content(Pat1, "Dev_Data");

							if (ERL_IS_INTEGER(Bus_Num) &&
									ERL_IS_INTEGER(Dev_Addr) &&
									ERL_IS_INTEGER(Dev_Reg) &&
									ERL_IS_BINARY(Dev_Data)) {
								bus_number = ERL_INT_UVALUE(Bus_Num);
								device_address = (char)ERL_INT_UVALUE(Dev_Addr);
								device_register = (char)ERL_INT_UVALUE(Dev_Reg);

								device_data_len = ERL_BIN_SIZE(Dev_Data);
								device_data = calloc(device_data_len, sizeof(char));
								memcpy(device_data, ERL_BIN_PTR(Dev_Data), device_data_len);

								got_data = true;
							}
						} else if (erl_match(Pat2, tuplep)) {
							Dev_Addr = erl_var_content(Pat2, "Dev_Addr");
							Dev_Reg = erl_var_content(Pat2, "Dev_Reg");
							Dev_Data = erl_var_content(Pat2, "Dev_Data");

							if (ERL_IS_INTEGER(Dev_Addr) &&
									ERL_IS_INTEGER(Dev_Reg) &&
									ERL_IS_BINARY(Dev_Data)) {
								device_address = (char)ERL_INT_UVALUE(Dev_Addr);
								device_register = (char)ERL_INT_UVALUE(Dev_Reg);

								device_data_len = ERL_BIN_SIZE(Dev_Data);
								device_data = calloc(device_data_len, sizeof(char));
								memcpy(device_data, ERL_BIN_PTR(Dev_Data), device_data_len);

								got_data = true;
							}
						} else if (erl_match(Pat3, tuplep)) {
							Dev_Reg = erl_var_content(Pat3, "Dev_Reg");
							Dev_Data = erl_var_content(Pat3, "Dev_Data");

							if (ERL_IS_INTEGER(Dev_Reg) &&
									ERL_IS_BINARY(Dev_Data)) {
								device_register = (char)ERL_INT_UVALUE(Dev_Reg);

								device_data_len = ERL_BIN_SIZE(Dev_Data);
								device_data = calloc(device_data_len, sizeof(char));
								memcpy(device_data, ERL_BIN_PTR(Dev_Data), device_data_len);

								got_data = true;
							}
						} else if (erl_match(Pat4, tuplep)) {
							Dev_Data = erl_var_content(Pat4, "Dev_Data");

							if (ERL_IS_BINARY(Dev_Data)) {
								device_data_len = ERL_BIN_SIZE(Dev_Data);
								device_data = calloc(device_data_len, sizeof(char));
								memcpy(device_data, ERL_BIN_PTR(Dev_Data), device_data_len);

								got_data = true;
							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {write_byte, error, badarg}}");
						}

						if (got_data) {
							cont = true;

							if ((i2c_bus = get_bus(bus_number, i2c_bus_list))) {
								if (device_address != i2c_bus->device_address) {
									if (i2c_set_address(i2c_bus->bus_fd, device_address) < 0) {
										resp = erl_format(
												"{erl_i2c_cnode, {write_byte, address_error, ~s}}",
												strerror(errno));
										cont = false;
									} else {
										current_address = device_address;
										i2c_bus->device_address = device_address;
									}
								}

								if (cont) {
									if (device_data_len <= 32) {
										if (i2c_smbus_write_i2c_block_data(
												i2c_bus->bus_fd,
												device_register,
												device_data_len,
												(__u8*) device_data) < 0) {
											resp = erl_format(
													"{erl_i2c_cnode, {write_byte, i2c_error, ~s}}",
													strerror(errno));
										} else {
											current_register = device_register;
											i2c_bus->device_register = device_register;

											resp = erl_format(
													"{erl_i2c_cnode, {write_byte, ok, ~i}}",
													device_data_len);
										}
									} else {
										resp = erl_format(
												"{erl_i2c_cnode, {write_byte, error, too_much_data}}");
									}
								}

								// at the end
								current_bus = bus_number;
								current_address = device_address;
								current_register = device_register;
							} else {
								resp = erl_format(
										"{erl_i2c_cnode, {write_byte, error, bus_not_open}}");
							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {write_byte, error, badarg}}");
						}

						if (device_data) {
							free(device_data);
						}

						erl_free_term(Bus_Num);
						erl_free_term(Dev_Addr);
						erl_free_term(Dev_Reg);
						erl_free_term(Dev_Data);
						erl_free_term(Pat1);
						erl_free_term(Pat2);
						erl_free_term(Pat3);
						erl_free_term(Pat4);
					} else {
						resp = erl_format(
								"{erl_i2c_cnode, {write_byte, error, no_open_bus}}");
					}

					erl_send(erl_fd, fromp, resp);
				}
/**************
 * get_address
 * {get_address} - returns device_address set on current bus
 * {get_address, Bus_Number}
 */
				else if (strncmp(ERL_ATOM_PTR(fnp), "get_address", 8) == 0) {
					if ((argp = erl_element(2, tuplep))) {
						if (ERL_IS_INTEGER(argp)) {
							bus_number = ERL_INT_VALUE(argp);
							if ((i2c_bus = get_bus(bus_number, i2c_bus_list))) {
								resp = erl_format(
										"{erl_i2c_cnode, {get_address, ok, ~i}}",
										i2c_bus->device_address);
							} else {
								resp = erl_format(
										"{erl_i2c_cnode, {get_address, error, bus_not_open}}");

							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {get_address, error, badarg}}");
						}
					} else {
						if (current_bus >= 0) {
							resp = erl_format(
									"{erl_i2c_cnode, {get_address, ok, ~i}}",
									get_bus_device_address(current_bus, i2c_bus_list));
						} else {
							resp = erl_format(
									"{erl_i2c_conde, {get_address, error, no_bus_set}}");
						}
					}

					erl_free_term(argp);

					erl_send(erl_fd, fromp, resp);
				}
/**************
 * set_address
 * {set_address, Device_Address}
 * {set_address, Bus_Number, Device_Address}
 */
				else if (strncmp(ERL_ATOM_PTR(fnp), "set_address", 8) == 0) {
					if (i2c_bus_list) {
						ETERM *Dev_Addr = NULL, *Bus_Num = NULL, *Pat1 = NULL, *Pat2 = NULL;
						Pat1 = erl_format("{set_address, Device_Address}");
						Pat2 = erl_format("{set_address, Bus_Number, Device_Address}");

						if (erl_match(Pat1, tuplep)) {
							Dev_Addr = erl_var_content(Pat1, "Device_Address");

							if (current_bus >= 0) {
								if (ERL_IS_INTEGER(Dev_Addr)) {
									device_address = ERL_INT_UVALUE(Dev_Addr);

									i2c_bus = get_bus(current_bus, i2c_bus_list);

									if (i2c_set_address(i2c_bus->bus_fd, device_address) < 0) {
										resp = erl_format(
												"{erl_i2c_cnode, {set_address, error, ~s}}",
												strerror(errno));
									} else {
										i2c_bus->device_address = device_address;
										current_address = device_address;
										resp = erl_format(
												"{erl_i2c_cnode, {set_address, ok, ~i}}",
												device_address);
									}
								} else {
									resp = erl_format(
											"{erl_i2c_cnode, {set_address, error, badarg}}");
								}
							} else {
								resp = erl_format(
										"{erl_i2c_cnode, {set_address, error, no_current_bus_set}}");
							}

							erl_free_term(Dev_Addr);
						} else if (erl_match(Pat2, tuplep)) {
							Dev_Addr = erl_var_content(Pat2, "Device_Address");
							Bus_Num  = erl_var_content(Pat2, "Bus_Number");

							if (ERL_IS_INTEGER(Dev_Addr) &&
									ERL_IS_INTEGER(Bus_Num)) {
								bus_number = ERL_INT_UVALUE(Bus_Num);
								device_address = ERL_INT_UVALUE(Dev_Addr);

								if ((i2c_bus = get_bus(bus_number, i2c_bus_list))) {
									current_bus = bus_number;
									if (i2c_set_address(i2c_bus->bus_fd, device_address) < 0) {
										resp = erl_format(
												"{erl_i2c_cnode, {set_address, error, ~s}}",
												strerror(errno));
									} else {
										i2c_bus->device_address = device_address;
										current_address = device_address;
										resp = erl_format(
												"{erl_i2c_cnode, {set_address, ok, ~i}}",
												device_address);
									}
								} else {
									resp = erl_format(
											"{erl_i2c_cnode, {set_address, error, bus_not_open}}");
								}
							} else {
								resp = erl_format(
										"{erl_i2c_cnode, {set_address, error, badarg}}");
							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {set_address, error, badarg}}");
						}

						erl_free_term(Dev_Addr);
						erl_free_term(Bus_Num);
						erl_free_term(Pat1);
						erl_free_term(Pat2);
					} else {
						resp = erl_format(
								"{erl_i2c_cnode, {set_address, error, no_open_bus}}");
					}

					erl_send(erl_fd, fromp, resp);
				}
/**************
 * TODO: bus_info
 * {bus_info}
 * {bus_info, Bus_Number}
 */
				else if (strncmp(ERL_ATOM_PTR(fnp), "bus_info", 8) == 0) {
					if (i2c_bus_list) {
						if ((argp = erl_element(2, tuplep)) && ERL_IS_INTEGER(argp)) {
							bus_number = ERL_INT_VALUE(argp);
							if ((i2c_bus = get_bus(bus_number, i2c_bus_list))) {
								resp = erl_format(
										"{erl_i2c_cnode, {bus_info, ~w}}",
										get_bus_info(i2c_bus));
							} else {
								resp = erl_format(
										"{erl_i2c_cnode, {bus_info, error, bus_not_open}}");
							}
						} else {
// constructing list of bus_info
							resp = erl_format(
									"{erl_i2c_cnode, {bus_info, list_not_implemented_yet}}");
						}

						erl_free_term(argp);
					} else {
						resp = erl_format(
								"{erl_i2c_cnode, {bus_info, error, no_open_bus}}");
					}

					erl_send(erl_fd, fromp, resp);
				}
/**************
 * get_bus
 * {get_bus}
 */
				else if (strncmp(ERL_ATOM_PTR(fnp), "get_bus", 7) == 0) {
					if (i2c_bus_list) {
						if (current_bus >= 0) {
							resp = erl_format(
									"{erl_i2c_cnode, {get_bus, ok, ~i}}",
									current_bus);
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {get_bus, error, no_current_bus}}");
						}
					} else {
						resp = erl_format(
								"{erl_i2c_cnode, {get_bus, error, no_bus_open}}");
					}

					erl_send(erl_fd, fromp, resp);
				}
/**************
 * set_bus
 * {set_bus, Bus_Number}
 */
				else if (strncmp(ERL_ATOM_PTR(fnp), "set_bus", 7) == 0) {
					if (i2c_bus_list) {
						if ((argp = erl_element(2, tuplep)) && ERL_IS_INTEGER(argp)) {
							bus_number = ERL_INT_VALUE(argp);
							if ((i2c_bus = get_bus(bus_number, i2c_bus_list))) {
								current_bus = bus_number;
								resp = erl_format(
										"{erl_i2c_cnode, {set_bus, ok, ~i}}",
										bus_number);
							} else {
								resp = erl_format(
										"{erl_i2c_cnode, {set_bus, error, bus_not_open}}");
							}
						} else {
							resp = erl_format(
									"{erl_i2c_cnode, {set_bus, error, badarg}}");
						}

						erl_free_term(argp);
					} else {
						resp = erl_format(
								"{erl_i2c_cnode, {set_bus, error, no_bus_open}}");
					}

					erl_send(erl_fd, fromp, resp);
				}
/**************
 * exit
 */
				else if (strncmp(ERL_ATOM_PTR(fnp), "exit", 4) == 0) {
					mainloop = false;

					resp = erl_format(
							"{erl_i2c_cnode, {ok, exiting}}");
					erl_send(erl_fd, fromp, resp);
				}
/**************
 * unknown command
 */
				else {
					// What should I say?!
					// I didn't even understand what you just said!
					resp = erl_format(
							"{erl_i2c_cnode, {error, unknown_command, ~w}}",
							tuplep);

					erl_send(erl_fd, fromp, resp);
				}

				erl_free_term(fromp);
				erl_free_term(tuplep);
				erl_free_term(fnp);
				erl_free_term(emsg.from);
				erl_free_term(emsg.msg);
				erl_free_compound(resp);
			}
		}
	}

	if (erl_fd) {
		erl_close_connection(erl_fd);
	}

	// cleanup i2c-buslist
	while (i2c_bus_list != NULL) {
		i2c_bus = i2c_bus_list;
		i2c_bus_list = i2c_bus_list->next;

		close(i2c_bus->bus_fd);
		free(i2c_bus->bus_device);
		free(i2c_bus);
	}

	exit(0);
}

// vim:ft=erlang shiftwidth=2 tabstop=2 softtabstop=2
