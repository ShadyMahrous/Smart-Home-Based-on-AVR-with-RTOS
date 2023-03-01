/*
 * mian.c
 *
 *  Created on: Jun 23, 2022
 *  Author:-
 *  		1- Hassan Ehab Al-Adgham
 *  		2- Mohamed Wageeh Mohamed
 *  		3- Mostafa Mohamed Momieh
 *  		4- Saif eldeen Mohamed
 *  		5- Shady Gamal Mahrous
 *  		6- Yasmin Mostafa Abdelazeem
 */

/************************************ Include Files ********************************************/
/*===========   INCLUDE LIB   ===========*/
#include "LIB/LBIT_MATH.h"
#include "LIB/LSTD_TYPES.h"
/*======================================*/
/*===========  INCLUDE MCAL   ===========*/
#include "MCAL/MDIO/MDIO_Interface.h"
#include "MCAL/MGIE/MGIE_Interface.h"
#include "MCAL/MADC/MADC_Interface.h"
/*======================================*/
/*===========  INCLUDE HAL   ===========*/
#include "HALL/HCLCD/HCLCD_Interface.h"
#include "HALL/KEY_PAD/HKPD_Interface.h"
/*======================================*/
/*===========  INCLUDE APP   ===========*/
#include "Free_RTOS/FreeRTOS.h"
#include "Free_RTOS/task.h"
#include "Free_RTOS/semphr.h"
/*======================================*/
#define NOT_PRESSEDD         '/'
#define N_OF_SENSORS		  2
/******************************** Declare Global variable ********************************/
static u16 Digital[N_OF_SENSORS] = {0};		 /* 0 -1023*/
static f32 Analog1 = 0; 						 /*0 - 5000 */
static f32 Analog2 = 0; 						 /*0 - 5000 */
static f32  Temp = 0;
u8 CHANNEL_Num = 0 ;
u8 i = 0 ;
u8 SW_flag = 0 ;
u8 wrongInputRemain = 2 ;
u8 password[]={'1','2'};
u8 enter_password[5]={0};
u8 Flag = 0 ;
u8 Clear = 0;
/************************************* Tasks Prototypes***********************************/
void Task_Pass(void*pv);
void Task_WelcomeTap(void*pv);
void Task_LCD(void*pv);
void taskADC_Read(void *pv);
void task_PotCal(void *pv);
void task_TempCal(void *pv);
/*************************** Main *****************************/
int main(void)
{
	/* Modules Initialization */
	MDIO_Error_State_SetPinDirection(PIN0,MDIO_PORTC,PIN_OUTPUT);//LED1
	MDIO_Error_State_SetPinDirection(PIN1,MDIO_PORTC,PIN_OUTPUT);//LED2
	MDIO_Error_State_SetPinDirection(PIN2,MDIO_PORTC,PIN_OUTPUT);//LED3

	MDIO_Error_State_SetPinDirection(PIN5,MDIO_PORTC,PIN_OUTPUT); //Buzzer

	MDIO_Error_State_SetPinDirection(PIN0,MDIO_PORTD,PIN_OUTPUT);//FAN MOTOR

	MDIO_Error_State_SetPinDirection(PIN6,MDIO_PORTD,PIN_OUTPUT);//WINDOW MOTOR
	MDIO_Error_State_SetPinDirection(PIN7,MDIO_PORTD,PIN_OUTPUT);//WINDOW MOTOR

	MDIO_Error_State_SetPinDirection(PIN0,MDIO_PORTB,PIN_OUTPUT);//DOOR MOTOR
	MDIO_Error_State_SetPinDirection(PIN1,MDIO_PORTB,PIN_OUTPUT);//DOOR MOTOR

	/**********************************************************
	 	 *  Create Tasks :
		 *  1- Task_Name
		 *  2-PC(Debug)_Name
		 *  3- Stack depth
		 *  4- Task parameter
		 *  5- priority
		 *  6- Task handler
	*************************************************************/
	xTaskCreate(Task_Pass,NULL,100,NULL,2,NULL);
	xTaskCreate(Task_WelcomeTap,NULL,100,NULL,2,NULL);
	xTaskCreate(Task_LCD,NULL,300,NULL,3,NULL);
	xTaskCreate(taskADC_Read,NULL,100,NULL,2,NULL);
	xTaskCreate(task_PotCal,NULL,100,NULL,2,NULL);
	xTaskCreate(task_TempCal,NULL,100,NULL,2,NULL);

	/****************************************************
	 * Init Peripherals
	 *  1- LCD Init
	 *  2- Keypad Init
	 *  3- ADC Init
	 ***********************************************/
	HCLCD_Vid4Bits_Init();
	HKPD_VidInit();
	MADC_VidInit();

	/* Start ADC */
	MADC_u16ADC_StartConversion_With_Interrupt(CHANNEL_Num);

	/* call the scheduler */
	vTaskStartScheduler();

	while(1);

	return 0;
}

/*************************** Tasks ***************************
 * Task_Pass :
 *
 *  1-Take Password
 *  2-Check Password
 *
 **************************************************************/
void Task_Pass(void*pv)
{
	while (1)
	{
		if( Flag == 0 && SW_flag == 0)
		{
			static u8 Local_u8counterpassword = 0;
			u8 check = 0 ;
			u8 Local_u8KeyPressed = NOT_PRESSEDD;
			Local_u8KeyPressed = HKPD_U8GetKeyPressed() ;

			if (Local_u8KeyPressed != NOT_PRESSEDD) // NOT_PRESSED = '/'
			{
				if (Local_u8KeyPressed >='0' && Local_u8KeyPressed <='9') //enter password
				{
					enter_password[Local_u8counterpassword] = Local_u8KeyPressed;
					Local_u8counterpassword++;
					MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTC,PIN_HIGH);//led
					SW_flag = 1;
				}

				else if('#' == Local_u8KeyPressed)  //Check Password
				{
					i = 0;
					for(Local_u8counterpassword=0;Local_u8counterpassword<2;Local_u8counterpassword++)
					{
						if (enter_password[Local_u8counterpassword]	!= password[Local_u8counterpassword])
						{check=1 ; }
					}
					Local_u8counterpassword = 0 ;

					if( check == 0) 			//right password
					{
						wrongInputRemain = 2 ;
						Clear = 0;
						Flag = 1 ;
					}
					else if( check != 0)		//wrong password
					{
						check = 0 ;
						Clear = 0;
						Flag = 2 ;
					}

				}
				Local_u8KeyPressed = NOT_PRESSEDD ;
			}
		}
		vTaskDelay(10);
	}
}
/**************************** WelcomeTap  Function ***********************************
 *  Task_WelcomeTap
 * 1- if Password is Right -->  Flag = 1 -->  open door (Motor ON and Led ON)
 *************************************************************************************/
void Task_WelcomeTap(void*pv)
{
	static u8 FLAG1 = 1 ;
	while (1)
	{
		if ( Flag == 1 )
		{
			/********Right password Open door and Light*********/
			if(FLAG1 == 1)
			{
				MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTC,PIN_HIGH);//Open led door (motor)
				MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTB,PIN_HIGH);//MOTOR OPEN DOOR
				vTaskDelay(600);
				MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTB,PIN_LOW);//MOTOR CLOSE DOOR
				vTaskDelay(300);
				MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTC,PIN_HIGH);//Open led door (motor)
				vTaskDelay(600);
				MDIO_Error_State_SetPinValue(PIN1,MDIO_PORTB,PIN_LOW);//STOP MOTOR
				MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTC,PIN_LOW);//Open led door (motor)
				FLAG1 = 0;
			}
		}
		vTaskDelay(200);
	}
}
/**************************** LCD Function ***********************************
	 * Task_LCD
	 *
	 * LCD Display All Project Features :
	 *
	 * IF Flag = 0  --> Display Enter Password
	 *
	 * IF Password Right -->  Flag = 1 -> Display D Open , Temp = ... C , Gas = ... G , Secured
	 * IF password Wrong -->  Flag = 2 -> Display no.Tries , Then Calling Ploice  , Buzzer ON , Unsecured
	 *
 ****************************************************************************/
void Task_LCD(void*pv)
{
	while (1)
	{
		if ( Flag == 0 )
		{
			if ( Clear == 0 )
			{HCLCD_VidWriteCommand_4Bits(0x01>>4);
			HCLCD_VidWriteCommand_4Bits(0x01);}
			Clear = 1;
			u8 string1[]={"Enter Password"};
			HCLCD_VidSetPosition_4BitsMode(1, 0);
			HCLCD_VidWriteString_4Bits(string1);
			vTaskDelay(50);
			if (SW_flag==1) //enter password
			{

			HCLCD_VidSetPosition_4BitsMode(2, i);
			HCLCD_VidWriteString_4Bits((u8*)"*");
			i++;
			MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTC,PIN_LOW);//led
			SW_flag = 0;
			}
			vTaskDelay(30);
		}

		if ( Flag == 1 )
		{
			if ( Clear == 0 )
			{HCLCD_VidWriteCommand_4Bits(0x01>>4);
			HCLCD_VidWriteCommand_4Bits(0x01);}
			Clear = 1;
			u8 string1[]={"D  Open"};
			u8 string2[]={"V="};
			u8 string3[]={"Secured   "};
			u8 string4[]={"T="};

			HCLCD_VidSetPosition_4BitsMode(1, 0);
			HCLCD_VidWriteString_4Bits(string1);

			HCLCD_VidSetPosition_4BitsMode(1, 8);
			HCLCD_VidWriteString_4Bits(string2);

			HCLCD_VidSetPosition_4BitsMode(1,15);
			HCLCD_VidWriteString_4Bits((u8*)"G");

			HCLCD_VidSetPosition_4BitsMode(2, 0);
			HCLCD_VidWriteString_4Bits(string3);

			HCLCD_VidSetPosition_4BitsMode(2, 8);
			HCLCD_VidWriteString_4Bits(string4);

			HCLCD_VidSetPosition_4BitsMode(2,14);
			HCLCD_VidWriteString_4Bits((u8*)"C");


			HCLCD_VidSetPosition_4BitsMode(1,10);
			HCLCD_VidWriteString_4Bits((u8*)"    ");
			HCLCD_VidSetPosition_4BitsMode(1,10);
			HCLCD_VidWriteNumber_4Bits(Analog1);


			HCLCD_VidSetPosition_4BitsMode(2,10);
			HCLCD_VidWriteString_4Bits((u8*)"  ");
			HCLCD_VidSetPosition_4BitsMode(2,10);
			HCLCD_VidWriteNumber_4Bits(Temp);

			vTaskDelay(50);
		}

		if  (Flag == 2)
		{
			if ( Clear == 0 )
			{HCLCD_VidWriteCommand_4Bits(0x01>>4);
			HCLCD_VidWriteCommand_4Bits(0x01);}
			Clear = 1;

			if(wrongInputRemain > 0)
			{
			HCLCD_VidSetPosition_4BitsMode(2, 6);
			HCLCD_VidWriteNumber_4Bits(wrongInputRemain);
			wrongInputRemain -- ;
			HCLCD_VidSetPosition_4BitsMode(2, 8);
			HCLCD_VidWriteString_4Bits((u8*)"Tries");
			Flag = 0 ;
			}
			else
			{
			HCLCD_VidSetPosition_4BitsMode(1, 0);
			HCLCD_VidWriteString_4Bits((u8*)"Calling Police");
			HCLCD_VidSetPosition_4BitsMode(2, 4);
			HCLCD_VidWriteString_4Bits((u8*)"Unsecured");
			MDIO_Error_State_SetPinValue(PIN5,MDIO_PORTC,PIN_HIGH);
			vTaskDelay(50);
			}
		}
		vTaskDelay(20);
	}
}
/*****************************************************************
*  Task_ADC
* ADC take Reading and convert it to Digital for Temp and GAS Sensors
********************************************************************/
	void taskADC_Read(void *pv)
	{
		u8 counter = 0 ;
		while(1)
		{
			if( Flag == 1 )
			{
					/* Read the Digital (0 - 1024) */
					Digital[counter] = MADC_u16ADCRead();
					if(CHANNEL_Num == 7 )
					{
						CHANNEL_Num = 0;
					}
					else if (CHANNEL_Num == 0)
					{
						CHANNEL_Num = 7 ;
					}
					MADC_u16ADC_StartConversion_With_Interrupt(CHANNEL_Num); //new conversion After Reading the Last conversion
					counter ++ ;
					if(counter == 2)
					{
						counter = 0;
					}
			}
			vTaskDelay(10);
		}

	}
/*****************************************************************
* Task_Gas Sensor
* IF Gas Reading  > 2500 ---> Buzzer ON , Window Open (MOTOR ON)
* IF Gas Reading < 2500 ---> Buzzer OFF , Window Close (MOTOR OFF)
* ***************************************************************/
	void task_PotCal(void *pv)
	{
		static u8 FLAG = 0;
		while(1)
		{
			Analog1 = (Digital[0]*5000UL) / 1024 ;
			vTaskDelay(15);
			if((Analog1<=2500)&&(FLAG == 1))
			{
				FLAG = 0;
				MDIO_Error_State_SetPinValue(PIN5,MDIO_PORTC,PIN_LOW);///gaz BUZZER
				MDIO_Error_State_SetPinValue(PIN2,MDIO_PORTC,PIN_HIGH);//led
				MDIO_Error_State_SetPinValue(PIN7,MDIO_PORTD,PIN_HIGH);//MOTOR CLOSE WINDO
				vTaskDelay(600);
				MDIO_Error_State_SetPinValue(PIN7,MDIO_PORTD,PIN_LOW);//STOP MOTOR
				MDIO_Error_State_SetPinValue(PIN2,MDIO_PORTC,PIN_LOW);//led
			}
			else if((Analog1>2500)&&(FLAG == 0))
				{
				FLAG = 1;
				MDIO_Error_State_SetPinValue(PIN5,MDIO_PORTC,PIN_HIGH);///gaz BUZZER
				MDIO_Error_State_SetPinValue(PIN2,MDIO_PORTC,PIN_HIGH);//led
				MDIO_Error_State_SetPinValue(PIN6,MDIO_PORTD,PIN_HIGH);//MOTOR OPEN WINDO
				vTaskDelay(600);
				MDIO_Error_State_SetPinValue(PIN6,MDIO_PORTD,PIN_LOW);//STOP MOTOR
				MDIO_Error_State_SetPinValue(PIN2,MDIO_PORTC,PIN_LOW);//led
			}
			vTaskDelay(15);
		}
	}

/*****************************************************************
 *  Temperature Sensor :
 *
 * IF Temp Reading  > 30 ---> FAN ON (MOTOR ON) and Led ON
 * IF Temp Reading < 30 ----> FAN Off (MOTOR OFF) and Led Off
 * **********************************************************/
	void task_TempCal(void *pv)
	{
		while(1)
		{
			Analog2 = ((float)Digital[1]*5000.0) / 1024.0 ;
			Temp = round (Analog2 / 10 );
			if(Temp<=30)
			{
				MDIO_Error_State_SetPinValue(PIN1,MDIO_PORTC,PIN_LOW);//LED
				MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTD,PIN_LOW);//STOP MOTOR
			}
			else if(Temp>30)
			{
				MDIO_Error_State_SetPinValue(PIN1,MDIO_PORTC,PIN_HIGH);//LED
				MDIO_Error_State_SetPinValue(PIN0,MDIO_PORTD,PIN_HIGH);//MOTOR on
			}
			vTaskDelay(15);
		}
	}
/***************************************************************/


