#
# Copyright (c) 2022 Apoidea Technology
#
# This file is part of Jeson Example Codes.
#
# It is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# It is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with FFmpeg; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
MODULE := jpeg_encoder

BIN_NAME := $(MODULE)

BIN_SRCS := $(wildcard *.c)

CFLAGS := -Werror -Wno-unused-parameter -Werror -Wno-missing-field-initializers

LDFLAGS := -L/usr/lib/aarch64-linux-gnu/xhiveai -ljpegenc -lagilelog -lMagFramework -lpicconverter \
           -L/usr/lib/aarch64-linux-gnu/tegra -lnvbuf_utils -lnvjpeg \
           -lavutil

BIN_OBJS=$(patsubst %.c, %.o, $(BIN_SRCS))

.PHONY: all clean

all: $(BIN_NAME)

%.o: %.c
	@echo "[compiling.. $(notdir $<)]"
	gcc $(CFLAGS) -c -o $@ $<

$(BIN_NAME): $(BIN_OBJS)
	@echo "[creating.. $(notdir $@)]"
	gcc -o $@ $^ $(LDFLAGS)

clean:
	@echo "[clean.. $(MODULE)]"
	rm -rf *.o $(BIN_NAME)