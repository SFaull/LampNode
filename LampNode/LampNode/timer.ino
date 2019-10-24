

/* pass this function a pointer to an unsigned long to store the start time for the timer */
void setTimer(unsigned long *startTime)
{
  runTime = millis();    // get time running in ms
  *startTime = runTime;  // store the current time
}

/* call this function and pass it the variable which stores the timer start time and the desired expiry time 
   returns true fi timer has expired */
bool timerExpired(unsigned long startTime, unsigned long expiryTime)
{
  runTime = millis(); // get time running in ms
  if ( (runTime - startTime) >= expiryTime )
    return true;
  else
    return false;
}


void timer_init(void)
{
    /* Start timers */
  setTimer(&readInputTimer);
  setTimer(&readTempTimer);
  setTimer(&ledTimer);
  setTimer(&brightnessTimer);
  setTimer(&twinkleTimer);
  setTimer(&rainbowTimer);
  setTimer(&cycleTimer);
}
