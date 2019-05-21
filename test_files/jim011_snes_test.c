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
	unsigned long int SMTickLED_calc = 13;

	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTickSNES_calc, SMTickLED_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTickSNES_period = SMTickSNES_calc/GCD;
	unsigned long int SMTickLED_period = SMTickLED_calc/GCD;

	//Declare an array of tasks 
	static task task1, task2;
	task *tasks[] = { &task1, &task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTickSNES_period;//Task Period.
	task1.elapsedTime = SMTickSNES_period;//Task current elapsed time.
	task1.TickFct = &TickSNESControl;//Function pointer for the tick.
	
	task2.state = -1;//Task initial state.
	task2.period = SMTickLED_calc;//Task Period.
	task2.elapsedTime = SMTickLED_period;//Task current elapsed time.
	task2.TickFct = &TickLED;//Function pointer for the tick.
	
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
