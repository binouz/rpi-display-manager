SOURCES  = main.c hdmi.c dbus.c
OBJS = $(SOURCES:.c=.o)
TARGET = rpi-display-manager
C_FLAGS  = $(CFLAGS) -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST `pkg-config --cflags gio-2.0`
LD_FLAGS  = $(LDFLAGS) -lbcm_host -lvcos -lvchiq_arm -lvchostif `pkg-config --libs gio-2.0`

all: $(OBJS) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LD_FLAGS) -o $(TARGET)

%.o : %.c
	$(CC) $(C_FLAGS) -c $< -o $@

clean:
	rm -rf *.o $(TARGET)
