// -----
// OneButton.cpp - Library for detecting button clicks, doubleclicks and long press pattern on a single button.
// This class is implemented for use with the Arduino environment.
// Copyright (c) by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
// More information on: http://www.mathertel.de/Arduino
// -----
// Changelog: see OneButton.h
// -----

#include "OneButton.h"

// ----- Initialization and Default Values -----

OneButton::OneButton(int pin, int activeLow)
{
  _pin = pin;

  _debounceTicks = 50;      // number of millisec that have to pass by before a click is assumed as safe.
  _clickTicks = 600;        // number of millisec that have to pass by before a click is detected.
  _pressTicks = 1000;       // number of millisec that have to pass by before a long button press is detected.
 
  _state = 0; // starting with state 0: waiting for button to be pressed
  _isLongPressed = false;  // Keep track of long press state

  if (activeLow) {
    // the button connects the input pin to GND when pressed.
    _buttonReleased = HIGH; // notPressed
    _buttonPressed = LOW;

    // use the given pin as input and activate internal PULLUP resistor.
    pinMode( pin, INPUT_PULLUP );

  } else {
    // the button connects the input pin to VCC when pressed.
    _buttonReleased = LOW;
    _buttonPressed = HIGH;

    // use the given pin as input
    pinMode(pin, INPUT);
  } // if

  // no functions attached yet: clear all function pointers.
  _clickFunc = NULL;
  _doubleClickFunc = NULL;
  _pressFunc = NULL;
  _longPressStartFunc = NULL;
  _longPressStopFunc = NULL;
  _duringLongPressFunc = NULL;
} // OneButton


// explicitly set the number of millisec that have to pass by before a click is assumed as safe.
void OneButton::setDebounceTicks(int ticks) { 
  _debounceTicks = ticks;
} // setDebounceTicks

// explicitly set the number of millisec that have to pass by before a click is detected.
void OneButton::setClickTicks(int ticks) { 
  _clickTicks = ticks;
} // setClickTicks


// explicitly set the number of millisec that have to pass by before a long button press is detected.
void OneButton::setPressTicks(int ticks) {
  _pressTicks = ticks;
} // setPressTicks


// save function for click event
void OneButton::attachClick(callbackFunction newFunction)
{
  _clickFunc = newFunction;
} // attachClick


// save function for doubleClick event
void OneButton::attachDoubleClick(callbackFunction newFunction)
{
  _doubleClickFunc = newFunction;
} // attachDoubleClick


// save function for press event
// DEPRECATED, is replaced by attachLongPressStart, attachLongPressStop, attachDuringLongPress, 
void OneButton::attachPress(callbackFunction newFunction)
{
  _pressFunc = newFunction;
} // attachPress

// save function for longPressStart event
void OneButton::attachLongPressStart(callbackFunction newFunction)
{
  _longPressStartFunc = newFunction;
} // attachLongPressStart

// save function for longPressStop event
void OneButton::attachLongPressStop(callbackFunction newFunction)
{
  _longPressStopFunc = newFunction;
} // attachLongPressStop

// save function for during longPress event
void OneButton::attachDuringLongPress(callbackFunction newFunction)
{
  _duringLongPressFunc = newFunction;
} // attachDuringLongPress

// function to get the current long pressed state
bool OneButton::isLongPressed(){
  return _isLongPressed;
}

void OneButton::tick(void)
{
  // Detect the input information 
  int buttonLevel = digitalRead(_pin); // current button signal.
  unsigned long now = millis(); // current (relative) time in msecs.

  // Implementation of the state machine
  if (_state == 0) { // waiting for menu pin being pressed.
    if (buttonLevel == _buttonPressed) {
      _state = 1; // step to state 1
      _startTime = now; // remember starting time
    } // if

  } else if (_state == 1) { // waiting for menu pin being released.

    if ((buttonLevel == _buttonReleased) && ((unsigned long)(now - _startTime) < _debounceTicks)) {
      // button was released to quickly so I assume some debouncing.
	  // go back to state 0 without calling a function.
      _state = 0;

    } else if (buttonLevel == _buttonReleased) {
      _state = 2; // step to state 2
      _stopTime = now; // remember stopping time

    } else if ((buttonLevel == _buttonPressed) && ((unsigned long)(now - _startTime) > _pressTicks)) {
      _isLongPressed = true;  // Keep track of long press state
      if (_pressFunc) _pressFunc();
	  if (_longPressStartFunc) _longPressStartFunc();
	  if (_duringLongPressFunc) _duringLongPressFunc();
      _state = 6; // step to state 6
      
    } else {
      // wait. Stay in this state.
    } // if

  } else if (_state == 2) { // waiting for menu pin being pressed the second time or timeout.
    if (_doubleClickFunc == NULL || (unsigned long)(now - _startTime) > _clickTicks) {
      // this was only a single short click
      if (_clickFunc) _clickFunc();
      _state = 0; // restart.

    } else if ((buttonLevel == _buttonPressed) && ((unsigned long)(now - _stopTime) > _debounceTicks)) {
      _state = 3; // step to state 3
      _startTime = now; // remember starting time
    } // if

  } else if (_state == 3) { // waiting for menu pin being released finally.
    // Stay here for at least _debounceTicks because else we might end up in state 1 if the
    // button bounces for too long.
    if (buttonLevel == _buttonReleased && ((unsigned long)(now - _startTime) > _debounceTicks)) {
      // this was a 2 click sequence.
      if (_doubleClickFunc) _doubleClickFunc();
      _state = 0; // restart.
    } // if

  } else if (_state == 6) { // waiting for menu pin being release after long press.
    if (buttonLevel == _buttonReleased) {
	  _isLongPressed = false;  // Keep track of long press state
	  if(_longPressStopFunc) _longPressStopFunc();
      _state = 0; // restart.
    } else {
	  // button is being long pressed
	  _isLongPressed = true; // Keep track of long press state
	  if (_duringLongPressFunc) _duringLongPressFunc();
    } // if  

  } // if  
} // OneButton.tick()


// end.

