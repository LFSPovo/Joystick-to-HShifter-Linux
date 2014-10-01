#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#define JS_EVENT_BUTTON         0x01    /* button pressed/released */
#define JS_EVENT_AXIS           0x02    /* joystick moved */
#define JS_EVENT_INIT           0x80    /* initial state of device */

#define NUM_GEARS 7

int joystick = -1;
int virstick = -1;

int shifter_x = 0;
int shifter_y = 0;
int current_gear = -1;
int last_gear = -1;

struct gear {
	int x;
	int y;
	int max_dist;
	int btn;
};
struct gear gears[NUM_GEARS];

struct js_event {
	__u32 time;     /* event timestamp in milliseconds */
	__s16 value;    /* value */
	__u8 type;      /* event type */
	__u8 number;    /* axis/button number */
};

void open_joystick() {
	joystick = open("/dev/input/js0", O_RDONLY);
	virstick = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	
	if (joystick < 0)
		printf("Couldn't open joystick!\n");
	else
		printf("Opened joystick with ID %d\n", joystick);
	
	if (virstick < 0)
		printf("Couldn't open the virtual joystick!\n");
	else
		printf("Opened virtual joystick with ID %d\n", virstick);
}

void init_gears() {
	gears[0].x = -24661;
	gears[0].y = -27026;
	gears[0].max_dist = 3000;
	gears[0].btn = BTN_0;
	
	gears[1].x = -32767;
	gears[1].y = 6080;
	gears[1].max_dist = 3000;
	gears[1].btn = BTN_1;
	
	gears[2].x = -5068;
	gears[2].y = -24661;
	gears[2].max_dist = 3000;
	gears[2].btn = BTN_2;
	
	gears[3].x = -21958;
	gears[3].y = 6080;
	gears[3].max_dist = 3000;
	gears[3].btn = BTN_3;
	
	gears[4].x = 675;
	gears[4].y = -20269;
	gears[4].max_dist = 3000;
	gears[4].btn = BTN_4;
	
	gears[5].x = -4054;
	gears[5].y = 6418;
	gears[5].max_dist = 3000;
	gears[5].btn = BTN_5;
	
	gears[6].x = 2026;
	gears[6].y = 6080;
	gears[6].max_dist = 2000; // Lower maximum because it would work in Neutral
	gears[6].btn = BTN_6;
}

void set_gear() {
	struct input_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = EV_KEY;
	ev.value = 1;
	ev.code = gears[current_gear].btn;
	write(virstick, &ev, sizeof(ev));
		
	ev.type = EV_SYN;
	ev.value = 0;
	ev.code = 0;
	write(virstick, &ev, sizeof(ev));
}

void release_buttons() {
	struct input_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = EV_KEY;
	ev.value = 0;
	ev.code = gears[last_gear].btn;
	write(virstick, &ev, sizeof(ev));

	ev.type = EV_SYN;
	ev.value = 0;
	ev.code = 0;
	write(virstick, &ev, sizeof(ev));
}

void init_virstick() {
	// Enable key press/release events
	ioctl(virstick, UI_SET_EVBIT, EV_KEY);
	// Set buttons that are allowed to be used
	ioctl(virstick, UI_SET_KEYBIT, BTN_0);
	ioctl(virstick, UI_SET_KEYBIT, BTN_1);
	ioctl(virstick, UI_SET_KEYBIT, BTN_2);
	ioctl(virstick, UI_SET_KEYBIT, BTN_3);
	ioctl(virstick, UI_SET_KEYBIT, BTN_4);
	ioctl(virstick, UI_SET_KEYBIT, BTN_5);
	ioctl(virstick, UI_SET_KEYBIT, BTN_6);
	
	ioctl(virstick, UI_SET_EVBIT, EV_ABS);
	ioctl(virstick, UI_SET_ABSBIT, ABS_X);
	ioctl(virstick, UI_SET_ABSBIT, ABS_Y);
	
	// Controller info
	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "H-Shifter");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x1234;
	uidev.id.product = 0xfedc;
	uidev.id.version = 1;
	
	write(virstick, &uidev, sizeof(uidev));
	ioctl(virstick, UI_DEV_CREATE);
}

int dist(int gear) {
	return sqrt(powl(gears[gear].x - shifter_x, 2) + powl(gears[gear].y - shifter_y, 2));
}

void check_gear() {
	int i;
	for (i = 0; i < NUM_GEARS; i++)
		if (dist(i) <= gears[i].max_dist) {
			current_gear = i;
			return;
		}

	current_gear = -1;
}

void read_joystick() {
	struct js_event e;
	int size = read(joystick, &e, sizeof(struct js_event));
	
	if (size == sizeof(e)) {
		if (e.type == JS_EVENT_AXIS) {
			if (e.number == 0) {
				shifter_x = e.value;
			}
			else if (e.number == 1) {
				shifter_y = e.value;
			}
		}
	}
}

int main() {
	printf("Joy4Shift Linux Edition\n");
	open_joystick();
	init_gears();
	init_virstick();
	
	while (1) {
		read_joystick();
		check_gear();
		
		if (current_gear == -1 && current_gear != last_gear)
			release_buttons();
		else if (current_gear != last_gear)
			set_gear();
		
		last_gear = current_gear;
	}

	return 0;
}
