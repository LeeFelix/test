#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

#define TEMP_LEVEL0_OFFSET 20
#define TEMP_LEVEL1_OFFSET 5
#define TEMP_LEVEL2_OFFSET 15

enum {
  temp_level0 = 0,
  temp_level1,
  temp_level2,
};

typedef struct TempState {
  int temp;
  int level;
} TempState;

int temp_control_init(void);
void temp_get_status(TempState* status);

#endif
