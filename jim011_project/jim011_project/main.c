/*Jimmy Im : jim011@ucr.edu
 *James Luo : jluo011@ucr.edu
 *Lab Section: 024
 *Assignment: Lab 11 Exercise 4
 *
 *I acknowledge all content contained herein, excluding template or example
 *code, is my own original work.
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <bit.h>
#include <timer.h>
#include <keypad.h>
#include <stdio.h>
#include <io.c>
#include "nokia5110.h"

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
const unsigned short SNESDELAYCLK = 12;

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

//const unsigned short BOMBPERIOD = 

struct playerObj {
	unsigned char playerPosX;
	unsigned char playerPosY;
	unsigned char currentGraphic;
	
	unsigned char isBombPlaced;
	unsigned char bombPosX;
	unsigned char bombPosY;
	unsigned short bombCount;
} player1;

enum objectType {OBJEmpty, OBJWall, OBJPlayer, OBJEnemy, OBJBomb, OBJBlock};
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
 {0x0AAB, 0x0557, 0x3FF,
	 0x020F, 0x0217, 0x020F,
	 0x0207, 0x022F, 0x0257,
 0x03FF, 0x0557, 0x0AAB};

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

/*void checkValidMovement(struct playerObj currentPlayer) {
	if(currentPlayer.playerPosX != 6 && (SNESOutput & 0x0080) == 0x0080) {
		if(objectLocMatrix[currentPlayer.playerPosY][(currentPlayer.playerPosX + 1)] == OBJEmpty){
			objectLocMatrix[currentPlayer.playerPosY][currentPlayer.playerPosX] = OBJEmpty;
			currentPlayer.playerPosX = currentPlayer.playerPosX + 1;
			objectLocMatrix[currentPlayer.playerPosY][currentPlayer.playerPosX] = OBJPlayer;
		}
	}
	else if(currentPlayer.playerPosX != 0 && (SNESOutput & 0x0040) == 0x0040) {
		if(objectLocMatrix[currentPlayer.playerPosY][(currentPlayer.playerPosX - 1)] == OBJEmpty){
			objectLocMatrix[currentPlayer.playerPosY][currentPlayer.playerPosX] = OBJEmpty;
			currentPlayer.playerPosX = currentPlayer.playerPosX - 1;
			objectLocMatrix[currentPlayer.playerPosY][currentPlayer.playerPosX] = OBJPlayer;
		}
	}
	else if(currentPlayer.playerPosY != 0 && (SNESOutput & 0x0010) == 0x0010) {
		if(objectLocMatrix[currentPlayer.playerPosY - 1][(currentPlayer.playerPosX)] == OBJEmpty){
			objectLocMatrix[currentPlayer.playerPosY][currentPlayer.playerPosX] = OBJEmpty;
			currentPlayer.playerPosY = currentPlayer.playerPosY - 1;
			objectLocMatrix[currentPlayer.playerPosY][currentPlayer.playerPosX] = OBJPlayer;
		}
	}
	else if(currentPlayer.playerPosY != 2 && (SNESOutput & 0x0020) == 0x0020) {
		if(objectLocMatrix[currentPlayer.playerPosY + 1][(currentPlayer.playerPosX)] == OBJEmpty){
			objectLocMatrix[currentPlayer.playerPosY][currentPlayer.playerPosX] = OBJEmpty;
			currentPlayer.playerPosY = currentPlayer.playerPosY + 1;
			objectLocMatrix[currentPlayer.playerPosY][currentPlayer.playerPosX] = OBJPlayer;
		}
	}
};*/


enum GameLogicStates{GLogicStart, GLogicInit, GLogicMenu, GLogicPlaying};
unsigned char TickGameLogic(unsigned char state) {
	switch(state) {
		case GLogicStart:
		state = GLogicInit;
		break;
		
		case GLogicInit:
		state = GLogicMenu;
		break;
		
		case GLogicMenu:
		state = GLogicPlaying;
		break;
		
		case GLogicPlaying:
		break;
		
		default:
		state = GLogicStart;
		break;
	}
	switch(state) {
			case GLogicInit: 
			break;
			
			case GLogicMenu:
			for(unsigned char tempY = 0; tempY < 3;tempY++) {
				for(unsigned char tempX = 0; tempX < 7;tempX++) {
					objectLocMatrix[tempY][tempX] = OBJEmpty;
				}
			}
			objectLocMatrix[1][1] = OBJBlock;
			objectLocMatrix[1][3] = OBJBlock;
			objectLocMatrix[1][5] = OBJBlock;
			//objectLocMatrix[0][2] = OBJWall;
			objectLocMatrix[2][0] = OBJWall;
			objectLocMatrix[1][6] = OBJWall;
			objectLocMatrix[0][0] = OBJPlayer;
			player1.playerPosX = 0;
			player1.playerPosY = 0;
			player1.isBombPlaced = 0;
			transferObjToDis();
			break;
			
			case GLogicPlaying:
			//Input and Actions
			if(player1.isBombPlaced == 0 && (SNESOutput & 0x0800) == 0x0800) {
				player1.isBombPlaced = 1;
				player1.bombPosX = player1.playerPosX;
				player1.bombPosY = player1.playerPosY;
				player1.bombCount = 0;
			}
			if(player1.playerPosX != 6 && (SNESOutput & 0x0010) == 0x0010) {
				if(objectLocMatrix[player1.playerPosY][(player1.playerPosX + 1)] == OBJEmpty){
					objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
					player1.playerPosX = player1.playerPosX + 1;
					objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
				}
			}
			else if(player1.playerPosX != 0 && (SNESOutput & 0x0020) == 0x0020) {
				if(objectLocMatrix[player1.playerPosY][(player1.playerPosX - 1)] == OBJEmpty){
					objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
					player1.playerPosX = player1.playerPosX - 1;
					objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
				}
			}
			else if(player1.playerPosY != 0 && (SNESOutput & 0x0080) == 0x0080) {
				if(objectLocMatrix[player1.playerPosY - 1][(player1.playerPosX)] == OBJEmpty){
					objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
					player1.playerPosY = player1.playerPosY - 1;
					objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
				}
			}
			else if(player1.playerPosY != 2 && (SNESOutput & 0x0040) == 0x0040) {
				if(objectLocMatrix[player1.playerPosY + 1][(player1.playerPosX)] == OBJEmpty){
					objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJEmpty;
					player1.playerPosY = player1.playerPosY + 1;
					objectLocMatrix[player1.playerPosY][player1.playerPosX] = OBJPlayer;
				}
			}
			if((player1.playerPosX != player1.bombPosX || player1.playerPosY != player1.bombPosY) && player1.isBombPlaced != 0) {
				objectLocMatrix[player1.bombPosY][player1.bombPosX] = OBJBomb;
			}
			/*if(player1.bombCount < BOMBPERIOD && player1.isBombPlaced != 0) {
				objectLocMatrix[player1.bombPosY][player1.bombPosX] = OBJBomb;
			}*/
			transferObjToDis();
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
}

enum LCDDisplayState{LCDDisplayStart, LCDDisplayInit, LCDDisplayRunning};
	
unsigned char TickLCDDisplay (unsigned char state) {
	switch(state) {
		case LCDDisplayStart:
		state = LCDDisplayInit;
		break;
		
		case LCDDisplayInit:
		state = LCDDisplayRunning;
		break;
		
		case LCDDisplayRunning:
		break;
		
		default:
		state = LCDDisplayStart;
		break;
	}
	switch(state) {
		case LCDDisplayInit:
		nokia_lcd_init();
		nokia_lcd_clear();
		nokia_lcd_render();
		break;
		
		case LCDDisplayRunning:
		nokia_lcd_clear();
		
		matrixToDisplay();
		
		nokia_lcd_render();
		break;
		
		default:
		break;
	}
	return state;
}



// --------END User defined FSMs-----------------------------------------------


// Implement scheduler code from PES.
int main(){
	// Set Data Direction Registers
	// Buttons PORTA[0-7], set AVR PORTA to pull down logic
	DDRA = 0x03; PORTA = 0xFC;
	
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	// Period for the tasks
	unsigned long int SMTickSNES_calc = 1;
	unsigned long int SMTickLCD_calc = 100;
	unsigned long int SMTickLogic_calc = 100;

	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTickSNES_calc, SMTickLCD_calc);
	tmpGCD = findGCD(SMTickLogic_calc, tmpGCD);
	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTickSNES_period = SMTickSNES_calc/GCD;
	unsigned long int SMTickLCD_period = SMTickLCD_calc/GCD;
	unsigned long int SMTickLogic_period = SMTickLogic_calc/GCD;

	//Declare an array of tasks 
	static task task1, task2, task3;
	task *tasks[] = { &task1, &task2, &task3};
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
