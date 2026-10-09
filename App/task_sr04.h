#ifndef TASK_SR04_H
#define TASK_SR04_H

#define SR04_TASK_STACK_SIZE    256
#define SR04_TASK_PRIORITY      1       /* 最低优先级，不干扰系统 */

void SR04Task_Run(void *pvParameters);

#endif
