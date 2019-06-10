/*Jimmy Im : jim011@ucr.edu
 *Lab Section: 024
 *Assignment: Final Project
 *
 *I acknowledge all content contained herein, excluding template or example
 *code, is my own original work.
 */

#define F_CPU 20000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <bit.h>
#include <timer.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "nokia5110.h"

#define NOTE_B0  31.0
#define NOTE_C1  33.0
#define NOTE_CS1 35.0
#define NOTE_D1  37.0
#define NOTE_DS1 39.0
#define NOTE_E1  41.0
#define NOTE_F1  44.0
#define NOTE_FS1 46.0
#define NOTE_G1  49.0
#define NOTE_GS1 52.0
#define NOTE_A1  55.0
#define NOTE_AS1 58.0
#define NOTE_B1  62.0
#define NOTE_C2  65.0
#define NOTE_CS2 69.0
#define NOTE_D2  73.0
#define NOTE_DS2 78.0
#define NOTE_E2  82.0
#define NOTE_F2  87.0
#define NOTE_FS2 93.0
#define NOTE_G2  98.0
#define NOTE_GS2 104.0
#define NOTE_A2  110.0
#define NOTE_AS2 117.0
#define NOTE_B2  123.0
#define NOTE_C3  131.0
#define NOTE_CS3 139.0
#define NOTE_D3  147.0
#define NOTE_DS3 156.0
#define NOTE_E3  165.0
#define NOTE_F3  175.0
#define NOTE_FS3 185.0
#define NOTE_G3  196.0
#define NOTE_GS3 208.0
#define NOTE_A3  220.0
#define NOTE_AS3 233.0
#define NOTE_B3  247.0
#define NOTE_C4  262.0
#define NOTE_CS4 277.0
#define NOTE_D4  294.0
#define NOTE_DS4 311.0
#define NOTE_E4  330.0
#define NOTE_F4  349.0
#define NOTE_FS4 370.0
#define NOTE_G4  392.0
#define NOTE_GS4 415.0
#define NOTE_A4  440.0
#define NOTE_AS4 466.0
#define NOTE_B4  494.0
#define NOTE_C5  523.0
#define NOTE_CS5 554.0
#define NOTE_D5  587.0
#define NOTE_DS5 622.0
#define NOTE_E5  659.0
#define NOTE_F5  698.0
#define NOTE_FS5 740.0
#define NOTE_G5  784.0
#define NOTE_GS5 831.0
#define NOTE_A5  880.0
#define NOTE_AS5 932.0
#define NOTE_B5  988.0
#define NOTE_C6  1047.0
#define NOTE_CS6 1109.0
#define NOTE_D6  1175.0
#define NOTE_DS6 1245.0
#define NOTE_E6  1319.0
#define NOTE_F6  1397.0
#define NOTE_FS6 1480.0
#define NOTE_G6  1568.0
#define NOTE_GS6 1661.0
#define NOTE_A6  1760.0
#define NOTE_AS6 1865.0
#define NOTE_B6  1976.0
#define NOTE_C7  2093.0
#define NOTE_CS7 2217.0
#define NOTE_D7  2349.0
#define NOTE_DS7 2489.0
#define NOTE_E7  2637.0
#define NOTE_F7  2794.0
#define NOTE_FS7 2960.0
#define NOTE_G7  3136.0
#define NOTE_GS7 3322.0
#define NOTE_A7  3520.0
#define NOTE_AS7 3729.0
#define NOTE_B7  3951.0
#define NOTE_C8  4186.0
#define NOTE_CS8 4435.0
#define NOTE_D8  4699.0
#define NOTE_DS8 4978.0

// 0.954 hz is lowest frequency possible with this function,
// based on settings in PWM_on()
// Passing in 0 as the frequency will stop the speaker from generating sound
void set_PWM(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR0B &= 0x08; } //stops timer/counter
		else { TCCR0B |= 0x03; } // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR0A = 0xFFFF; }
		
		// prevents OCR0A from underflowing, using prescaler 64                    // 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR0A = 0x0000; }
		
		// set OCR3A based on desired frequency
		else { OCR0A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT0 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on() {
	TCCR0A = (1 << WGM02) | (1 << WGM00) | (1 << COM0A0);
	// COM3A0: Toggle PB3 on compare match between counter and OCR0A
	TCCR0B = (1 << WGM02) | (1 << CS01) | (1 << CS00);
	// WGM02: When counter (TCNT0) matches OCR0A, reset counter
	// CS01 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR0A = 0x00;
	TCCR0B = 0x00;
}


//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
    unsigned long int c;
    while(1){
        c = a%b;
        if(c==0){return b;}
        a = b;
		b = c;
    }
    return 0;
}
//--------End find GCD function ----------------------------------------------

//global varibles
unsigned short SNESOutput = 0x0000;
//unsigned char LEDSpecial = 0x00;

//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
    /*Tasks should have members that include: state, period,
        a measurement of elapsed time, and a function pointer.*/
    unsigned char state; //Task's current state
    unsigned long int period; //Task period
    unsigned long int elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

enum SNESStates {SNESStart, SNESInit, SNESWait, SNESLatch, SNESLatchAfter, SNESData, SNESClock, SNESFinish};
const unsigned short SNESDELAYWAIT = 6;
const unsigned short SNESDELAYCLK = 16;

unsigned short countSNESControl;
unsigned short dataSNESControl;
unsigned char TickSNESControl(unsigned char state) {
	
	unsigned char SNESPins;
	unsigned char S2;
	unsigned char S3;
	unsigned short S4;
	//LEDSpecial = 0x00;
	SNESPins = (~PINA);
	S4 = (SNESPins & 0x0004) >> 2;
	//Transition
	switch (state) {
		case SNESStart:
		state = SNESInit;
		break;
		
		case SNESInit:
		state = SNESWait;
		break;
		
		case SNESWait:
		if(countSNESControl >= SNESDELAYWAIT) {
			state = SNESLatch;
		}
		//LEDSpecial = 0xF0;
		break;
		
		case SNESLatch:
		state = SNESLatchAfter;
		break;
		
		case SNESLatchAfter:
		state = SNESData;
		break;
		
		case SNESData:
		state = SNESClock;
		break;
		
		case SNESClock:
		if(countSNESControl >= SNESDELAYCLK) {
			state = SNESFinish;
		}
		else {
			state = SNESData;
		}
		break;
		
		case SNESFinish:
		state = SNESWait;
		break;
		
		default:
		state = SNESStart;
		break;
	}
	//Action
	switch (state) {
		case SNESInit:
		countSNESControl = 0;
		SNESOutput = 0;
		dataSNESControl = 0;
		break;
		
		case SNESWait:
		countSNESControl++;
		dataSNESControl = 0x0000;
		S3 = 0;
		S2 = 0;
		break;
		
		case SNESLatch:
		S3 = 1;
		S2 = 0;
		countSNESControl = 0;
		break;
		
		case SNESLatchAfter:
		S3 = 0;
		S2 = 0;
		break;
		
		case SNESData:
		S2 = 0;
		S3 = 0;
		dataSNESControl = dataSNESControl | S4;
		break;
		
		case SNESClock:
		countSNESControl++;
		S2 = 1;
		S3 = 0;
		if(countSNESControl < SNESDELAYCLK) {
			dataSNESControl = dataSNESControl<<1;
		}
		break;
		
		case SNESFinish:
		countSNESControl = 0;
		SNESOutput = dataSNESControl;
		break;
		
		default:
		break;
	}
	S3 = S3 << 1;
	S4 = S4 << 2;
	SNESPins = 0x00 | S2 | S3 | S4;
	PORTA = SNESPins;
	return state;
}


enum LEDOutputStates {LEDStart, LEDDisplay};
	
unsigned char TickLED(unsigned char state) {
	unsigned short tempLED;
	static unsigned char LEDB;
	static unsigned LEDD;
	switch(state) {
		case LEDStart:
		state = LEDDisplay;
		break;
		
		case LEDDisplay:
		break;
		
		default:
		state = LEDStart;
		break;
	}
	switch(state) {
		case LEDDisplay:
		tempLED = SNESOutput;
		LEDB = (tempLED & 0x00FF);
		LEDD = (tempLED & 0xFF00) >> 8;
		//LEDD = LEDD | LEDSpecial;
		PORTD = LEDD;
		PORTB = LEDB;
		break;
		
		default:
		break;
	}
	
	return state;
}

const unsigned short BOMBPERIOD = 30;

struct playerObj {
	unsigned char playerPosX;
	unsigned char playerPosY;
	unsigned char currentGraphic;
	
	unsigned char isBombPlaced;
	unsigned char bombPosX;
	unsigned char bombPosY;
	unsigned short bombCount;
} player1;

enum objectType {OBJEmpty, OBJWall, OBJHidden, OBJDoor, OBJPlayer, OBJEnemy, OBJBomb, OBJBlock,OBJExplode};
unsigned char objectLocMatrix[3][7];

unsigned short bombperson_img[12] = 
{0x03FC,0x07FE,0x0D1C,0x0999,
 0x0999,0x030C,0x07FE,0x03FC,
 0x0090,0x0090,0x06F6,0x0F0F};
 
 unsigned short bomb_img[12] =
 {0x001C,0x0016,0x00FA,0x013A,
	 0x02F4,0x07EE,0x07FE,0x059E,
 0x049E,0x027C,0x01F8,0x00F0};
 
 unsigned short wall_img[12] =
 {0x07AE,0x0F5F, 0x0FAF,
	 0x0AFA,0x07F7, 0x0AFA,
	 0x0FAF,0x0F7F, 0x0FAF,
 0x0AFA,0x07F7, 0x02FA};
 
 unsigned short enemy_img[12] =
 {0x03FC, 0x0606, 0x0C03,
	 0x0B9D, 0x0B9D, 0x0999,
	 0x0C03, 0x070E, 0x01F8,
 0x0158, 0x01A8, 0x00F0};

 unsigned short nobreak_img[12] =
 {0x03FC, 0x0606, 0x0C03,
	 0x0B9D, 0x0B9D, 0x0999,
	 0x0C03, 0x070E, 0x01F8,
 0x0158, 0x01A8, 0x00F0};
 
 unsigned short block_img[12] =
 {0x0AAB, 0x0557, 0x03FF,
	 0x020F, 0x0217, 0x020F,
	 0x0207, 0x022F, 0x0257,
 0x03FF, 0x0557, 0x0AAB};
 
 unsigned short explode_img[12] =
 {0x03EC, 0x0412, 0x402,
	 0x0402, 0x0802, 0x0801,
	 0x0801, 0x0801, 0x0803,
 0x0402, 0x0264, 0x03B8};

 unsigned short door_img[12] =
 {0x01F8, 0x0264, 0X0462,
	 0x0462, 0x0861, 0x0861,
	 0x0861, 0x0B6D, 0x0861,
 0x0861, 0x0861, 0x0FFF};

unsigned char levelDatabase[3][3][7]= {
					{{OBJPlayer, OBJEmpty, OBJWall, OBJEmpty, OBJEmpty, OBJEmpty, OBJEmpty},
					{OBJEmpty, OBJBlock, OBJEmpty, OBJBlock, OBJEmpty, OBJBlock, OBJWall},
					{OBJHidden, OBJEmpty, OBJEmpty, OBJEmpty, OBJEmpty, OBJEmpty, OBJEmpty}},
						
					{{OBJPlayer, OBJEmpty, OBJEmpty, OBJEmpty, OBJWall, OBJEmpty, OBJEmpty},
					{OBJEmpty, OBJBlock, OBJEmpty, OBJBlock, OBJEmpty, OBJBlock, OBJWall},
					{OBJEmpty, OBJWall, OBJEmpty, OBJHidden, OBJEmpty, OBJEmpty, OBJEmpty}},
	
					{{OBJPlayer, OBJEmpty, OBJWall, OBJWall, OBJEmpty, OBJEmpty, OBJEmpty},
					{OBJEmpty, OBJBlock, OBJEmpty, OBJBlock, OBJEmpty, OBJBlock, OBJEmpty},
					{OBJEmpty, OBJEmpty, OBJEmpty, OBJEnemy, OBJEmpty, OBJEmpty, OBJHidden}}
					};
unsigned char displayMatrix[48][84];

const unsigned TILESIZE = 12;
void transferObjToDis() {
	unsigned short tempTileData = 0x0000;
	for(unsigned char y = 0; y < 3; y++) {
		for(unsigned char x = 0; x < 7; x++) {
			switch(objectLocMatrix[y][x]) {
				case OBJEmpty:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = 0x0000;
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				case OBJWall:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = wall_img[tileY];
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				case OBJHidden:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = wall_img[tileY];
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				case OBJDoor:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = door_img[tileY];
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				case OBJBlock:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = block_img[tileY];
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				case OBJPlayer:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = bombperson_img[tileY];
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				case OBJBomb:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = bomb_img[tileY];
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				case OBJEnemy:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = enemy_img[tileY];
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				case OBJExplode:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					tempTileData = explode_img[tileY];
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0x01 & tempTileData;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
				
				default:
				for(unsigned char tileY = 0; tileY < TILESIZE; tileY++) {
					for(unsigned char tileX = 0; tileX < TILESIZE; tileX++) {
						displayMatrix[y*TILESIZE + tileY][x*TILESIZE + tileX] = 0;
						tempTileData = tempTileData >> 1;
					}
				}
				break;
			}
		}
	}
}

unsigned char vibrateQueue[3];
char vibrateQueueSize = 0;
char vibrateQueueStart = 0;
char vibrateQueueEnd = 0;
const char VIB_QUEUE_MAX = 3;
unsigned short vibrateDuration;
unsigned short vibrateQueueCount;

unsigned short PWMCount = 0;
unsigned char tempPWM;
const unsigned short PWMOnPERIOD = 5;


unsigned char rumbleMes = 0;
unsigned char rumbleOn = 0;
unsigned char tempC = 0;
enum PWMMotorState {PWMMotorStart, PWMMotorInit, PWMMotorWait, PWMMotorGetQueueData, PWMMotorOn, PWMMotorOff};

unsigned char TickPWMMotor(unsigned char state) {
	switch(state) {
		case PWMMotorStart:
		state = PWMMotorInit;
		break;
		
		case PWMMotorInit:
		
		state = PWMMotorOn;
		break;
		
		case PWMMotorOn:
		if(PWMCount >= PWMOnPERIOD) {
			state = PWMMotorOff;
			PWMCount = 0;
		}
		break;
		
		case PWMMotorOff:
		if(PWMCount >= 5) {
			state = PWMMotorOn;
			PWMCount = 0;
		}
		break;
		
		default:
		state = PWMMotorStart;
		break;
	}
	switch(state) {
			case PWMMotorInit:
			tempC = 0;
			break;
			
			case PWMMotorOn:
			if(rumbleOn) {
				tempC = 0xFFFF;
				rumbleMes = 1;
			}
			else {
				tempC = 0x0000;
				rumbleMes = 0;
			}
			PWMCount = PWMCount + 1;
			break;
			
			case PWMMotorOff:
			if(rumbleOn) {
				tempC = 0x0000;
				rumbleMes = 1;
			}
			else {
				tempC = 0x0000;
				rumbleMes = 0;
			}
			PWMCount = PWMCount + 1;
			break;
			
			default:
			break;
	}
	PORTC = tempC;
	return state;
}


enum PWMMotorManagerState {PWMMotorManagerStart, PWMMotorManagerInit, PWMMotorManagerWait, PWMMotorManagerGetQueueData, PWMMotorManagerCount, PWMMotorManagerFinish};
unsigned char TickPWMMotorManager(unsigned char state) {
	switch(state) {
		case PWMMotorManagerStart:
		state = PWMMotorManagerInit;
		break;
		
		case PWMMotorManagerInit:
		state = PWMMotorManagerWait;
		break;
		
		case PWMMotorManagerWait:
		if(vibrateQueueSize > 0) {
			state = PWMMotorGetQueueData;
		}
		break;
		
		case PWMMotorManagerGetQueueData:
		state = PWMMotorManagerCount;
		break;
		
		case PWMMotorManagerCount:
		if(vibrateDuration <= vibrateQueueCount) {
			state = PWMMotorManagerFinish;
		}
		break;
		
		case PWMMotorManagerFinish:
		state = PWMMotorManagerWait;
		break;
		
		default:
		state = PWMMotorManagerStart;
		break;
	}
	switch(state) {
		case PWMMotorManagerInit:
		vibrateQueueSize = 0;
		vibrateQueueStart = 0;
		vibrateQueueEnd = 0;
		vibrateDuration = 0;
		break;
		
		case PWMMotorManagerWait:
		rumbleOn = 0;
		break;
		
		case PWMMotorManagerGetQueueData:
		vibrateDuration = vibrateQueue[vibrateQueueStart];
		vibrateQueueCount = 0;
		break;
		
		case PWMMotorManagerCount:
		rumbleOn = 1;
		vibrateQueueCount = vibrateQueueCount + 1;
		break;
		
		case PWMMotorManagerFinish:
		rumbleOn = 0;
		if(vibrateQueueStart >= 2) {
			vibrateQueueStart = 0;
		}
		else {
			vibrateQueueStart = vibrateQueueStart + 1;
		}
		vibrateQueueSize = vibrateQueueSize - 1;
		vibrateQueueCount = 0;
		break;
		
		default:
		break;
	}
	return state;
}

struct explodeNode {
	unsigned char posX;
	unsigned char posY;
};

struct explodeNode explodeStack[10];
unsigned char explodeStackSize;

unsigned short highScore;
uint16_t* EEPROM_ADDRESS_0 = 0x0000;

unsigned short menuInputDelay = 10;
unsigned short menuInputDelayCount = 0;

unsigned short tempScore;
unsigned char displayScore[3];

unsigned short gameTimer;
unsigned char gameTimerCountSecond;
const unsigned char gameTimerCountSecondPeriod = 10;
const unsigned short ROUNDPERIOD = 20;
unsigned char displayGameTimer[3];

unsigned char playerLives;
const unsigned char LIVESAMOUNT = 1;

unsigned char gameOver;
unsigned char levelFinish;
unsigned char levelCount;
const unsigned char LEVELMAX = 3;

//Sound
double soundQueue[3];
char soundQueueSize = 0;
char soundQueueStart = 0;
char soundQueueEnd = 0;
const char SOUND_QUEUE_MAX = 3;
const unsigned short soundDuration = 3;
unsigned short soundQueueCount;

void gamePlayingPlayerActions(){
	
	//Player Input and Actions
	if(player1.isBombPlaced == 0 && (SNESOutput & 0x8000) == 0x8000) {
		player1.isBombPlaced = 1;
		player1.bombPosX = player1.playerPosX;
		player1.bombPosY = player1.playerPosY;
		player1.bombCount = 0;
	}
	unsigned char tempObj;
	if(player1.playerPosX != 6 && (SNESOutput & 0x0100) == 0x0100) {
		tempObj = objectLocMatrix[player1.playerPosY][(player1.playerPosX + 1)];
		if(tempObj == OBJEmpty){
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
			player1.playerPosX = player1.playerPosX + 1;
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
		}
		else if(tempObj == OBJDoor) {
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
			player1.playerPosX = player1.playerPosX + 1;
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
			levelFinish = 1;
		}
		else if(tempObj == OBJEnemy) {
			gameOver = 1;
		}
	}
	else if(player1.playerPosX != 0 && (SNESOutput & 0x0200) == 0x0200) {
		tempObj = objectLocMatrix[player1.playerPosY][(player1.playerPosX - 1)];
		if(tempObj == OBJEmpty){
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
			player1.playerPosX = player1.playerPosX - 1;
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
		}
		else if(tempObj == OBJDoor) {
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
			player1.playerPosX = player1.playerPosX - 1;
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
			levelFinish = 1;
		}
		else if(tempObj == OBJEnemy) {
			gameOver = 1;
		}
	}
	else if(player1.playerPosY != 0 && (SNESOutput & 0x0800) == 0x0800) {
		tempObj = objectLocMatrix[player1.playerPosY - 1][(player1.playerPosX)];
		if(tempObj == OBJEmpty){
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
			player1.playerPosY = player1.playerPosY - 1;
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
		}
		else if(tempObj == OBJDoor) {
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
			player1.playerPosY = player1.playerPosY - 1;
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
			levelFinish = 1;
		}
		else if(tempObj == OBJEnemy) {
			gameOver = 1;
		}
	}
	else if(player1.playerPosY != 2 && (SNESOutput & 0x0400) == 0x0400) {
		tempObj = objectLocMatrix[player1.playerPosY + 1][(player1.playerPosX)];
		if(tempObj == OBJEmpty){
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
			player1.playerPosY = player1.playerPosY + 1;
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
		}
		else if(tempObj == OBJDoor) {
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
			player1.playerPosY = player1.playerPosY + 1;
			objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
			levelFinish = 1;
		}
		else if(tempObj == OBJEnemy) {
			gameOver = 1;
		}
	}
	if((player1.playerPosX != player1.bombPosX || player1.playerPosY != player1.bombPosY) && player1.isBombPlaced != 0) {
		objectLocMatrix[player1.bombPosY][player1.bombPosX] = OBJBomb;
	}
}

void gamePlayingBombCheck() {
	unsigned char tempObj;
	
	
	//Check Bomb And Explosion Objs
	while(explodeStackSize != 0) {
		objectLocMatrix[explodeStack[explodeStackSize - 1].posY][explodeStack[explodeStackSize - 1].posX] = OBJEmpty;
		explodeStackSize = explodeStackSize - 1;
	}
	
	if(player1.bombCount < BOMBPERIOD && player1.isBombPlaced != 0) {
		player1.bombCount += 1;
	}
	else if(player1.bombCount >= BOMBPERIOD && player1.isBombPlaced != 0) {
		tempObj = objectLocMatrix[player1.bombPosY][(player1.bombPosX)];
		vibrateQueue[vibrateQueueEnd] = 10;
		if(vibrateQueueEnd >= 2) {
			vibrateQueueEnd = 0;
		}
		else {
			vibrateQueueEnd = vibrateQueueEnd + 1;
		}
		vibrateQueueSize = vibrateQueueSize + 1;
		
		soundQueue[soundQueueEnd] = NOTE_B0;
		if(soundQueueEnd >= 2) {
			soundQueueEnd = 0;
		}
		else {
			soundQueueEnd = soundQueueEnd + 1;
		}
		soundQueueSize = soundQueueSize + 1;
		
		if(tempObj == OBJPlayer){
			gameOver = 1;
		}
		else {
			struct explodeNode tempExplo;
			tempExplo.posX = player1.bombPosX;
			tempExplo.posY = player1.bombPosY;
			objectLocMatrix[player1.bombPosY][(player1.bombPosX)] = OBJExplode;
			explodeStack[explodeStackSize] = tempExplo;
			explodeStackSize = explodeStackSize + 1;
		}
		
		if(player1.bombPosX != 6) {
			tempObj = objectLocMatrix[player1.bombPosY][(player1.bombPosX + 1)];
			if(tempObj == OBJEmpty || tempObj == OBJWall || tempObj == OBJEnemy){
				struct explodeNode tempExplo;
				tempExplo.posX = player1.bombPosX + 1;
				tempExplo.posY = player1.bombPosY;
				objectLocMatrix[player1.bombPosY][(player1.bombPosX + 1)] = OBJExplode;
				explodeStack[explodeStackSize] = tempExplo;
				explodeStackSize = explodeStackSize + 1;
				if(tempObj == OBJWall) {
					tempScore = tempScore + 1;
				}
			}
			else if(tempObj == OBJHidden){
				objectLocMatrix[player1.bombPosY][(player1.bombPosX + 1)] = OBJDoor;
				tempScore = tempScore + 5;
			}
			else if(tempObj == OBJPlayer){
				gameOver = 1;
			}
		}
		if(player1.bombPosX != 0) {
			tempObj = objectLocMatrix[player1.bombPosY][(player1.bombPosX - 1)];
			if(tempObj == OBJEmpty || tempObj == OBJWall || tempObj == OBJEnemy){
				struct explodeNode tempExplo;
				tempExplo.posX = player1.bombPosX - 1;
				tempExplo.posY = player1.bombPosY;
				objectLocMatrix[player1.bombPosY][(player1.bombPosX - 1)] = OBJExplode;
				explodeStack[explodeStackSize] = tempExplo;
				explodeStackSize = explodeStackSize + 1;
				if(tempObj == OBJWall) {
					tempScore = tempScore + 1;
				}
			}
			else if(tempObj == OBJHidden){
				objectLocMatrix[player1.bombPosY][(player1.bombPosX - 1)] = OBJDoor;
				tempScore = tempScore + 5;
			}
			else if(tempObj == OBJPlayer){
				gameOver = 1;
			}
		}
		if(player1.bombPosY != 0) {
			tempObj = objectLocMatrix[player1.bombPosY - 1][(player1.bombPosX)];
			if(tempObj == OBJEmpty || tempObj == OBJWall || tempObj == OBJEnemy){
				struct explodeNode tempExplo;
				tempExplo.posX = player1.bombPosX;
				tempExplo.posY = player1.bombPosY - 1;
				objectLocMatrix[player1.bombPosY - 1][(player1.bombPosX)] = OBJExplode;
				explodeStack[explodeStackSize] = tempExplo;
				explodeStackSize = explodeStackSize + 1;
				if(tempObj == OBJWall) {
					tempScore = tempScore + 1;
				}
			}
			else if(tempObj == OBJHidden){
				objectLocMatrix[player1.bombPosY - 1][(player1.bombPosX)] = OBJDoor;
				tempScore = tempScore + 5;
			}
			else if(tempObj == OBJPlayer){
				gameOver = 1;
			}
		}
		if(player1.bombPosY != 2) {
			tempObj = objectLocMatrix[player1.bombPosY + 1][(player1.bombPosX)];
			if(tempObj == OBJEmpty || tempObj == OBJWall || tempObj == OBJEnemy){
				struct explodeNode tempExplo;
				tempExplo.posX = player1.bombPosX;
				tempExplo.posY = player1.bombPosY + 1;
				objectLocMatrix[player1.bombPosY + 1][(player1.bombPosX)] = OBJExplode;
				explodeStack[explodeStackSize] = tempExplo;
				explodeStackSize = explodeStackSize + 1;
				if(tempObj == OBJWall) {
					tempScore = tempScore + 1;
				}
			}
			else if(tempObj == OBJHidden){
				objectLocMatrix[player1.bombPosY + 1][(player1.bombPosX)] = OBJDoor;
				tempScore = tempScore + 5;
			}
			else if(tempObj == OBJPlayer){
				gameOver = 1;
			}
		}
		player1.bombCount = 0;
		player1.isBombPlaced = 0;
	}
}

enum GameLogicStates{GLogicStart, GLogicInit, GLogicMenu, GLogicLevelInit, GLogicPlaying, GLogicGameOver, GLogicLevelComplete, GLogicNextLevel, GLogicRestartLevel, GLogicWin};
	
unsigned char currentGameState = GLogicMenu;
unsigned char TickGameLogic(unsigned char state) {
	switch(state) {
		case GLogicStart:
		state = GLogicInit;
		break;
		
		case GLogicInit:
		state = GLogicMenu;
		break;
		
		case GLogicMenu:
		if(((SNESOutput & 0x1000) == 0x1000) && menuInputDelayCount >= menuInputDelay){
			state = GLogicLevelInit;
		}
		break;
		
		case GLogicLevelInit:
		state = GLogicPlaying;
		break;
		
		case GLogicPlaying:
		if(levelFinish != 0 && levelCount < LEVELMAX) {
			state = GLogicLevelComplete;
		}
		else if(levelFinish != 0 && levelCount >= LEVELMAX) {
			state = GLogicWin;
		}
		else if(gameOver != 0 && playerLives > 0) {
			state = GLogicRestartLevel;
		}
		else if(gameOver != 0 && playerLives <= 0) {
			state = GLogicGameOver;
		}
		break;
		
		case GLogicNextLevel:
		state = GLogicPlaying;
		break;
		
		case GLogicLevelComplete:
		if((SNESOutput & 0x1000) == 0x1000){
			state = GLogicNextLevel;
		}
		break;
		
		case GLogicWin:
		if((SNESOutput & 0x1000) == 0x1000){
			state = GLogicMenu;
		}
		break;
		
		case GLogicRestartLevel:
		state = GLogicPlaying;
		break;
		
		case GLogicGameOver:
		if((SNESOutput &0x1000) == 0x1000){
			state = GLogicMenu;
		}
		break;
		
		default:
		state = GLogicStart;
		break;
	}
	switch(state) {
			case GLogicInit:
			//SNES_init();
			highScore = 0;
			menuInputDelayCount = 0;
			break;
			
			case GLogicMenu:
			currentGameState = GLogicMenu;
			displayScore[2] = (highScore % 10) + '0';
			displayScore[1] = ((highScore / 10) % 10) + '0';
			displayScore[0] = ((highScore / 100) % 10) + '0';
			if((SNESOutput & 0x0010) == 0x0010) {
				//Save Score = "R"
				eeprom_write_word(EEPROM_ADDRESS_0, highScore);
			}
			else if((SNESOutput & 0x0020) == 0x0020) {
				//Load Score = "L"
				highScore = eeprom_read_word(EEPROM_ADDRESS_0);
			}
			else if((SNESOutput & 0x2000) == 0x2000) {
				//Clear Score = "SELECT"
				eeprom_write_word(EEPROM_ADDRESS_0, 0x0000);
			}
			
			if(menuInputDelayCount < menuInputDelay) {
				menuInputDelayCount = menuInputDelayCount + 1;
			}
			
			break;
			
			case GLogicLevelInit:
			for(unsigned char tempY = 0; tempY < 3;tempY++) {
				for(unsigned char tempX = 0; tempX < 7;tempX++) {
					objectLocMatrix[tempY][tempX] = levelDatabase[0][tempY][tempX];
				}
			}
			
			//Reset Game values
			player1.playerPosX = 0;
			player1.playerPosY = 0;
			player1.isBombPlaced = 0;
			
			
			currentGameState = GLogicPlaying;
			levelFinish = 0;
			levelCount = 1;
			gameOver = 0;
			tempScore = 0;
			gameTimer = ROUNDPERIOD;
			gameTimerCountSecond = 0;
			playerLives = LIVESAMOUNT;
			transferObjToDis();
			break;
			
			case GLogicLevelComplete:
			currentGameState = GLogicLevelComplete;
			break;
			
			case GLogicNextLevel:
			levelCount = levelCount + 1;
			
			for(unsigned char tempY = 0; tempY < 3;tempY++) {
				for(unsigned char tempX = 0; tempX < 7;tempX++) {
					objectLocMatrix[tempY][tempX] = levelDatabase[levelCount - 1][tempY][tempX];
				}
			}
			
			
			//Reset Game values
			player1.playerPosX = 0;
			player1.playerPosY = 0;
			player1.isBombPlaced = 0;
			tempScore = tempScore + gameTimer;
			
			currentGameState = GLogicPlaying;
			levelFinish = 0;
			gameOver = 0;
			gameTimer = ROUNDPERIOD;
			gameTimerCountSecond = 0;
			transferObjToDis();
			break;
			
			case GLogicWin:
			currentGameState = GLogicWin;
			displayScore[2] = (tempScore % 10) + '0';
			displayScore[1] = ((tempScore / 10) % 10) + '0';
			displayScore[0] = ((tempScore / 100) % 10) + '0';
			if(highScore < tempScore) {
				highScore = tempScore;
			}
			menuInputDelayCount = 0;
			break;
			
			case GLogicRestartLevel:
			playerLives = playerLives - 1;
			for(unsigned char tempY = 0; tempY < 3;tempY++) {
				for(unsigned char tempX = 0; tempX < 7;tempX++) {
					objectLocMatrix[tempY][tempX] = levelDatabase[levelCount - 1][tempY][tempX];
				}
			}
			
			//Reset Game values
			player1.playerPosX = 0;
			player1.playerPosY = 0;
			player1.isBombPlaced = 0;
			
			currentGameState = GLogicPlaying;
			levelFinish = 0;
			gameOver = 0;
			gameTimer = ROUNDPERIOD;
			gameTimerCountSecond = 0;
			transferObjToDis();
			break;
			
			case GLogicPlaying:
			
			gamePlayingPlayerActions();
			gamePlayingBombCheck();
			
			transferObjToDis();
			
			displayScore[2] = (tempScore % 10) + '0';
			displayScore[1] = ((tempScore / 10) % 10) + '0';
			displayScore[0] = ((tempScore / 100) % 10) + '0';
			if(gameTimerCountSecond >= gameTimerCountSecondPeriod) {
				gameTimer = gameTimer - 1;
				if(gameTimer == 0) {
					gameOver = 1;
				}
				gameTimerCountSecond = 0;
			}
			else {
				gameTimerCountSecond = gameTimerCountSecond + 1;
			}
			displayGameTimer[2] = (gameTimer % 10) + '0';
			displayGameTimer[1] = ((gameTimer / 10) % 10) + '0';
			displayGameTimer[0] = ((gameTimer / 100) % 10) + '0';
			
			
			break;
			
			case GLogicGameOver:
			currentGameState = GLogicGameOver;
			if(highScore < tempScore) {
				highScore = tempScore;
			}
			menuInputDelayCount = 0;
			break;
			
			default:
			break;
	}
	return state;
}

void matrixToDisplay() {
	for(unsigned char y = 0; y < 48;y++) {
		for(unsigned char x = 0; x < 84; x++) {
			nokia_lcd_set_pixel(x,y, displayMatrix[y][x]);
		}
	}
	nokia_lcd_set_cursor(0,40);
	nokia_lcd_write_char('S', 1);
	nokia_lcd_set_cursor(10,40);
	nokia_lcd_write_char(displayScore[0], 1);
	nokia_lcd_set_cursor(15,40);
	nokia_lcd_write_char(displayScore[1], 1);
	nokia_lcd_set_cursor(20,40);
	nokia_lcd_write_char(displayScore[2], 1);
	
	nokia_lcd_set_cursor(30,40);
	nokia_lcd_write_char('L', 1);
	nokia_lcd_set_cursor(35,40);
	nokia_lcd_write_char(':', 1);
	nokia_lcd_set_cursor(40,40);
	nokia_lcd_write_char('0' + playerLives, 1);
	
	nokia_lcd_set_cursor(55,40);
	nokia_lcd_write_char('T', 1);
	nokia_lcd_set_cursor(65,40);
	nokia_lcd_write_char(displayGameTimer[0], 1);
	nokia_lcd_set_cursor(70,40);
	nokia_lcd_write_char(displayGameTimer[1], 1);
	nokia_lcd_set_cursor(75,40);
	nokia_lcd_write_char(displayGameTimer[2], 1);
}

const char MENUMESSAGE1[] = "R:SAVE L:LOAD";
const char MENUMESSAGE2[] = "SEL: CLEAR";
const char MENUMESSAGE3[] = "B. Maze";
//const char RUMBLEMESSAGE1[] = "RUMBLING!";

void menuDisplay() {
	nokia_lcd_set_cursor(0,0);
	nokia_lcd_write_string(MENUMESSAGE3,2);
	
	nokia_lcd_set_cursor(0,20);
	nokia_lcd_write_string(MENUMESSAGE1,1);
	nokia_lcd_set_cursor(0,30);
	nokia_lcd_write_string(MENUMESSAGE2,1);
	
	nokia_lcd_set_cursor(0,40);
	nokia_lcd_write_char('H', 1);
	nokia_lcd_set_cursor(5,40);
	nokia_lcd_write_char('i', 1);
	nokia_lcd_set_cursor(10,40);
	nokia_lcd_write_char('g', 1);
	nokia_lcd_set_cursor(15,40);
	nokia_lcd_write_char('h', 1);
	
	nokia_lcd_set_cursor(25,40);
	nokia_lcd_write_char('S', 1);
	nokia_lcd_set_cursor(30,40);
	nokia_lcd_write_char('c', 1);
	nokia_lcd_set_cursor(35,40);
	nokia_lcd_write_char('o', 1);
	nokia_lcd_set_cursor(40,40);
	nokia_lcd_write_char('r', 1);
	nokia_lcd_set_cursor(45,40);
	nokia_lcd_write_char('e', 1);
	nokia_lcd_set_cursor(50,40);
	nokia_lcd_write_char(':', 1);
	
	nokia_lcd_set_cursor(60,40);
	nokia_lcd_write_char(displayScore[0], 1);
	nokia_lcd_set_cursor(65,40);
	nokia_lcd_write_char(displayScore[1], 1);
	nokia_lcd_set_cursor(70,40);
	nokia_lcd_write_char(displayScore[2], 1);
}

const char LEVELMESSAGE1[] = "Level";
const char LEVELMESSAGE2[] = "Completed";
const char LEVELMESSAGE3[] = "Press Start";

void levelBeatDisplay() {
	nokia_lcd_set_cursor(0,10);
	nokia_lcd_write_string(LEVELMESSAGE1,2);
	nokia_lcd_set_cursor(70,10);
	nokia_lcd_write_char('0'+levelCount, 2);
	nokia_lcd_set_cursor(0,30);
	nokia_lcd_write_string(LEVELMESSAGE2,1);
	nokia_lcd_set_cursor(0,40);
	nokia_lcd_write_string(LEVELMESSAGE3,1);
}

const char GAMEOVERMESSAGE1[] = "GameOver";
const char GAMEOVERMESSAGE2[] = "Score:";
const char GAMEOVERMESSAGE3[] = "Press Start";
void gameOverDisplay() {
	nokia_lcd_set_cursor(0,0);
	nokia_lcd_write_string(GAMEOVERMESSAGE1,2);
	nokia_lcd_set_cursor(0,0);
	nokia_lcd_write_char('G', 2);
	nokia_lcd_set_cursor(0,30);
	nokia_lcd_write_string(GAMEOVERMESSAGE2,1);
	nokia_lcd_set_cursor(40,30);
	nokia_lcd_write_char(displayScore[0], 1);
	nokia_lcd_set_cursor(45,30);
	nokia_lcd_write_char(displayScore[1], 1);
	nokia_lcd_set_cursor(50,30);
	nokia_lcd_write_char(displayScore[2], 1);
	nokia_lcd_set_cursor(0,40);
	nokia_lcd_write_string(GAMEOVERMESSAGE3,1);
}

const char WINMESSAGE1[] = "YOU WIN";
void winDisplay() {
	nokia_lcd_set_cursor(0,0);
	nokia_lcd_write_string(WINMESSAGE1,2);
	nokia_lcd_set_cursor(0,30);
	nokia_lcd_write_string(GAMEOVERMESSAGE2,1);
	nokia_lcd_set_cursor(40,30);
	nokia_lcd_write_char(displayScore[0], 1);
	nokia_lcd_set_cursor(45,30);
	nokia_lcd_write_char(displayScore[1], 1);
	nokia_lcd_set_cursor(50,30);
	nokia_lcd_write_char(displayScore[2], 1);
	nokia_lcd_set_cursor(0,40);
	nokia_lcd_write_string(GAMEOVERMESSAGE3,1);
}
enum LCDDisplayState{LCDDisplayStart, LCDDisplayInit,LCDDisplayMenu, LCDDisplayRunning, LCDDisplayGameOver,LCDDisplayNextLevel, LCDDisplayWin};
	
unsigned char TickLCDDisplay (unsigned char state) {
	switch(state) {
		case LCDDisplayStart:
		state = LCDDisplayInit;
		break;
		
		case LCDDisplayInit:
		state = LCDDisplayMenu;
		break;
		
		case LCDDisplayMenu:
		if(currentGameState == GLogicPlaying) {
			state = LCDDisplayRunning;
		}
		break;
		
		case LCDDisplayRunning:
		if(currentGameState == GLogicMenu) {
			state = LCDDisplayMenu;
		}
		else if(currentGameState == GLogicGameOver) {
			state = LCDDisplayGameOver;
		}
		else if(currentGameState == GLogicLevelComplete) {
			state = LCDDisplayNextLevel;
		}
		else if(currentGameState == GLogicWin) {
			state = LCDDisplayWin;
		}
		break;
		
		case LCDDisplayWin:
		if(currentGameState == GLogicMenu) {
			state = LCDDisplayMenu;
		}
		break;
		
		case LCDDisplayNextLevel:
		if(currentGameState == GLogicPlaying){
			state = LCDDisplayRunning;
		}
		break;
		
		case LCDDisplayGameOver:
		if(currentGameState == GLogicMenu) {
			state = LCDDisplayMenu;
		}
		break;
		
		default:
		state = LCDDisplayStart;
		break;
	}
	switch(state) {
		case LCDDisplayInit:
		break;
		
		case LCDDisplayMenu:
		nokia_lcd_clear();
		menuDisplay();
		nokia_lcd_render();
		break;
		
		case LCDDisplayGameOver:
		nokia_lcd_clear();
		gameOverDisplay();
		nokia_lcd_render();
		break;
		
		case LCDDisplayWin:
		nokia_lcd_clear();
		winDisplay();
		nokia_lcd_render();
		break;
		
		case LCDDisplayRunning:
		nokia_lcd_clear();
		matrixToDisplay();
		nokia_lcd_render();
		break;
		
		case LCDDisplayNextLevel:
		nokia_lcd_clear();
		levelBeatDisplay();
		nokia_lcd_render();
		break;
		
		default:
		break;
	}
	return state;
}

//https://www.hackster.io/techarea98/super-mario-theme-song-with-piezo-buzzer-and-arduino-2cc461
const unsigned char MAXINDEX = 78;
//Mario main theme melody
double melody[78] = {
	NOTE_E7, NOTE_E7, 0, NOTE_E7,
	0, NOTE_C7, NOTE_E7, 0,
	NOTE_G7, 0, 0,  0,
	NOTE_G6, 0, 0, 0,

	NOTE_C7, 0, 0, NOTE_G6,
	0, 0, NOTE_E6, 0,
	0, NOTE_A6, 0, NOTE_B6,
	0, NOTE_AS6, NOTE_A6, 0,

	NOTE_G6, NOTE_E7, NOTE_G7,
	NOTE_A7, 0, NOTE_F7, NOTE_G7,
	0, NOTE_E7, 0, NOTE_C7,
	NOTE_D7, NOTE_B6, 0, 0,

	NOTE_C7, 0, 0, NOTE_G6,
	0, 0, NOTE_E6, 0,
	0, NOTE_A6, 0, NOTE_B6,
	0, NOTE_AS6, NOTE_A6, 0,

	NOTE_G6, NOTE_E7, NOTE_G7,
	NOTE_A7, 0, NOTE_F7, NOTE_G7,
	0, NOTE_E7, 0, NOTE_C7,
	NOTE_D7, NOTE_B6, 0, 0
};
//Mario main them tempo
unsigned char tempo[78] = {
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,

	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,

	9, 9, 9,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,

	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,

	9, 9, 9,
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
};

//https://www.ninsheetmusic.org/browse/series/Bomberman
const unsigned char MAXINDEXBOMB = 16;

double melodyBomb[16] = {
	NOTE_B3, NOTE_B3, NOTE_B4, NOTE_B3,
	NOTE_D4, 0, NOTE_FS4, NOTE_GS4,
	NOTE_A4, 0, NOTE_A4,  0,
	NOTE_GS4, 0, 0, 0
};
//Mario main them tempo
unsigned char tempoBomb[16] = {
	12, 12, 12, 12,
	12, 12, 12, 12,
	12, 12, 12, 12,
	24, 12, 12, 12
};

unsigned short countSound = 0;
const unsigned short PERIODSOUND = 200;
unsigned char noteIndex;
double freqOut = 0.00;

enum SoundState{SoundStart, SoundInit,SoundMenu, SoundRunning, SoundGameOver,SoundNextLevel, SoundWin};

unsigned char TickSound(unsigned char state) {
	switch(state) {
		case SoundStart:
		state = SoundInit;
		break;
		
		case SoundInit:
		state = SoundMenu;
		break;
		
		case SoundMenu:
		if(currentGameState == GLogicPlaying) {
			soundQueueSize = 0;
			soundQueueStart = 0;
			soundQueueEnd = 0;
			soundQueueCount = 0;
			state = SoundRunning;
		}
		break;
		
		case SoundRunning:
		if(currentGameState == GLogicMenu) {
			state = SoundMenu;
		}
		else if(currentGameState == GLogicGameOver) {
			state = SoundGameOver;
		}
		else if(currentGameState == GLogicLevelComplete) {
			state = SoundNextLevel;
			noteIndex = 0;
			freqOut = 0;
			countSound = 0;
		}
		else if(currentGameState == GLogicWin) {
			state = SoundWin;
			noteIndex = 0;
			freqOut = 0;
			countSound = 0;
		}
		break;
		
		case SoundWin:
		if(currentGameState == GLogicMenu) {
			state = SoundMenu;
		}
		break;
		
		case SoundNextLevel:
		if(currentGameState == GLogicPlaying){
			soundQueueSize = 0;
			soundQueueStart = 0;
			soundQueueEnd = 0;
			soundQueueCount = 0;
			state = SoundRunning;
		}
		break;
		
		case SoundGameOver:
		if(currentGameState == GLogicMenu) {
			state = SoundMenu;
		}
		break;
		
		default:
		state = SoundStart;
		break;
	}
	switch(state) {
		case SoundInit:
		
		set_PWM(0);
		break;
		
		case SoundMenu:
		freqOut = 0;
		break;
		
		case SoundGameOver:
		freqOut = 0;
		break;
		
		case SoundWin:
		if(noteIndex < MAXINDEX - 1) {
			if(tempo[noteIndex] >= countSound) {
				if(noteIndex < MAXINDEX - 1) {
					noteIndex++;
					countSound = 0;
				}
			}
			freqOut = melody[noteIndex];
			countSound++;
		}
		break;
		
		case SoundRunning:
		if(soundQueueSize != 0) {
			if(soundDuration > soundQueueCount) {
				freqOut = soundQueue[soundQueueStart] + soundQueueCount;
				soundQueueCount++;
			}
			else {
				if(soundQueueStart >= 2) {
					soundQueueStart = 0;
				}
				else {
					soundQueueStart = soundQueueStart + 1;
				}
				soundQueueSize = soundQueueSize - 1;
				soundQueueCount = 0;
			}
		}
		else {
			freqOut = 0;
		}
		break;
		
		case SoundNextLevel:
		if(noteIndex < MAXINDEXBOMB - 1) {
			if(tempoBomb[noteIndex] >= countSound) {
				if(noteIndex < MAXINDEXBOMB - 1) {
					noteIndex++;
					countSound = 0;
				}
			}
			freqOut = melodyBomb[noteIndex];
			countSound++;
		}
		break;
		
		default:
		break;
	}
	set_PWM(freqOut);
	return state;
}

// --------END User defined FSMs-----------------------------------------------


// Implement scheduler code from PES.
int main(){
	// Set Data Direction Registers
	// Buttons PORTA[0-7], set AVR PORTA to pull down logic
	DDRA = 0x03; PORTA = 0xFC;
	DDRC = 0xFF; PORTC = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	nokia_lcd_init();
	nokia_lcd_clear();
	nokia_lcd_render();
	
	PWM_on();
	// Period for the tasks
	//unsigned long int SMTickSNES_calc = 1;
	unsigned long int SMTickLCD_calc = 10;
	unsigned long int SMTickLogic_calc = 40;
	unsigned long int SMTickSNES_calc = 1;
	unsigned long int SMTickPWMMotor_calc = 10;
	unsigned long int SMTickPWMMotorManager_calc = 10;
	unsigned long int SMTickSound_calc = 20;
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTickLogic_calc, SMTickLCD_calc);
	tmpGCD = findGCD(SMTickSNES_calc, tmpGCD);
	tmpGCD = findGCD(SMTickPWMMotor_calc, tmpGCD);
	tmpGCD = findGCD(SMTickPWMMotorManager_calc, tmpGCD);
	tmpGCD = findGCD(SMTickSound_calc, tmpGCD);
	
	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	//unsigned long int SMTickSNES_period = SMTickSNES_calc/GCD;
	unsigned long int SMTickLCD_period = SMTickLCD_calc/GCD;
	unsigned long int SMTickLogic_period = SMTickLogic_calc/GCD;
	unsigned long int SMTickSNES_period = SMTickSNES_calc/GCD;
	unsigned long int SMTickPWMMotor_period = SMTickPWMMotor_calc/GCD;
	unsigned long int SMTickPWMMotorManager_period = SMTickPWMMotorManager_calc/GCD;
	unsigned long int SMTickSound_period = SMTickSound_calc/GCD;
	
	//Declare an array of tasks 
	static task task1, task2, task3, task4, task5, task6;
	task *tasks[] = {&task1, &task2, &task3, &task4, &task5, &task6};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTickSNES_period;//Task Period.
	task1.elapsedTime = SMTickSNES_period;//Task current elapsed time.
	task1.TickFct = &TickSNESControl;//Function pointer for the tick.
	
	task2.state = -1;//Task initial state.
	task2.period = SMTickLCD_calc;//Task Period.
	task2.elapsedTime = SMTickLCD_period;//Task current elapsed time.
	task2.TickFct = &TickLCDDisplay;//Function pointer for the tick.
	
	task3.state = -1;//Task initial state.
	task3.period = SMTickLogic_calc;//Task Period.
	task3.elapsedTime = SMTickLogic_period;//Task current elapsed time.
	task3.TickFct = &TickGameLogic;//Function pointer for the tick.
	
	task4.state = -1;
	task4.period = SMTickPWMMotor_calc;
	task4.elapsedTime = SMTickPWMMotor_period;
	task4.TickFct = &TickPWMMotor;
	
	task5.state = -1;
	task5.period = SMTickPWMMotorManager_calc;
	task5.elapsedTime = SMTickPWMMotorManager_period;
	task5.TickFct = &TickPWMMotorManager;
	
	task6.state = -1;
	task6.period = SMTickSound_calc;
	task6.elapsedTime = SMTickSound_period;
	task6.TickFct = &TickSound;
	
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	
	unsigned short i; // Scheduler for-loop iterator
	while(1) {
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}

	// Error: Program should not exit!
	return 0;
}
