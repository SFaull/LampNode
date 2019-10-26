/**
 * @file timer.cpp
 * @author Sam Faull
 * @brief Class for basic timekeeping
 * @version NA
 * @date 2019-10-10
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include "timer.h"


/**
 * @brief Set the timer by capturing the current timestamp
 * 
 * @param long
 */
void TimerClass::set(unsigned long* startTime)
{
	*startTime = millis();;  // store the current time
}

/**
* @brief Check whether a given number of ms has passed by passing in a timer variable and the expiry time
* 
* @param startTime timestamp
* @param expiryTime exipre time in ms
* @return true 
* @return false 
*/
bool TimerClass::isExpired(unsigned long startTime, unsigned long expiryTime)
{
	return ((millis() - startTime) >= expiryTime);
}

/**
 * @brief Returns the time elapsed in ms since the timer was started
 * 
 * @param startTime 
 * @return unsigned long 
 */
unsigned long TimerClass::elapsed(unsigned long startTime)
{
	return (millis() - startTime);
}


TimerClass Timer;

