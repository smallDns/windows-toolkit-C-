#ifndef TK_HARDWARE_H
#define TK_HARDWARE_H
#define HW_ITEMS 7
#define HW_VALUE_MAX 4096
typedef struct {
    const char *label;
    char value[HW_VALUE_MAX];
} HwItem;
typedef struct { HwItem items[HW_ITEMS]; } HwInfo;
void hw_get_data(HwInfo *d);
void hw_show(void);
#endif
