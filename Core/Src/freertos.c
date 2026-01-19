/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rtc.h"
#include <stdio.h>
#include "st7789.h"
#include "fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum JoyStickDirection
{
	NONE,
	DOWN,
	UP,
	LEFT,
	RIGHT,
	SELECT,
	MIDDLE
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
RTC_TimeTypeDef sTime = {0};
RTC_DateTypeDef sDate = {0};

//for time
char timeStr[20];
char dateStr[20];

//for joystick
uint16_t x_value;
uint16_t y_value;
uint16_t joystickDMAbuffer[2];
uint8_t last_state_x = 0;
uint8_t last_state_y = 0;
uint8_t last_state_press = 0;
uint8_t current_location;


//for logic
uint8_t command;
int8_t menu_index_x = 0;
int8_t menu_index_y = 0;
int8_t menu_index_select = 0;
/* USER CODE END Variables */
/* Definitions for timeTask */
osThreadId_t timeTaskHandle;
const osThreadAttr_t timeTask_attributes = {
  .name = "timeTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for inputTask */
osThreadId_t inputTaskHandle;
const osThreadAttr_t inputTask_attributes = {
  .name = "inputTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for LogicTask */
osThreadId_t LogicTaskHandle;
const osThreadAttr_t LogicTask_attributes = {
  .name = "LogicTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for directionQueue */
osMessageQueueId_t directionQueueHandle;
const osMessageQueueAttr_t directionQueue_attributes = {
  .name = "directionQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Get_Time_Date_Function(void);
void Display_time(void);
void Display_Date(void);
void GetJoyStickValue(void);
void X_Direction(void);
void Y_Direction(void);
void logic_handler(void);
void Pressed_Button(void);
/* USER CODE END FunctionPrototypes */

void starttimeTask(void *argument);
void StartInputTask(void *argument);
void StartLogicTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of directionQueue */
  directionQueueHandle = osMessageQueueNew (8, sizeof(uint8_t), &directionQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of timeTask */
  timeTaskHandle = osThreadNew(starttimeTask, NULL, &timeTask_attributes);

  /* creation of inputTask */
  inputTaskHandle = osThreadNew(StartInputTask, NULL, &inputTask_attributes);

  /* creation of LogicTask */
  LogicTaskHandle = osThreadNew(StartLogicTask, NULL, &LogicTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_starttimeTask */
/**
  * @brief  Function implementing the timeTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_starttimeTask */
void starttimeTask(void *argument)
{
  /* USER CODE BEGIN starttimeTask */
  /* Infinite loop */
	ST7789_Fill_Color(BLACK);
  for(;;)
  {
	  Get_Time_Date_Function();
	  Display_time();
	  Display_Date();
	  osDelay(1000);
  }
  /* USER CODE END starttimeTask */
}

/* USER CODE BEGIN Header_StartInputTask */
/**
* @brief Function implementing the inputTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartInputTask */
void StartInputTask(void *argument)
{
  /* USER CODE BEGIN StartInputTask */
  /* Infinite loop */
  for(;;)
  {
	GetJoyStickValue();
	X_Direction();
	Y_Direction();
	Pressed_Button();
    osDelay(10);
  }
  /* USER CODE END StartInputTask */
}

/* USER CODE BEGIN Header_StartLogicTask */
/**
* @brief Function implementing the LogicTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLogicTask */
void StartLogicTask(void *argument)
{
  /* USER CODE BEGIN StartLogicTask */
  /* Infinite loop */
  for(;;)
  {
	logic_handler();
    osDelay(1);
  }
  /* USER CODE END StartLogicTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void Get_Time_Date_Function(void)
{
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void Display_time(void)
{
	ST7789_WriteString(10, 10, "Current time is: ", Font_11x18, CYAN, BLACK);
	sprintf(timeStr, "%d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
	ST7789_WriteString(10, 30, timeStr, Font_11x18, CYAN, BLACK);
}

void Display_Date(void)
{
	ST7789_WriteString(10, 50, "Current date is: ", Font_11x18, CYAN, BLACK);
	sprintf(dateStr, "%02d/%d/20%d", sDate.Month, sDate.Date, sDate.Year);
	ST7789_WriteString(10, 70, dateStr, Font_11x18, CYAN, BLACK);
}

void GetJoyStickValue(void)
{
	x_value = joystickDMAbuffer[0];
	y_value = joystickDMAbuffer[1];
}

void X_Direction(void)
{
	if (x_value > 4000 && last_state_x != UP)
	{
		current_location = UP;
		osMessageQueuePut(directionQueueHandle, &current_location, 1, 0);
		last_state_x = UP;
	}
	if (x_value < 1000 && last_state_x != DOWN)
	{
		current_location = DOWN;
		osMessageQueuePut(directionQueueHandle, &current_location, 1, 0);
		last_state_x = DOWN;
	}
	if (x_value > 1500 && x_value < 2500)
	{
		last_state_x = MIDDLE;
	}
}

void Y_Direction(void)
{
	if (y_value > 4000 && last_state_y != RIGHT)
	{
		current_location = RIGHT;
		osMessageQueuePut(directionQueueHandle, &current_location, 1, 0);
		last_state_y = RIGHT;
	}
	if (y_value < 1000 && last_state_y != LEFT)
	{
		current_location = LEFT;
		osMessageQueuePut(directionQueueHandle, &current_location, 1, 0);
		last_state_y = LEFT;
	}
	if (y_value > 1500 && y_value < 2500)
	{
		last_state_y = MIDDLE;
	}
}

void Pressed_Button(void)
{
	if (HAL_GPIO_ReadPin(JoyStick_Input_GPIO_Port, JoyStick_Input_Pin) == 0 && last_state_press != SELECT)
	{
		current_location = SELECT;
		osMessageQueuePut(directionQueueHandle, &current_location, 1, 0);
		last_state_press = SELECT;
	}
	else
	{
		last_state_press = NONE;
	}
}



void logic_handler(void)
{
	osStatus_t status = osMessageQueueGet(directionQueueHandle, &command, NULL, osWaitForever);
	if ((status == osOK))
	{
		switch (command) {
		case UP:
			menu_index_x++;
			if (menu_index_x > 1)
			{
				menu_index_x = 0;
			}
			else if (menu_index_x < 0)
			{
				menu_index_x = 1;
			}
			printf("%d\r\n",menu_index_x);
			break;
		case DOWN:
			menu_index_x--;
			if (menu_index_x > 1)
			{
				menu_index_x = 0;
			}
			else if (menu_index_x < 0)
			{
				menu_index_x = 1;
			}
			printf("%d\r\n",menu_index_x);
			break;
		case LEFT:
			menu_index_y++;
			if (menu_index_y > 1)
			{
				menu_index_y = 0;
			}
			else if (menu_index_y < 0)
			{
				menu_index_y = 1;
			}
			printf("%d\r\n",menu_index_y);
			break;
		case RIGHT:
			menu_index_y--;
			if (menu_index_y < 0)
			{
				menu_index_y = 1;
			}
			else if (menu_index_y > 1)
			{
				menu_index_y = 0;
			}
			printf("%d\r\n",menu_index_y);
			break;
		case SELECT:
			if ((menu_index_select == 0))
			{
				printf("Selected\r\n");
				menu_index_select++;
			}
			else
			{
				menu_index_select = 0;
			}
		}
	}
}
/* USER CODE END Application */

